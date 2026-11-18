
#ifndef SENSOR_CAUDAL_H
#define SENSOR_CAUDAL_H

#include "Sensor.h"

class SensorCaudal : public Sensor {
private:
    double umbralMinimo;   

public:
    SensorCaudal(int _id, const std::string& _ub, double _lect,
                 double _umbral = 5.0);

    void procesarLectura() override;
};

#endif
