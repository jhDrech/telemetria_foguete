#include <Arduino.h>
#include <Wire.h>
#include "ICM_20948.h"

#define PIN_SDA 8
#define PIN_SCL 9

ICM_20948_I2C icm;

// O DMP configura o acelerometro em ±4 g
constexpr float ACC_LSB_POR_G = 8192.0f;
constexpr float GRAVIDADE = 9.80665f;

// Tempo disponível para colocar o sensor
// na próxima orientação
constexpr uint32_t TEMPO_POSICIONAMENTO_MS = 8000;

// Tempo de coleta parado em cada posição
constexpr uint32_t TEMPO_COLETA_MS = 5000;

// Seis orientações
constexpr int NUM_POSICOES = 6;

int posicaoAtual = 1;

bool coletando = false;

uint32_t inicioEtapa = 0;

bool experimentoFinalizado = false;

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

    // Precisamos apenas do acelerometro neste experimento.
    // LINEAR_ACCELERATION faz o DMP fornecer
    // Raw_Accel junto com Quat6.
    sucesso &= (
        icm.enableDMPSensor(
            INV_ICM20948_SENSOR_LINEAR_ACCELERATION
        )
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

    // CSV puro para Serial Studio
    Serial.println(
        "timestamp_ms,posicao,coletando,"
        "ax_mps2,ay_mps2,az_mps2"
    );

    inicioEtapa = millis();
}

void loop()
{
    if (experimentoFinalizado)
    {
        return;
    }

    uint32_t agora = millis();
    uint32_t tempoEtapa = agora - inicioEtapa;

    // =========================================================
    // CONTROLE DAS ETAPAS
    // =========================================================

    if (!coletando)
    {
        // Fase em que o usuário reposiciona o sensor
        if (tempoEtapa >= TEMPO_POSICIONAMENTO_MS)
        {
            coletando = true;
            inicioEtapa = agora;
        }
    }
    else
    {
        // Fase de coleta parada
        if (tempoEtapa >= TEMPO_COLETA_MS)
        {
            coletando = false;

            posicaoAtual++;

            inicioEtapa = agora;

            if (posicaoAtual > NUM_POSICOES)
            {
                experimentoFinalizado = true;
                return;
            }
        }
    }

    // =========================================================
    // LEITURA DO DMP
    // =========================================================

    icm_20948_DMP_data_t dado;

    icm.readDMPdataFromFIFO(&dado);

    if (
        icm.status != ICM_20948_Stat_Ok &&
        icm.status != ICM_20948_Stat_FIFOMoreDataAvail
    )
    {
        return;
    }

    bool temAccel =
        (dado.header & DMP_header_bitmap_Accel) > 0;

    if (!temAccel)
    {
        return;
    }

    // =========================================================
    // RAW -> m/s²
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
    // CSV
    // =========================================================

    Serial.print(agora);
    Serial.print(",");

    Serial.print(posicaoAtual);
    Serial.print(",");

    Serial.print(coletando ? 1 : 0);
    Serial.print(",");

    Serial.print(ax, 6);
    Serial.print(",");

    Serial.print(ay, 6);
    Serial.print(",");

    Serial.println(az, 6);
}