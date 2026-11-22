#include <Arduino.h>
#include "sensores.h"
#include <Adafruit_AHRS.h>

extern Adafruit_Madgwick filtroAccel;

DadoSensor dado;

float biasGx = 0.0f;
float biasGy = 0.0f;
float biasGz = 0.0f;

uint32_t timestampAnterior = 0;

void setup() {

    Serial.begin(115200);
    delay(2000);

    iniciarSensores();

    Serial.println("Posicione o sensor com X alinhado com a gravidade.");
    Serial.println("Mantenha completamente parado.");
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

    Serial.println(
        "timestamp_ms,"
        "ax,ay,az,"
        "qw,qx,qy,qz,"
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


    // -------------------------
    // GIROSCOPIO CORRIGIDO
    // -------------------------

    float gxCorrigido =
        dado.gx - biasGx;

    float gyCorrigido =
        dado.gy - biasGy;

    float gzCorrigido =
        dado.gz - biasGz;


    // -------------------------
    // MADGWICK
    // -------------------------

    atualizarOrientacao(
        dado,
        dt,
        biasGx,
        biasGy,
        biasGz
    );


    // -------------------------
    // QUATERNION
    // -------------------------

    float qw;
    float qx;
    float qy;
    float qz;

    filtroAccel.getQuaternion(
        &qw,
        &qx,
        &qy,
        &qz
    );


    // -------------------------
    // VETOR DE GRAVIDADE
    // -------------------------

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


    // -------------------------
    // CSV
    // -------------------------

    Serial.print(dado.timestamp_ms);
    Serial.print(",");

    Serial.print(dado.ax, 4);
    Serial.print(",");
    Serial.print(dado.ay, 4);
    Serial.print(",");
    Serial.print(dado.az, 4);
    Serial.print(",");

    Serial.print(qw, 6);
    Serial.print(",");
    Serial.print(qx, 6);
    Serial.print(",");
    Serial.print(qy, 6);
    Serial.print(",");
    Serial.print(qz, 6);
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