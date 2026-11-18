
#ifndef SERVIDORSOCKETS_H
#define SERVIDORSOCKETS_H

#include <string>

#include "Sensor.h"
#include "ListaGen.h"
#include "MuestraSensor.h"

class ServidorSMTI {
private:
    int  server_fd;
    int  puerto;
    bool cierreSolicitado;
    ListaGen<Sensor*>&       sensores;    
    ListaGen<MuestraSensor>& historial;   

    void atenderCliente(int client_fd, const std::string& ipCliente);
    void procesarLinea(int client_fd, const std::string& linea, const std::string& ipCliente);

    static MuestraSensor parsearTrama(const std::string& linea, std::string& ubicacion);
    static Sensor*       crearSensor(const MuestraSensor& m, const std::string& ubicacion);
    static void          enviarTexto(int fd, const std::string& texto);

public:
    ServidorSMTI(int _puerto, ListaGen<Sensor*>& _sensores, ListaGen<MuestraSensor>& _historial);
    ~ServidorSMTI();

    ServidorSMTI(const ServidorSMTI&) = delete;
    ServidorSMTI& operator=(const ServidorSMTI&) = delete;

    void iniciar();    
    void ejecutar();   
    void detener();   
};

typedef ServidorSMTI ServidorSockets;

#endif
