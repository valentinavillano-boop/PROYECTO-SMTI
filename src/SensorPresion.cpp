// =================================================================
// src/SensorPresion.cpp  (Clase Derivada Polimorfica - Unidad 8)
// =================================================================
#include "../include/SensorPresion.h"

SensorPresion::SensorPresion(int _id, const std::string& _ub, double _lect,
                             double _umbral)
    : Sensor(_id, _ub, _lect), umbralMaximo(_umbral) {}

void SensorPresion::procesarLectura() {
    std::cout << "[PRESION POLIMORFICO] " << *this << " kPa";
    if (lectura > umbralMaximo) {
        std::cout << " --> ALERTA: SOBREPRESION CRITICA!";
    }
    std::cout << std::endl;
}
