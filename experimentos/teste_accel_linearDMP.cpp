#include <Arduino.h>
#include <Wire.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    icm.begin(Wire, 0);

    if (icm.status != ICM_20948_Stat_Ok)
    {
        while (true)
        {
            delay(1000);
        }
    }

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
        icm.enableDMPSensor(
            INV_ICM20948_SENSOR_LINEAR_ACCELERATION
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

    if (!sucesso)
    {
        while (true)
        {
            delay(1000);
        }
    }

    // Cabeçalho CSV para o Serial Studio
    Serial.println(
        "timestamp_ms,header,tem_quat6,tem_accel,ax_raw,ay_raw,az_raw"
    );
}

void loop()
{
    icm_20948_DMP_data_t dado;

    icm.readDMPdataFromFIFO(&dado);

    if (
        icm.status == ICM_20948_Stat_Ok ||
        icm.status == ICM_20948_Stat_FIFOMoreDataAvail
    )
    {
        bool temQuat6 =
            (dado.header & DMP_header_bitmap_Quat6) > 0;

        bool temAccel =
            (dado.header & DMP_header_bitmap_Accel) > 0;

        Serial.print(millis());
        Serial.print(",");

        // Header em decimal para facilitar leitura no CSV/Serial Studio
        Serial.print(dado.header);
        Serial.print(",");

        Serial.print(temQuat6 ? 1 : 0);
        Serial.print(",");

        Serial.print(temAccel ? 1 : 0);
        Serial.print(",");

        if (temAccel)
        {
            Serial.print(dado.Raw_Accel.Data.X);
            Serial.print(",");

            Serial.print(dado.Raw_Accel.Data.Y);
            Serial.print(",");

            Serial.println(dado.Raw_Accel.Data.Z);
        }
        else
        {
            Serial.println("nan,nan,nan");
        }
    }
}