// =================================================================
// src/Sensor.cpp  (Clase Base Abstracta - Unidad 8)
// =================================================================
#include "../include/Sensor.h"

Sensor::Sensor(int _id, const std::string& _ub, double _lect)
    : id(_id), ubicacion(_ub), lectura(_lect) {}

Sensor::~Sensor() {
    std::cout << "[DESTRUCTOR] Sensor ID " << id << " liberado del Heap." << std::endl;
}

int Sensor::getId() const { return id; }
const std::string& Sensor::getUbicacion() const { return ubicacion; }
double Sensor::getLectura() const { return lectura; }

std::ostream& operator<<(std::ostream& os, const Sensor& s) {
    os << "Sensor ID: " << s.id << " | Ubicacion: " << s.ubicacion
       << " | Valor: " << s.lectura;
    return os;
}
