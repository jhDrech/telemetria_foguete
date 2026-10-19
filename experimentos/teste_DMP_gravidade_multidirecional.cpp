#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

// Acelerômetro configurado pelo DMP em ±4 g
constexpr float ACC_LSB_POR_G = 8192.0f;

// Aceleração gravitacional padrão
constexpr float GRAVIDADE = 9.80665f;

void setup()
{
    Serial.begin(115200);
    delay(8000);

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

    // Inicializa o DMP
    sucesso &= (
        icm.initializeDMP()
        == ICM_20948_Stat_Ok
    );

    // Quaternion de 6 eixos:
    // acelerômetro + giroscópio
    sucesso &= (
        icm.enableDMPSensor(
            INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR
        )
        == ICM_20948_Stat_Ok
    );

    // Faz o DMP enviar Accel + Quat6
    sucesso &= (
        icm.enableDMPSensor(
            INV_ICM20948_SENSOR_LINEAR_ACCELERATION
        )
        == ICM_20948_Stat_Ok
    );

    // Maior taxa do Quat6 configurada pelo DMP
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

    // CSV para Serial Studio
    Serial.println(
        "timestamp_ms,"
        "ax_mps2,ay_mps2,az_mps2,"
        "gx_mps2,gy_mps2,gz_mps2,"
        "ax_linear_mps2,ay_linear_mps2,az_linear_mps2,"
        "q0,q1,q2,q3"
    );
}

void loop()
{
    icm_20948_DMP_data_t dado;

    icm.readDMPdataFromFIFO(&dado);

    if (
        icm.status != ICM_20948_Stat_Ok &&
        icm.status != ICM_20948_Stat_FIFOMoreDataAvail
    )
    {
        return;
    }

    bool temQuat6 =
        (dado.header & DMP_header_bitmap_Quat6) > 0;

    bool temAccel =
        (dado.header & DMP_header_bitmap_Accel) > 0;

    // Só usamos frames onde Accel e Quat6
    // chegaram juntos.
    if (!temQuat6 || !temAccel)
    {
        return;
    }

    // =========================================================
    // 1. QUATERNION
    // =========================================================

    double q1 =
        (double)dado.Quat6.Data.Q1 / 1073741824.0;

    double q2 =
        (double)dado.Quat6.Data.Q2 / 1073741824.0;

    double q3 =
        (double)dado.Quat6.Data.Q3 / 1073741824.0;

    double termoQ0 =
        1.0
        - q1 * q1
        - q2 * q2
        - q3 * q3;

    if (termoQ0 < 0.0)
    {
        termoQ0 = 0.0;
    }

    double q0 = sqrt(termoQ0);

    // Normalização adicional por segurança
    double normaQ =
        sqrt(
            q0 * q0 +
            q1 * q1 +
            q2 * q2 +
            q3 * q3
        );

    if (normaQ > 0.0)
    {
        q0 /= normaQ;
        q1 /= normaQ;
        q2 /= normaQ;
        q3 /= normaQ;
    }

    // =========================================================
    // 2. ACELERAÇÃO BRUTA -> m/s²
    // =========================================================

    float ax =
        ((float)dado.Raw_Accel.Data.X / ACC_LSB_POR_G)
        * GRAVIDADE;

    float ay =
        ((float)dado.Raw_Accel.Data.Y / ACC_LSB_POR_G)
        * GRAVIDADE;

    float az =
        ((float)dado.Raw_Accel.Data.Z / ACC_LSB_POR_G)
        * GRAVIDADE;

    // =========================================================
    // 3. DIREÇÃO DA GRAVIDADE A PARTIR DO QUATERNION
    //
    // Resultado primeiro em unidades de "g":
    // exemplo:
    //
    // gxUnit = 0
    // gyUnit = 0
    // gzUnit = 1
    //
    // significa gravidade totalmente no eixo Z.
    // =========================================================

    double gxUnit =
        2.0 * (
            q1 * q3 -
            q0 * q2
        );

    double gyUnit =
        2.0 * (
            q0 * q1 +
            q2 * q3
        );

    double gzUnit =
        q0 * q0
        - q1 * q1
        - q2 * q2
        + q3 * q3;

    // Converte vetor unitário da gravidade
    // para m/s²
    float gx =
        (float)(gxUnit * GRAVIDADE);

    float gy =
        (float)(gyUnit * GRAVIDADE);

    float gz =
        (float)(gzUnit * GRAVIDADE);

    // =========================================================
    // 4. REMOVE A GRAVIDADE
    // =========================================================

    float axLinear = ax - gx;
    float ayLinear = ay - gy;
    float azLinear = az - gz;

    // =========================================================
    // 5. CSV
    // =========================================================

    Serial.print(millis());
    Serial.print(",");

    Serial.print(ax, 5);
    Serial.print(",");

    Serial.print(ay, 5);
    Serial.print(",");

    Serial.print(az, 5);
    Serial.print(",");

    Serial.print(gx, 5);
    Serial.print(",");

    Serial.print(gy, 5);
    Serial.print(",");

    Serial.print(gz, 5);
    Serial.print(",");

    Serial.print(axLinear, 5);
    Serial.print(",");

    Serial.print(ayLinear, 5);
    Serial.print(",");

    Serial.print(azLinear, 5);
    Serial.print(",");

    Serial.print(q0, 6);
    Serial.print(",");

    Serial.print(q1, 6);
    Serial.print(",");

    Serial.print(q2, 6);
    Serial.print(",");

    Serial.println(q3, 6);
}