#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

// ============================================================
// CONSTANTES DO ACELERÔMETRO
// ============================================================

constexpr float ACC_LSB_POR_G = 8192.0f;
constexpr float GRAVIDADE = 9.80665f;

// ============================================================
// CRITÉRIO DE CONVERGÊNCIA DO DMP
// ============================================================

constexpr uint32_t TEMPO_MINIMO_DMP_MS = 30000;
constexpr uint32_t JANELA_ESTABILIDADE_MS = 5000;

constexpr double LIMITE_VARIACAO_GRAUS = 0.10;

// ============================================================
// EXPERIMENTO DE BIAS
// ============================================================

constexpr int NUM_POSICOES = 6;
constexpr int NUM_REPETICOES = 3;

// Tempo para reposicionar a placa.
constexpr uint32_t TEMPO_POSICIONAMENTO_MS = 8000;

// Tempo de aquisição parada em cada posição.
constexpr uint32_t TEMPO_COLETA_MS = 10000;

// ============================================================
// ESTADOS
//
// 0 = aguardando convergência do DMP
// 1 = posicionando
// 2 = coletando
// 3 = experimento finalizado
// ============================================================

int estado = 0;

int posicao = 0;
int repeticao = 0;

bool dmpPronto = false;

// ============================================================
// VARIÁVEIS PARA CONVERGÊNCIA
// ============================================================

uint32_t inicioSistemaMs = 0;
uint32_t inicioJanelaMs = 0;

bool janelaAtiva = false;

double rollMin = 0.0;
double rollMax = 0.0;

double pitchMin = 0.0;
double pitchMax = 0.0;

double variacaoRoll = 0.0;
double variacaoPitch = 0.0;

// ============================================================
// CONTROLE DAS ETAPAS
// ============================================================

uint32_t inicioEtapaMs = 0;

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

    inicioSistemaMs = millis();

    // CSV puro para Serial Studio
    Serial.println(
        "timestamp_ms,"
        "estado,repeticao,posicao,coletando,dmp_pronto,"
        "roll_deg,pitch_deg,"
        "variacao_roll_deg,variacao_pitch_deg,"
        "ax_mps2,ay_mps2,az_mps2,"
        "modulo_accel_mps2"
    );
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    if (estado == 3)
    {
        return;
    }

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
        (dado.header & DMP_header_bitmap_Quat6) != 0;

    bool temAccel =
        (dado.header & DMP_header_bitmap_Accel) != 0;

    uint32_t agora = millis();

    static double rollDeg = 0.0;
    static double pitchDeg = 0.0;

    // ========================================================
    // QUATERNION -> ROLL / PITCH
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

        double pitch =
            asin(sinPitch);

        rollDeg =
            roll * 180.0 / PI;

        pitchDeg =
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

                    rollMin = rollDeg;
                    rollMax = rollDeg;

                    pitchMin = pitchDeg;
                    pitchMax = pitchDeg;
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

                variacaoRoll =
                    rollMax - rollMin;

                variacaoPitch =
                    pitchMax - pitchMin;

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

                        repeticao = 1;
                        posicao = 1;

                        estado = 1;

                        inicioEtapaMs = agora;
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
    // CONTROLE DO EXPERIMENTO
    // ========================================================

    if (dmpPronto)
    {
        uint32_t tempoEtapa =
            agora - inicioEtapaMs;

        // ----------------------------------------------------
        // ESTADO 1
        // Posicionar a placa
        // ----------------------------------------------------

        if (estado == 1)
        {
            if (
                tempoEtapa
                >= TEMPO_POSICIONAMENTO_MS
            )
            {
                estado = 2;

                inicioEtapaMs = agora;
            }
        }

        // ----------------------------------------------------
        // ESTADO 2
        // Coleta parada
        // ----------------------------------------------------

        else if (estado == 2)
        {
            if (
                tempoEtapa
                >= TEMPO_COLETA_MS
            )
            {
                posicao++;

                // Terminou as seis posições
                if (posicao > NUM_POSICOES)
                {
                    repeticao++;

                    // Terminou todas as repetições
                    if (
                        repeticao
                        > NUM_REPETICOES
                    )
                    {
                        estado = 3;

                        return;
                    }

                    // Nova repetição começa novamente em +X
                    posicao = 1;
                }

                estado = 1;

                inicioEtapaMs = agora;
            }
        }
    }

    // ========================================================
    // SAÍDA APENAS QUANDO HÁ AMOSTRA DO ACELERÔMETRO
    // ========================================================

    if (!temAccel)
    {
        return;
    }

    // ========================================================
    // CONVERSÃO PARA m/s²
    // ========================================================

    float ax =
        ((float)dado.Raw_Accel.Data.X
         / ACC_LSB_POR_G)
        * GRAVIDADE;

    float ay =
        ((float)dado.Raw_Accel.Data.Y
         / ACC_LSB_POR_G)
        * GRAVIDADE;

    float az =
        ((float)dado.Raw_Accel.Data.Z
         / ACC_LSB_POR_G)
        * GRAVIDADE;

    float moduloAccel =
        sqrtf(
            ax * ax +
            ay * ay +
            az * az
        );

    bool coletando =
        estado == 2;

    // ========================================================
    // CSV
    // ========================================================

    Serial.print(agora);
    Serial.print(",");

    Serial.print(estado);
    Serial.print(",");

    Serial.print(repeticao);
    Serial.print(",");

    Serial.print(posicao);
    Serial.print(",");

    Serial.print(coletando ? 1 : 0);
    Serial.print(",");

    Serial.print(dmpPronto ? 1 : 0);
    Serial.print(",");

    Serial.print(rollDeg, 5);
    Serial.print(",");

    Serial.print(pitchDeg, 5);
    Serial.print(",");

    Serial.print(variacaoRoll, 5);
    Serial.print(",");

    Serial.print(variacaoPitch, 5);
    Serial.print(",");

    Serial.print(ax, 6);
    Serial.print(",");

    Serial.print(ay, 6);
    Serial.print(",");

    Serial.print(az, 6);
    Serial.print(",");

    Serial.println(moduloAccel, 6);
}