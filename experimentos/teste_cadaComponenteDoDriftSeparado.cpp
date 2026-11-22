#include <Arduino.h>
#include "sensores.h"
#include <Adafruit_AHRS.h>

// usa o mesmo filtro definido em sensores.cpp
extern Adafruit_Madgwick filtroAccel;

DadoSensor dado;

uint32_t timestampAnterior = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    iniciarSensores();

    Serial.println("Mantenha o sensor completamente parado.");
    Serial.println("Estabilizando Madgwick...");

    uint32_t inicio = millis();

    // deixa o filtro convergir antes de começar a registrar
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

    timestampAnterior = 0;

    Serial.println(
        "timestamp_ms,"
        "ax,ay,az,"
        "gx,gy,gz,"
        "gravX,gravY,gravZ,"
        "axLinear,ayLinear,azLinear,"
        "aceleracaoVertical"
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

    // -----------------------------
    // atualiza Madgwick
    // -----------------------------

    atualizarOrientacao(dado, dt);


    // -----------------------------
    // vetor de gravidade estimado
    // -----------------------------

    float gravX;
    float gravY;
    float gravZ;

    filtroAccel.getGravityVector(
        &gravX,
        &gravY,
        &gravZ
    );

    // converte de g para m/s²
    gravX *= 9.80665f;
    gravY *= 9.80665f;
    gravZ *= 9.80665f;


    // -----------------------------
    // aceleração linear
    // -----------------------------

    float axLinear;
    float ayLinear;
    float azLinear;

    obterAceleracaoLinear(
        dado,
        axLinear,
        ayLinear,
        azLinear
    );


    // -----------------------------
    // aceleração vertical
    // -----------------------------

    float aceleracaoVertical =
        obterAceleracaoVertical(
            axLinear,
            ayLinear,
            azLinear
        );


    // -----------------------------
    // CSV
    // -----------------------------

    Serial.print(dado.timestamp_ms);
    Serial.print(",");

    Serial.print(dado.ax, 4);
    Serial.print(",");
    Serial.print(dado.ay, 4);
    Serial.print(",");
    Serial.print(dado.az, 4);
    Serial.print(",");

    Serial.print(dado.gx, 6);
    Serial.print(",");
    Serial.print(dado.gy, 6);
    Serial.print(",");
    Serial.print(dado.gz, 6);
    Serial.print(",");

    Serial.print(gravX, 4);
    Serial.print(",");
    Serial.print(gravY, 4);
    Serial.print(",");
    Serial.print(gravZ, 4);
    Serial.print(",");

    Serial.print(axLinear, 4);
    Serial.print(",");
    Serial.print(ayLinear, 4);
    Serial.print(",");
    Serial.print(azLinear, 4);
    Serial.print(",");

    Serial.println(aceleracaoVertical, 4);

    delay(16);
}