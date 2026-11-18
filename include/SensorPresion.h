
#ifndef SENSOR_PRESION_H
#define SENSOR_PRESION_H

#include "Sensor.h"

class SensorPresion : public Sensor {
private:
    double umbralMaximo;  

public:
    SensorPresion(int _id, const std::string& _ub, double _lect,
                  double _umbral = 120.0);

    void procesarLectura() override;
};

#endif
