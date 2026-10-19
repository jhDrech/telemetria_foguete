#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

void setup() {
    Serial.begin(115200);
    delay(8000);

    Serial.println();
    Serial.println("=== TESTE DMP - ESTABILIDADE QUAT6 ===");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    icm.begin(Wire, 0);

    if (icm.status != ICM_20948_Stat_Ok) {
        Serial.print("Erro ao iniciar ICM20948: ");
        Serial.println(icm.statusString());
        return;
    }

    Serial.println("ICM20948... OK");

    bool sucesso = true;

    sucesso &= (
        icm.initializeDMP()
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.enableDMPSensor(
            INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR
        )
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.setDMPODRrate(
            DMP_ODR_Reg_Quat6,
            0
        )
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.enableFIFO()
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.enableDMP()
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.resetDMP()
        == ICM_20948_Stat_Ok
    );

    sucesso &= (
        icm.resetFIFO()
        == ICM_20948_Stat_Ok
    );

    if (!sucesso) {
        Serial.println("Erro ao configurar DMP.");
        return;
    }

    Serial.println("DMP... OK");
    Serial.println("Mantenha o sensor completamente parado.");
    Serial.println();

    Serial.println(
        "tempo_ms,roll_deg,pitch_deg,q0,q1,q2,q3"
    );
}

void loop() {
    icm_20948_DMP_data_t dado;

    icm.readDMPdataFromFIFO(&dado);

    if (
        icm.status == ICM_20948_Stat_Ok ||
        icm.status == ICM_20948_Stat_FIFOMoreDataAvail
    ) {

        if (
            (dado.header & DMP_header_bitmap_Quat6)
            > 0
        ) {

            double q1 =
                (double)dado.Quat6.Data.Q1
                / 1073741824.0;

            double q2 =
                (double)dado.Quat6.Data.Q2
                / 1073741824.0;

            double q3 =
                (double)dado.Quat6.Data.Q3
                / 1073741824.0;

            double termo =
                1.0
                - q1 * q1
                - q2 * q2
                - q3 * q3;

            if (termo < 0.0) {
                termo = 0.0;
            }

            double q0 = sqrt(termo);

            // Quaternion -> roll
            double roll =
                atan2(
                    2.0 * (
                        q0 * q1 +
                        q2 * q3
                    ),
                    1.0 - 2.0 * (
                        q1 * q1 +
                        q2 * q2
                    )
                );

            // Quaternion -> pitch
            double sinPitch =
                2.0 * (
                    q0 * q2 -
                    q3 * q1
                );

            if (sinPitch > 1.0) {
                sinPitch = 1.0;
            }

            if (sinPitch < -1.0) {
                sinPitch = -1.0;
            }

            double pitch = asin(sinPitch);

            double rollDeg =
                roll * 180.0 / PI;

            double pitchDeg =
                pitch * 180.0 / PI;

            Serial.print(millis());
            Serial.print(",");

            Serial.print(rollDeg, 4);
            Serial.print(",");

            Serial.print(pitchDeg, 4);
            Serial.print(",");

            Serial.print(q0, 6);
            Serial.print(",");

            Serial.print(q1, 6);
            Serial.print(",");

            Serial.print(q2, 6);
            Serial.print(",");

            Serial.println(q3, 6);
        }
    }
}