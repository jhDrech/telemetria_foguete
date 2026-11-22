#include <Arduino.h>
#include "sensores.h"
#include <Adafruit_AHRS.h>

extern Adafruit_Madgwick filtroAccel;

DadoSensor dado;

uint32_t timestampAnterior = 0;

float biasGx = 0.0f;
float biasGy = 0.0f;
float biasGz = 0.0f;

void setup() {

    Serial.begin(115200);
    delay(2000);

    iniciarSensores();

    Serial.println("Mantenha o sensor parado na posicao de lancamento.");
    Serial.println("Calibrando giroscopio...");

    calibrarGiroscopio(
        biasGx,
        biasGy,
        biasGz
    );

    Serial.print("biasGx:");
    Serial.println(biasGx, 6);

    Serial.print("biasGy:");
    Serial.println(biasGy, 6);

    Serial.print("biasGz:");
    Serial.println(biasGz, 6);


    Serial.println("Estabilizando Madgwick...");

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

                atualizarOrientacao(
                    dado,
                    dt,
                    biasGx,
                    biasGy,
                    biasGz
                );
            }
        }

        timestampAnterior = dado.timestamp_ms;

        delay(16);
    }

    timestampAnterior = 0;

    Serial.println(
        "timestamp_ms,"
        "ax,ay,az,"
        "gravX,gravY,gravZ,"
        "gxCorrigido,gyCorrigido,gzCorrigido"
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
    // GIROSCOPIO CORRIGIDO
    // -----------------------------

    float gxCorrigido =
        dado.gx - biasGx;

    float gyCorrigido =
        dado.gy - biasGy;

    float gzCorrigido =
        dado.gz - biasGz;


    // -----------------------------
    // ATUALIZA MADGWICK
    // -----------------------------

    atualizarOrientacao(
        dado,
        dt,
        biasGx,
        biasGy,
        biasGz
    );


    // -----------------------------
    // GRAVIDADE ESTIMADA
    // -----------------------------

    float gravX;
    float gravY;
    float gravZ;

    filtroAccel.getGravityVector(
        &gravX,
        &gravY,
        &gravZ
    );

    gravX *= 9.80665f;
    gravY *= 9.80665f;
    gravZ *= 9.80665f;


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

    Serial.print(gravX, 4);
    Serial.print(",");
    Serial.print(gravY, 4);
    Serial.print(",");
    Serial.print(gravZ, 4);
    Serial.print(",");

    Serial.print(gxCorrigido, 6);
    Serial.print(",");
    Serial.print(gyCorrigido, 6);
    Serial.print(",");
    Serial.println(gzCorrigido, 6);

    delay(16);
}