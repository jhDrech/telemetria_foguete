#pragma once

#include <Arduino.h>

struct DadoSensor {
    uint32_t timestamp_us;

    float pressao_pa;
    float temperatura_c;
    float altitude_m;

    float ax, ay, az;
    float gx, gy, gz;
    float mx, my, mz;

    //orientação estimada pelo DMP
    float q0,q1,q2,q3;
    bool quaternion_valido;
};

struct DadoTratado{
    uint32_t timestamp_us;

    float altitude_relativa_m;

    float ax;
    float ay;
    float az;

    float gx_corrigido;
    float gy_corrigido;
    float gz_corrigido;

    float mx;
    float my;
    float mz;

    float q0,q1,q2,q3;
    bool quaternion_valido;
};

struct Calibracao {
    float altitudeInicial_m;

    float giroBiasX;
    float giroBiasY;
    float giroBiasZ;

};
