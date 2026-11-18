
#include "../include/ServidorSockets.h"
#include "../include/ExcepcionesSMTI.h"
#include "../include/SensorTemperatura.h"
#include "../include/SensorPresion.h"
#include "../include/SensorCaudal.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

const std::size_t MAX_LINEA = 4096;   
const int TIMEOUT_RECV_SEG  = 10;     
volatile sig_atomic_t g_detener = 0;  

void manejarSenal(int) { g_detener = 1; }

std::string recortar(const std::string& s) {
    const char* blancos = " \t\r\n";
    std::size_t ini = s.find_first_not_of(blancos);
    if (ini == std::string::npos) return "";
    std::size_t fin = s.find_last_not_of(blancos);
    return s.substr(ini, fin - ini + 1);
}

std::string aMayusculas(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
    return s;
}

std::vector<std::string> dividir(const std::string& s, char sep) {
    std::vector<std::string> partes;
    std::size_t ini = 0;
    while (true) {
        std::size_t pos = s.find(sep, ini);
        if (pos == std::string::npos) {
            partes.push_back(s.substr(ini));
            break;
        }
        partes.push_back(s.substr(ini, pos - ini));
        ini = pos + 1;
    }
    return partes;
}

} 
ServidorSMTI::ServidorSMTI(int _puerto, ListaGen<Sensor*>& _sensores,
                           ListaGen<MuestraSensor>& _historial)
    : server_fd(-1), puerto(_puerto), cierreSolicitado(false),
      sensores(_sensores), historial(_historial) {}

ServidorSMTI::~ServidorSMTI() {
    detener();  
}

void ServidorSMTI::detener() {
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
}

void ServidorSMTI::iniciar() {
    if (server_fd >= 0) {
        throw std::runtime_error("Error SMTI: el servidor ya fue iniciado.");
    }
    if (puerto < 1 || puerto > 65535) {
        throw std::runtime_error("Error SMTI: puerto fuera de rango (1-65535): " +
                                 std::to_string(puerto));
    }

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejarSenal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error(
            describirErrno("Error POSIX: No se pudo instanciar el socket", errno));
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        int e = errno;
        detener();
        throw std::runtime_error(describirErrno("Error POSIX: setsockopt(SO_REUSEADDR) fallo", e));
    }

    sockaddr_in direccion;
    std::memset(&direccion, 0, sizeof(direccion));
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = htonl(INADDR_ANY);             
    direccion.sin_port = htons(static_cast<uint16_t>(puerto));  

    // bind()
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&direccion), sizeof(direccion)) < 0) {
        int e = errno;
        detener();
        if (e == EADDRINUSE) {
            throw std::runtime_error("Error POSIX: Enlace (bind) fallido, el puerto " +
                                     std::to_string(puerto) +
                                     " esta ocupado por otro proceso.");
        }
        throw std::runtime_error(describirErrno(
            "Error POSIX: Enlace (bind) fallido en puerto " + std::to_string(puerto), e));
    }

    if (listen(server_fd, 5) < 0) {
        int e = errno;
        detener();
        throw std::runtime_error(describirErrno("Error POSIX: Fallo la operacion listen", e));
    }

    std::cout << "[SERVIDORSockets] Escuchando telemetria en puerto " << puerto
              << "... (Ctrl+C o comando SALIR para terminar)" << std::endl;
}

void ServidorSMTI::ejecutar() {
    if (server_fd < 0) {
        throw std::runtime_error("Error SMTI: llame a iniciar() antes de ejecutar().");
    }

    while (!g_detener && !cierreSolicitado) {
        sockaddr_in cliente_addr;
        socklen_t addr_len = sizeof(cliente_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&cliente_addr), &addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) continue;   
            throw std::runtime_error(
                describirErrno("Error POSIX: No se pudo aceptar la conexion del cliente", errno));
        }

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &cliente_addr.sin_addr, ip, sizeof(ip));
        std::cout << "[SERVIDORSockets] Cliente conectado: " << ip << std::endl;

        try {
            atenderCliente(client_fd, ip);
        } catch (...) {
            close(client_fd);
            throw;
        }
        close(client_fd);
        std::cout << "[SERVIDORSockets] Cliente " << ip << " desconectado." << std::endl;
    }

    if (g_detener) {
        std::cout << "\n[SERVIDORSockets] Senal de terminacion recibida." << std::endl;
    }
}

