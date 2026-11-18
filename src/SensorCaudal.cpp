// =================================================================
// src/SensorCaudal.cpp  (Clase Derivada Polimorfica - Unidad 8)
// =================================================================
#include "../include/SensorCaudal.h"

SensorCaudal::SensorCaudal(int _id, const std::string& _ub, double _lect,
                           double _umbral)
    : Sensor(_id, _ub, _lect), umbralMinimo(_umbral) {}

void SensorCaudal::procesarLectura() {
    std::cout << "[CAUDAL POLIMORFICO] " << *this << " L/min";
    if (lectura < umbralMinimo) {
        std::cout << " --> ALERTA: CAUDAL BAJO!";
    }
    std::cout << std::endl;
}
