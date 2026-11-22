#include <Arduino.h>
#include "sensores.h"

DadoSensor dado;

void setup() {
    Serial.begin(115200);
    delay(5000);

    if(!iniciarSensores()){
        Serial.println("Erro ao iniciar sensores");
        while(true){
            delay(1000);
        }
    }

    Serial.println(
        "timestamp_ms,ax,ay,az,axLinear,ayLinear,azLinear"
    );
}

void loop() {

    if(lerSensores(dado)) {
        float axLinear = NAN;
        float ayLinear = NAN;
        float azLinear = NAN;

        //se os dados do icm não forem nulos
        if (
            !isnan(dado.ax) &&
            !isnan(dado.ay) &&
            !isnan(dado.az) &&
            !isnan(dado.gx) &&
            !isnan(dado.gy) &&
            !isnan(dado.gz)
        ){
            // Atualiza a orientação estimada pelo Madgwick
            atualizarOrientacao(dado);
            
            // Remove a gravidade
            obterAceleracaoLinear(
                dado,
                axLinear,
                ayLinear,
                azLinear
            );
        }

        Serial.print(dado.timestamp_ms);
        Serial.print(",");

        Serial.print(dado.ax, 4);
        Serial.print(",");
        Serial.print(dado.ay, 4);
        Serial.print(",");
        Serial.print(dado.az, 4);
        Serial.print(",");

        Serial.print(axLinear, 4);
        Serial.print(",");
        Serial.print(ayLinear, 4);
        Serial.print(",");
        Serial.println(azLinear, 4);
    }

    delay(10);
}