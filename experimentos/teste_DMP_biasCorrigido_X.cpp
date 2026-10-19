#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

// ============================================================
// CONSTANTES
// ============================================================

constexpr float GRAVIDADE = 9.80665f;
constexpr float ACC_LSB_POR_G = 8192.0f;

// Apenas o bias de X será corrigido
constexpr float BIAS_AX = -0.146f;

// ============================================================
// CONVERGÊNCIA DO DMP
// ============================================================

constexpr uint32_t TEMPO_MINIMO_DMP_MS = 30000;
constexpr uint32_t JANELA_ESTABILIDADE_MS = 5000;
constexpr double LIMITE_VARIACAO_GRAUS = 0.10;

uint32_t inicioSistemaMs = 0;
uint32_t inicioJanelaMs = 0;

bool janelaAtiva = false;
bool dmpPronto = false;

double rollMin = 0.0;
double rollMax = 0.0;

double pitchMin = 0.0;
double pitchMax = 0.0;

// ============================================================
// ÚLTIMO QUATERNION VÁLIDO
// ============================================================

double q0Atual = 1.0;
double q1Atual = 0.0;
double q2Atual = 0.0;
double q3Atual = 0.0;

bool quaternionValido = false;

// ============================================================
// ÚLTIMA LEITURA DO ACELERÔMETRO
// ============================================================

float axRaw = 0.0f;
float ayRaw = 0.0f;
float azRaw = 0.0f;

bool accelValido = false;

// ============================================================
// SETUP
// ============================================================

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
            INV_ICM20948_SENSOR_RAW_ACCELEROMETER
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
        icm.setDMPODRrate(
            DMP_ODR_Reg_Accel,
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

    inicioSistemaMs = millis();

    Serial.println(
        "timestamp_ms,"
        "dmp_pronto,"
        "ax_raw,ay_raw,az_raw,"
        "ax_corr,ay_corr,az_corr,"
        "ax_linear_raw,ay_linear_raw,az_linear_raw,"
        "modulo_linear_raw,"
        "ax_linear_corr,ay_linear_corr,az_linear_corr,"
        "modulo_linear_corr"
    );
}

