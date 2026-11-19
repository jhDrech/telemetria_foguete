#include <Arduino.h>
#include <Wire.h>
#include "sensores.h"


void setup() {
    Serial.begin(115200);
    if(!iniciarSensores()){
        Serial.println("Sensores não iniciados.");
        while (true)
        {
            delay(1000);
        }
        
    }
    //CABEÇALHO CSV
    Serial.println("ax,ay,az,gx,gy,gz,mx,my,mz,pressao,temp,timestamp_ms");
}

void loop() {
    DadoSensor dado{}; // "{}" inicializa o dado com campos em zero, caso algum não seja preenchido
    if(!lerSensores(dado)){
        return; //encerra o loop atual, depois chama denovo
    }

    Serial.print(dado.ax);
    Serial.print(",");
    Serial.print(dado.ay);
    Serial.print(",");
    Serial.print(dado.az);
    Serial.print(",");

    Serial.print(dado.gx);
    Serial.print(",");
    Serial.print(dado.gy);
    Serial.print(",");
    Serial.print(dado.gz);
    Serial.print(",");

    Serial.print(dado.mx);
    Serial.print(",");
    Serial.print(dado.my);
    Serial.print(",");
    Serial.print(dado.mz);
    Serial.print(",");

    Serial.print(dado.pressao);
    Serial.print(",");
    Serial.print(dado.temp);
    Serial.print(",");
    Serial.println(dado.timestamp_ms);
}