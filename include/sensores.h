#pragma once

#include <Arduino.h>

struct DadoSensor
{
    uint32_t timestamp_ms;

    float pressao = NAN;
    float temp = NAN;

    // O ICM20948 e usado somente como acelerometro.
    float ax = NAN;
    float ay = NAN;
    float az = NAN;
};

struct DadoEstimado
{
    // Estimativa barometrica usada durante o voo.
    float altitude_relativa = NAN;
};

bool iniciarSensores();

bool lerSensores(DadoSensor &dado);

bool atualizarPressaoInicial(float &pressaoInicial);

float obterAlturaRelativa(
    float pressaoInicial,
    float pressaoAtual);

float obterAceleracaoTotal(const DadoSensor &dado);