void ServidorSMTI::atenderCliente(int client_fd, const std::string& ipCliente) {
    
    timeval tv;
    tv.tv_sec = TIMEOUT_RECV_SEG;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string pendiente;
    char buffer[1024];

    while (!g_detener && !cierreSolicitado) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

        if (n > 0) {
            pendiente.append(buffer, static_cast<std::size_t>(n));

            std::size_t pos;
            while ((pos = pendiente.find('\n')) != std::string::npos) {
                std::string linea = pendiente.substr(0, pos);
                pendiente.erase(0, pos + 1);
                procesarLinea(client_fd, linea, ipCliente);
                if (cierreSolicitado) return;
            }

            if (pendiente.size() > MAX_LINEA) {
                enviarTexto(client_fd, "ERR SMTI: trama demasiado larga, conexion cerrada.\n");
                std::cerr << "[SERVIDORSockets] Trama excesiva de " << ipCliente
                          << ", se cierra la conexion." << std::endl;
                pendiente.clear();
                break;
            }
        } else if (n == 0) {
            break;                                   
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "[SERVIDORSockets] Tiempo de espera agotado con " << ipCliente
                          << "." << std::endl;
            } else {
                std::cerr << describirErrno("[SERVIDORSockets] Error en recv()", errno)
                          << std::endl;
            }
            break;   
        }
    }

    if (!pendiente.empty() && !cierreSolicitado) {
        procesarLinea(client_fd, pendiente, ipCliente);
    }
}

void ServidorSMTI::procesarLinea(int client_fd, const std::string& cruda,
                                 const std::string& ipCliente) {
    std::string linea = recortar(cruda);
    if (linea.empty()) return;

    if (aMayusculas(linea) == "SALIR") {
        cierreSolicitado = true;
        enviarTexto(client_fd, "ACK SMTI: servidor cerrando.\n");
        std::cout << "[SERVIDORSockets] Comando SALIR recibido." << std::endl;
        return;
    }

    Sensor* nuevo = nullptr;
    try {
        std::string ubicacion = "Nodo-" + ipCliente;
        MuestraSensor m = parsearTrama(linea, ubicacion);

        historial.insertar(m);
        nuevo = crearSensor(m, ubicacion);   
        sensores.insertar(nuevo);
        nuevo = nullptr;                     

        std::ostringstream ack;
        ack << "ACK SMTI: " << m.tipo << " id=" << m.idSensor
            << " valor=" << m.valor << " registrado.\n";
        enviarTexto(client_fd, ack.str());
        std::cout << "[SERVIDORSockets] Trama aceptada: " << linea << std::endl;

    } catch (const ExcepcionTrama& e) {
        std::cerr << "[SERVIDORSockets] " << e.what() << " <- \"" << linea << "\"" << std::endl;
        enviarTexto(client_fd, std::string("ERR SMTI: ") + e.what() + "\n");
    } catch (...) {
        delete nuevo;   
        throw;
    }
}

MuestraSensor ServidorSMTI::parsearTrama(const std::string& linea, std::string& ubicacion) {
    std::vector<std::string> campos = dividir(linea, ';');
    if (campos.size() < 3 || campos.size() > 4) {
        throw ExcepcionTrama("se esperaba ID;TIPO;VALOR[;UBICACION]");
    }

    
    std::string sId = recortar(campos[0]);
    char* fin = nullptr;
    errno = 0;
    long id = std::strtol(sId.c_str(), &fin, 10);
    if (sId.empty() || *fin != '\0' || errno == ERANGE || id <= 0 || id > INT_MAX) {
        throw ExcepcionTrama("ID de sensor invalido '" + sId + "'");
    }

    std::string tipo = aMayusculas(recortar(campos[1]));
    if (tipo != "TEMPERATURA" && tipo != "PRESION" && tipo != "CAUDAL") {
        throw ExcepcionTrama("tipo desconocido '" + tipo +
                             "' (use TEMPERATURA, PRESION o CAUDAL)");
    }

    std::string sVal = recortar(campos[2]);
    errno = 0;
    double valor = std::strtod(sVal.c_str(), &fin);
    if (sVal.empty() || *fin != '\0' || !std::isfinite(valor)) {
        throw ExcepcionTrama("valor numerico invalido '" + sVal + "'");
    }

    if (campos.size() == 4) {
        std::string ub = recortar(campos[3]);
        if (!ub.empty()) ubicacion = ub;
    }

    MuestraSensor m;
    std::memset(&m, 0, sizeof(m));
    m.idSensor = static_cast<int>(id);
    m.valor = valor;
    std::strncpy(m.tipo, tipo.c_str(), sizeof(m.tipo) - 1);   
    return m;
}

Sensor* ServidorSMTI::crearSensor(const MuestraSensor& m, const std::string& ubicacion) {
    const std::string tipo(m.tipo);
    if (tipo == "TEMPERATURA") return new SensorTemperatura(m.idSensor, ubicacion, m.valor);
    if (tipo == "PRESION")     return new SensorPresion(m.idSensor, ubicacion, m.valor);
    if (tipo == "CAUDAL")      return new SensorCaudal(m.idSensor, ubicacion, m.valor);
    throw ExcepcionTrama("tipo desconocido '" + tipo + "'");
}

void ServidorSMTI::enviarTexto(int fd, const std::string& texto) {
    std::size_t enviados = 0;
    while (enviados < texto.size()) {
        ssize_t n = send(fd, texto.c_str() + enviados, texto.size() - enviados, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;   
        }
        enviados += static_cast<std::size_t>(n);
    }
}
