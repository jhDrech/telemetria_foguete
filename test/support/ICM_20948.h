#pragma once
#include "Wire.h"
constexpr int ICM_20948_Stat_Ok = 0;
namespace icmSimulado {
    inline bool presente = false;
    inline bool pronto = false;
    inline bool leituraOk = true;
}
class ICM_20948_I2C {
public:
    int status = 1;
    void begin(TwoWire&, int) { status = icmSimulado::presente ? 0 : 1; }
    bool dataReady() { return icmSimulado::pronto; }
    void getAGMT() { status = icmSimulado::leituraOk ? 0 : 1; }
    float accX() { return 1000.0f; }
    float accY() { return 0.0f; }
    float accZ() { return -1000.0f; }
    float gyrX() { return 180.0f; }
    float gyrY() { return 0.0f; }
    float gyrZ() { return -180.0f; }
    float magX() { return 12.0f; }
    float magY() { return 13.0f; }
    float magZ() { return 14.0f; }
};
