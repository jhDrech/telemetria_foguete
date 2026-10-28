#pragma once
#include <Arduino.h>
struct DadoSensor{
    uint32_t timestamp_ms;

    float ax, ay, az;
    float pressao;
    float temp;
    float gx,gy,gz;
    float mx, my, mz;
};

struct DadoEstimado{
    float altitude_relativa;
    float velocidade_vertical;
};

bool iniciarSensores();
bool lerSensores(DadoSensor& dados);