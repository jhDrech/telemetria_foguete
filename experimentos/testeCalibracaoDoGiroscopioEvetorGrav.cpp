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

    Serial.println("Mantenha o sensor completamente parado.");
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
        "gx,gy,gz,"
        "gxCorrigido,gyCorrigido,gzCorrigido,"
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


    atualizarOrientacao(
        dado,
        dt,
        biasGx,
        biasGy,
        biasGz
    );


    float gxCorrigido = dado.gx - biasGx;
    float gyCorrigido = dado.gy - biasGy;
    float gzCorrigido = dado.gz - biasGz;


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


    Serial.print(dado.timestamp_ms);
    Serial.print(",");

    Serial.print(dado.gx, 6);
    Serial.print(",");
    Serial.print(dado.gy, 6);
    Serial.print(",");
    Serial.print(dado.gz, 6);
    Serial.print(",");

    Serial.print(gxCorrigido, 6);
    Serial.print(",");
    Serial.print(gyCorrigido, 6);
    Serial.print(",");
    Serial.print(gzCorrigido, 6);
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