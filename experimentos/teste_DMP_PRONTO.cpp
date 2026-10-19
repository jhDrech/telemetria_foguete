#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

// ============================================================
// CRITÉRIOS DE CONVERGÊNCIA
// ============================================================

// Antes disso, o sistema nunca considera o DMP pronto.
constexpr uint32_t TEMPO_MINIMO_MS = 30000;

// Janela usada para verificar estabilidade.
constexpr uint32_t JANELA_ESTABILIDADE_MS = 5000;

// Maior variação permitida de roll e pitch
// dentro da janela.
constexpr double LIMITE_VARIACAO_GRAUS = 0.10;

// ============================================================
// ESTADO DA CONVERGÊNCIA
// ============================================================

uint32_t inicioSistemaMs = 0;
uint32_t inicioJanelaMs = 0;

bool janelaAtiva = false;
bool dmpPronto = false;

double rollMin = 0.0;
double rollMax = 0.0;

double pitchMin = 0.0;
double pitchMax = 0.0;

// ============================================================
// INICIALIZAÇÃO
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

    inicioSistemaMs = millis();

    // CSV para Serial Studio
    Serial.println(
        "timestamp_ms,"
        "roll_deg,pitch_deg,"
        "variacao_roll_deg,variacao_pitch_deg,"
        "dmp_pronto"
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

    if (
        (dado.header & DMP_header_bitmap_Quat6)
        == 0
    )
    {
        return;
    }

    // ========================================================
    // QUATERNION
    // ========================================================

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

    // ========================================================
    // QUATERNION -> ROLL / PITCH
    // ========================================================

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

    uint32_t agora = millis();
    uint32_t tempoLigado =
        agora - inicioSistemaMs;

    // ========================================================
    // DETECÇÃO DE ESTABILIDADE
    // ========================================================

    double variacaoRoll = 0.0;
    double variacaoPitch = 0.0;

    if (!dmpPronto &&
        tempoLigado >= TEMPO_MINIMO_MS)
    {
        // Inicia nova janela
        if (!janelaAtiva)
        {
            janelaAtiva = true;
            inicioJanelaMs = agora;

            rollMin = rollDeg;
            rollMax = rollDeg;

            pitchMin = pitchDeg;
            pitchMax = pitchDeg;
        }

        // Atualiza mínimos e máximos
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

        variacaoRoll =
            rollMax - rollMin;

        variacaoPitch =
            pitchMax - pitchMin;

        // Terminou uma janela de 5 segundos
        if (
            agora - inicioJanelaMs
            >= JANELA_ESTABILIDADE_MS
        )
        {
            bool rollEstavel =
                variacaoRoll
                <= LIMITE_VARIACAO_GRAUS;

            bool pitchEstavel =
                variacaoPitch
                <= LIMITE_VARIACAO_GRAUS;

            if (
                rollEstavel &&
                pitchEstavel
            )
            {
                dmpPronto = true;
            }
            else
            {
                // Falhou:
                // começa uma nova janela
                janelaAtiva = false;
            }
        }
    }

    // ========================================================
    // CSV
    // ========================================================

    Serial.print(agora);
    Serial.print(",");

    Serial.print(rollDeg, 5);
    Serial.print(",");

    Serial.print(pitchDeg, 5);
    Serial.print(",");

    Serial.print(variacaoRoll, 5);
    Serial.print(",");

    Serial.print(variacaoPitch, 5);
    Serial.print(",");

    Serial.println(
        dmpPronto ? 1 : 0
    );
}