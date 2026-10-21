#include <Arduino.h>
#include "sensores.h"

void setup() {
    Serial.begin(115200);
    delay(8000);

    bool ok = iniciarSensores();

    if (!ok) {
        while (true) {
            delay(1000);
        }
    }

    Serial.println(
        "timestamp_ms,"
        "pressao_pa,temperatura_c,altitude_m,"
        "ax,ay,az,"
        "gx,gy,gz,"
        "mx,my,mz,"
        "q0,q1,q2,q3,"
        "quaternion_valido"
    );
}

void loop() {
    DadoSensor d = lerSensores();

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

    Serial.println(d.quaternion_valido ? 1 : 0);

    delay(20);
}