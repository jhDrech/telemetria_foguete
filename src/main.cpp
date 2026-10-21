#include <Arduino.h>
#include "sensores.h"
#include "orientacao.h"

void setup() {

    Serial.begin(115200);

    delay(8000);

    if (!iniciarSensores()) {

        while (true) {
            delay(1000);
        }
    }

    // Comeca a contar o tempo de convergencia do DMP
    // depois que os sensores foram inicializados.
    iniciarOrientacao();

    Serial.println(
        "timestamp_ms,"
        "pressao_pa,temperatura_c,altitude_m,"
        "ax,ay,az,"
        "gx,gy,gz,"
        "mx,my,mz,"
        "q0,q1,q2,q3,"
        "norma_q,"
        "quaternion_valido,"
        "roll_deg,pitch_deg,"
        "variacao_roll_deg,variacao_pitch_deg,"
        "janela_estabilidade,"
        "dmp_pronto"
    );
}

void loop() {

    // ------------------------------------------------------------------------
    // AQUISICAO
    // ------------------------------------------------------------------------

    DadoSensor d =
        lerSensores();

    // ------------------------------------------------------------------------
    // ORIENTACAO / CONVERGENCIA
    // ------------------------------------------------------------------------

    EstadoDMP estadoDMP =
        atualizarEstadoDMP(d);

    // ------------------------------------------------------------------------
    // CSV
    // ------------------------------------------------------------------------

    Serial.print(millis());
    Serial.print(",");

    Serial.print(d.pressao_pa, 2);
    Serial.print(",");

    Serial.print(d.temperatura_c, 2);
    Serial.print(",");

    Serial.print(d.altitude_m, 3);
    Serial.print(",");

    Serial.print(d.ax, 5);
    Serial.print(",");

    Serial.print(d.ay, 5);
    Serial.print(",");

    Serial.print(d.az, 5);
    Serial.print(",");

    Serial.print(d.gx, 6);
    Serial.print(",");

    Serial.print(d.gy, 6);
    Serial.print(",");

    Serial.print(d.gz, 6);
    Serial.print(",");

    Serial.print(d.mx, 3);
    Serial.print(",");

    Serial.print(d.my, 3);
    Serial.print(",");

    Serial.print(d.mz, 3);
    Serial.print(",");

    Serial.print(d.q0, 7);
    Serial.print(",");

    Serial.print(d.q1, 7);
    Serial.print(",");

    Serial.print(d.q2, 7);
    Serial.print(",");

    Serial.print(d.q3, 7);
    Serial.print(",");

    Serial.print(estadoDMP.norma_q, 7);
    Serial.print(",");

    Serial.print(
        d.quaternion_valido ? 1 : 0
    );
    Serial.print(",");

    Serial.print(
        estadoDMP.roll_deg,
        4
    );
    Serial.print(",");

    Serial.print(
        estadoDMP.pitch_deg,
        4
    );
    Serial.print(",");

    Serial.print(
        estadoDMP.variacao_roll_deg,
        4
    );
    Serial.print(",");

    Serial.print(
        estadoDMP.variacao_pitch_deg,
        4
    );
    Serial.print(",");

    Serial.print(
        estadoDMP.janela_estabilidade ? 1 : 0
    );
    Serial.print(",");

    Serial.println(
        estadoDMP.dmp_pronto ? 1 : 0
    );

    delay(20);
}