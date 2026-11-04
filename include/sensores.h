#pragma once
#include <Arduino.h>

bool iniciarSensores();
bool lerSensores(DadoSensor& dados);
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

bool calibrarInicial(float& pressaoInicial, float& biasAx, float& biasAy, float& biasAz);