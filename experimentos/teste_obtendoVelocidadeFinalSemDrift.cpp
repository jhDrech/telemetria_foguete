#include <Arduino.h>
#include "sensores.h"

DadoSensor dado;

uint32_t timestampAnterior = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    iniciarSensores();

    Serial.println("Estabilizando orientacao...");

    // Deixa o Madgwick convergir antes de começar
    // a integrar a velocidade.
    uint32_t inicio = millis();

    while(millis() - inicio < 10000){

        lerSensores(dado);

        // evita as primeiras leituras NaN do ICM
        if(!isnan(dado.ax) && !isnan(dado.gx)){

            if(timestampAnterior != 0){

                float dt =
                    (dado.timestamp_ms - timestampAnterior)
                    / 1000.0f;

                if(dt > 0){
                    atualizarOrientacao(dado, dt);
                }
            }

            timestampAnterior = dado.timestamp_ms;
        }

        delay(16);
    }

    // Começaremos um novo cálculo de dt para o teste
    timestampAnterior = 0;

    Serial.println(
        "timestamp_ms,axLinear,ayLinear,azLinear,"
        "aceleracaoVertical,velocidadeVertical"
    );
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

    // Atualiza a orientação
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

    // Aceleração no eixo vertical
    float aceleracaoVertical =
        obterAceleracaoVertical(
            axLinear,
            ayLinear,
            azLinear
        );

    // Integra aceleração -> velocidade
    float velocidadeVertical =
        obterVelocidadeVertical(
            aceleracaoVertical,
            dt
        );

    // CSV para Serial Studio
    Serial.print(dado.timestamp_ms);
    Serial.print(",");

    Serial.print(axLinear, 4);
    Serial.print(",");

    Serial.print(ayLinear, 4);
    Serial.print(",");

    Serial.print(azLinear, 4);
    Serial.print(",");

    Serial.print(aceleracaoVertical, 4);
    Serial.print(",");

    Serial.println(velocidadeVertical, 4);

    delay(16);
}