// ============================================================
// LOOP
// ============================================================

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

    uint32_t agora = millis();

    bool temQuat6 =
        (dado.header & DMP_header_bitmap_Quat6) != 0;

    bool temAccel =
        (dado.header & DMP_header_bitmap_Accel) != 0;

    // ========================================================
    // QUATERNION
    // ========================================================

    if (temQuat6)
    {
        double q1 =
            (double)dado.Quat6.Data.Q1
            / 1073741824.0;

        double q2 =
            (double)dado.Quat6.Data.Q2
            / 1073741824.0;

        double q3 =
            (double)dado.Quat6.Data.Q3
            / 1073741824.0;

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

        q0Atual = q0;
        q1Atual = q1;
        q2Atual = q2;
        q3Atual = q3;

        quaternionValido = true;

        // ====================================================
        // ROLL
        // ====================================================

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

        // ====================================================
        // PITCH
        // ====================================================

        double sinPitch =
            2.0 * (
                q0 * q2 -
                q3 * q1
            );

        if (sinPitch > 1.0)
        {
            sinPitch = 1.0;
        }

        if (sinPitch < -1.0)
        {
            sinPitch = -1.0;
        }

        double pitch = asin(sinPitch);

        double rollDeg =
            roll * 180.0 / PI;

        double pitchDeg =
            pitch * 180.0 / PI;

        // ====================================================
        // CONVERGÊNCIA DO DMP
        // ====================================================

        if (!dmpPronto)
        {
            uint32_t tempoLigado =
                agora - inicioSistemaMs;

            if (tempoLigado >= TEMPO_MINIMO_DMP_MS)
            {
                if (!janelaAtiva)
                {
                    janelaAtiva = true;
                    inicioJanelaMs = agora;

                    rollMin = rollMax = rollDeg;
                    pitchMin = pitchMax = pitchDeg;
                }

                if (rollDeg < rollMin)
                {
                    rollMin = rollDeg;
                }

                if (rollDeg > rollMax)
                {
                    rollMax = rollDeg;
                }

                if (pitchDeg < pitchMin)
                {
                    pitchMin = pitchDeg;
                }

                if (pitchDeg > pitchMax)
                {
                    pitchMax = pitchDeg;
                }

                double variacaoRoll =
                    rollMax - rollMin;

                double variacaoPitch =
                    pitchMax - pitchMin;

                if (
                    agora - inicioJanelaMs
                    >= JANELA_ESTABILIDADE_MS
                )
                {
                    bool rollEstavel =
                        variacaoRoll <= LIMITE_VARIACAO_GRAUS;

                    bool pitchEstavel =
                        variacaoPitch <= LIMITE_VARIACAO_GRAUS;

                    if (
                        rollEstavel &&
                        pitchEstavel
                    )
                    {
                        dmpPronto = true;
                    }
                    else
                    {
                        janelaAtiva = false;
                    }
                }
            }
        }
    }

    // ========================================================
    // ACELERÔMETRO
    // ========================================================

    if (temAccel)
    {
        axRaw =
            ((float)dado.Raw_Accel.Data.X
             / ACC_LSB_POR_G)
            * GRAVIDADE;

        ayRaw =
            ((float)dado.Raw_Accel.Data.Y
             / ACC_LSB_POR_G)
            * GRAVIDADE;

        azRaw =
            ((float)dado.Raw_Accel.Data.Z
             / ACC_LSB_POR_G)
            * GRAVIDADE;

        accelValido = true;
    }

    if (!quaternionValido || !accelValido)
    {
        return;
    }

    // ========================================================
    // CORREÇÃO DE BIAS
    //
    // Apenas X é corrigido.
    // ========================================================

    float axCorr =
        axRaw - BIAS_AX;

    float ayCorr =
        ayRaw;

    float azCorr =
        azRaw;

    // ========================================================
    // VETOR GRAVIDADE
    // ========================================================

    double gxUnit =
        2.0 * (
            q1Atual * q3Atual -
            q0Atual * q2Atual
        );

    double gyUnit =
        2.0 * (
            q0Atual * q1Atual +
            q2Atual * q3Atual
        );

    double gzUnit =
        q0Atual * q0Atual
        - q1Atual * q1Atual
        - q2Atual * q2Atual
        + q3Atual * q3Atual;

    float gx =
        (float)(gxUnit * GRAVIDADE);

    float gy =
        (float)(gyUnit * GRAVIDADE);

    float gz =
        (float)(gzUnit * GRAVIDADE);

    // ========================================================
    // SEM CORREÇÃO
    // ========================================================

    float axLinearRaw =
        axRaw - gx;

    float ayLinearRaw =
        ayRaw - gy;

    float azLinearRaw =
        azRaw - gz;

    float moduloLinearRaw =
        sqrtf(
            axLinearRaw * axLinearRaw +
            ayLinearRaw * ayLinearRaw +
            azLinearRaw * azLinearRaw
        );

    // ========================================================
    // COM CORREÇÃO SOMENTE EM X
    // ========================================================

    float axLinearCorr =
        axCorr - gx;

    float ayLinearCorr =
        ayCorr - gy;

    float azLinearCorr =
        azCorr - gz;

    float moduloLinearCorr =
        sqrtf(
            axLinearCorr * axLinearCorr +
            ayLinearCorr * ayLinearCorr +
            azLinearCorr * azLinearCorr
        );

    // ========================================================
    // CSV
    // ========================================================

    Serial.print(agora);
    Serial.print(",");

    Serial.print(dmpPronto ? 1 : 0);
    Serial.print(",");

    Serial.print(axRaw, 6);
    Serial.print(",");

    Serial.print(ayRaw, 6);
    Serial.print(",");

    Serial.print(azRaw, 6);
    Serial.print(",");

    Serial.print(axCorr, 6);
    Serial.print(",");

    Serial.print(ayCorr, 6);
    Serial.print(",");

    Serial.print(azCorr, 6);
    Serial.print(",");

    Serial.print(axLinearRaw, 6);
    Serial.print(",");

    Serial.print(ayLinearRaw, 6);
    Serial.print(",");

    Serial.print(azLinearRaw, 6);
    Serial.print(",");

    Serial.print(moduloLinearRaw, 6);
    Serial.print(",");

    Serial.print(axLinearCorr, 6);
    Serial.print(",");

    Serial.print(ayLinearCorr, 6);
    Serial.print(",");

    Serial.print(azLinearCorr, 6);
    Serial.print(",");

    Serial.println(moduloLinearCorr, 6);
}