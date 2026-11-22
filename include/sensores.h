#pragma once
#include <Arduino.h>

extern bool icmDisponivel;
bool iniciarSensores();
struct DadoSensor{
    uint32_t timestamp_ms;
    
    float pressao = NAN;
    float temperatura = NAN;
    float ax = NAN;
    float ay = NAN;
    float az = NAN;

    float gx = NAN;
    float gy = NAN;
    float gz = NAN;

};
bool lerSensores(DadoSensor& dados);

struct DadoEstimado{
    // são definidos apenas após o lançamento depois da fase de processamento de dados brutos
    float ax = NAN;
    float ay = NAN;
    float az = NAN;

    float gx = NAN;
    float gy = NAN;
    float gz = NAN;
    // obtidos durante o voo para definição dos estados
    float altura_relativa = NAN;
    float velocidade_total = NAN;
    // float velocidade_vertical = NAN;
};

bool atualizarPressaoInicial(float& pressaoAtual);
float obterAlturaRelativa(float pressaoInicial, float pressaoAtual);
