#include <Arduino.h>
#include "sensores.h"

DadoSensor dado;

uint32_t timestampAnterior = 0;

float biasAceleracaoVertical = 0.0f;

void setup() {
    Serial.begin(115200);
    delay(2000);

    iniciarSensores();

    Serial.println("Mantenha o sensor parado na posicao de lancamento.");
    Serial.println("Estabilizando Madgwick...");

    // -----------------------------
    // 1. CONVERGENCIA DO MADGWICK
    // -----------------------------

    uint32_t inicio = millis();

    while(millis() - inicio < 10000){

        lerSensores(dado);

        if(isnan(dado.ax) || isnan(dado.gx)){
            delay(10);
            continue;
        }

        if(timestampAnterior != 0){

            float dt =
                (dado.timestamp_ms - timestampAnterior)
                / 1000.0f;

            if(dt > 0){
                atualizarOrientacao(dado, dt);
            }
        }

        timestampAnterior = dado.timestamp_ms;

        delay(16);
    }


    // -----------------------------
    // 2. CALCULO DO BIAS
    // -----------------------------

    Serial.println("Calculando bias da aceleracao vertical...");
    Serial.println("NAO MEXA NO SENSOR.");

    float soma = 0.0f;
    int quantidade = 0;

    inicio = millis();

    while(millis() - inicio < 5000){

        lerSensores(dado);

        if(isnan(dado.ax) || isnan(dado.gx)){
            delay(10);
            continue;
        }

        float dt =
            (dado.timestamp_ms - timestampAnterior)
            / 1000.0f;

        timestampAnterior = dado.timestamp_ms;

        if(dt <= 0){
            continue;
        }

        atualizarOrientacao(dado, dt);

        float axLinear;
        float ayLinear;
        float azLinear;

        obterAceleracaoLinear(
            dado,
            axLinear,
            ayLinear,
            azLinear
        );

        float aceleracaoVertical =
            obterAceleracaoVertical(
                axLinear,
                ayLinear,
                azLinear
            );

        soma += aceleracaoVertical;
        quantidade++;

        delay(16);
    }

    if(quantidade > 0){
        biasAceleracaoVertical =
            soma / quantidade;
    }

    Serial.print("Bias calculado: ");
    Serial.println(biasAceleracaoVertical, 6);

    Serial.println("INICIO DO TESTE");

    Serial.println(
        "timestamp_ms,aceleracaoVertical,"
        "aceleracaoCorrigida,velocidadeVertical"
    );

    timestampAnterior = 0;
}


void loop() {

    lerSensores(dado);

    if(isnan(dado.ax) || isnan(dado.gx)){
        return;
    }

    if(timestampAnterior == 0){
        timestampAnterior = dado.timestamp_ms;
        return;
    }

    float dt =
        (dado.timestamp_ms - timestampAnterior)
        / 1000.0f;

    timestampAnterior = dado.timestamp_ms;

    if(dt <= 0){
        return;
    }


    // Atualiza orientacao
    atualizarOrientacao(dado, dt);


    // Remove gravidade
    float axLinear;
    float ayLinear;
    float azLinear;

    obterAceleracaoLinear(
        dado,
        axLinear,
        ayLinear,
        azLinear
    );


    // Calcula aceleracao vertical
    float aceleracaoVertical =
        obterAceleracaoVertical(
            axLinear,
            ayLinear,
            azLinear
        );


    // Remove o bias medido em repouso
    float aceleracaoCorrigida =
        aceleracaoVertical - biasAceleracaoVertical;


    // Integra a aceleracao corrigida
    float velocidadeVertical =
        obterVelocidadeVertical(
            aceleracaoCorrigida,
            dt
        );


    // CSV
    Serial.print(dado.timestamp_ms);
    Serial.print(",");

    Serial.print(aceleracaoVertical, 4);
    Serial.print(",");

    Serial.print(aceleracaoCorrigida, 4);
    Serial.print(",");

    Serial.println(velocidadeVertical, 4);

    delay(16);
}