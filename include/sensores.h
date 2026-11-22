#pragma once
#include <Arduino.h>

bool iniciarSensores();
struct DadoSensor{
    uint32_t timestamp_ms;
    
    float pressao = NAN;
    float temp = NAN;
    float ax = NAN;
    float ay = NAN;
    float az = NAN;

    float gx = NAN;
    float gy = NAN;
    float gz = NAN;

};
bool lerSensores(DadoSensor& dado);

struct DadoEstimado{
    // são definidos apenas após o lançamento depois da fase de processamento de dados brutos
    float ax = NAN;
    float ay = NAN;
    float az = NAN;

    float gx = NAN;
    float gy = NAN;
    float gz = NAN;

    // obtidos durante o voo para definição dos estados
    float altitude_relativa = NAN;
    float velocidade_vertical = NAN;
};

bool atualizarPressaoInicial(float& pressaoAtual);
float obterAlturaRelativa(float pressaoInicial, float pressaoAtual);

void atualizarOrientacao(const DadoSensor& dado, float dt, float biasGx, float biasGy, float biasGz);

void obterAceleracaoLinear(const DadoSensor& dado,float& axLinear,float& ayLinear,float& azLinear);

float obterAceleracaoVertical(float axLinear,float ayLinear,float azLinear);

float calibrarBiasAceleracaoVertical(DadoSensor amostras[],int quantidadeAmostras);

float obterVelocidadeVertical(float aceleracaoVertical, float dt);

float obterAceleracaoTotal(DadoSensor& dado);

bool reconhecerDisparo(float accelTotal,float alturaRelativa);

bool processarDadosBaixados(const String& linha,DadoSensor& dado);

void calibrarGiroscopio(
    float& biasGx,
    float& biasGy,
    float& biasGz
);