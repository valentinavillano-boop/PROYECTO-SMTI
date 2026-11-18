
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "../include/ExcepcionesSMTI.h"
#include "../include/ListaGen.h"
#include "../include/MuestraSensor.h"
#include "../include/Sensor.h"
#include "../include/ServidorSockets.h"

int main(int argc, char* argv[]) {
    std::cout << "==================================================" << std::endl;
    std::cout << " SMTI: SERVIDOR DE TELEMETRIA DISTRIBUIDA (UNIDAD 9)" << std::endl;
    std::cout << "==================================================" << std::endl;

    int puerto = 8080;
    if (argc >= 2) {
        char* fin = nullptr;
        long p = std::strtol(argv[1], &fin, 10);
        if (*argv[1] == '\0' || *fin != '\0' || p < 1 || p > 65535) {
            std::cerr << "[ERROR] Puerto invalido: " << argv[1] << std::endl;
            return 1;
        }
        puerto = static_cast<int>(p);
    }

    ListaGen<Sensor*>       listaPlanta;
    ListaGen<MuestraSensor> historial;
    int codigoSalida = 0;

    try {
        ServidorSMTI servidor(puerto, listaPlanta, historial);
        servidor.iniciar();    
        servidor.ejecutar();   
    } catch (const std::runtime_error& e) {
        // Fallo de socket, puerto ocupado, etc.
        std::cerr << "[EXCEPCION GRAVE SMTI] " << e.what() << std::endl;
        codigoSalida = 1;
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPCION INESPERADA] " << e.what() << std::endl;
        codigoSalida = 1;
    }

    std::cout << "\n--- HISTORIAL DE MUESTRAS RECIBIDAS (" << historial.tamano()
              << ") ---" << std::endl;
    historial.recorrer([](const MuestraSensor& m) {
        std::cout << "  id=" << m.idSensor << " | tipo=" << m.tipo
                  << " | valor=" << m.valor << std::endl;
    });

    std::cout << "\n--- PROCESAMIENTO POLIMORFICO DE TELEMETRIA ("
              << listaPlanta.tamano() << " sensores) ---" << std::endl;
    listaPlanta.procesarTodo();

    std::cout << "\n--- LIBERANDO MEMORIA DEL HEAP ---" << std::endl;
    return codigoSalida;   
}
