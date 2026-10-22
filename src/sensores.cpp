#include "sensores.h"

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include <Adafruit_Sensor.h>

namespace {

constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;

// VARIAVEIS PARA ACELEROMETRO

static constexpr float GRAVIDADE = 9.80665f;
static constexpr float ACC_LSB_POR_G = 8192.0f;
static constexpr float DEG_PARA_RAD = PI / 180.0f;

Adafruit_BMP280 bmp;
ICM_20948_I2C icm;

bool bmpDisponivel = false;
bool icmDisponivel = false;

static float axAtual = 0.0f;
static float ayAtual = 0.0f;
static float azAtual = 0.0f;

static bool aceleracaoValida = false;

// QUATERNION

static float q0Atual = 1.0f;
static float q1Atual = 0.0f;
static float q2Atual = 0.0f;
static float q3Atual = 0.0f;

static bool quaternionValido = false;

//INICIALIZACAO

}

bool iniciarSensores() {

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    bmpDisponivel = false;
    icmDisponivel = false;

    aceleracaoValida = false;
    quaternionValido = false;

    Serial.print("ICM20948... ");

    icm.begin(Wire, 0);
    if(icm.status != ICM_20948_Stat_Ok){
        return false;
    }

    bool sucesso = true;

    sucesso &= (icm.initializeDMP() == ICM_20948_Stat_Ok);

    sucesso &= (icm.enableDMPSensor(INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR) == ICM_20948_Stat_Ok);
    sucesso &= (icm.enableDMPSensor(INV_ICM20948_SENSOR_RAW_ACCELEROMETER) == ICM_20948_Stat_Ok);

    sucesso &= (icm.setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_Stat_Ok);
    sucesso &= (icm.setDMPODRrate(DMP_ODR_Reg_Quat6, 0) == ICM_20948_Stat_Ok);
    sucesso &= (icm.enableFIFO() == ICM_20948_Stat_Ok);
    sucesso &= (icm.enableDMP() == ICM_20948_Stat_Ok);

    sucesso &= (icm.resetDMP() == ICM_20948_Stat_Ok);
    sucesso &= (icm.resetFIFO() == ICM_20948_Stat_Ok);

    if (!sucesso){
        return false;
    }
    
    icmDisponivel = true;

    Serial.print("BMP280... ");

    if (bmp.begin(0x76) || bmp.begin(0x77)) {
        Serial.println("OK");
        bmpDisponivel = true;

        bmp.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X1,
            Adafruit_BMP280::SAMPLING_X4,
            Adafruit_BMP280::FILTER_OFF,
            Adafruit_BMP280::STANDBY_MS_1
        );
    } else {
        Serial.println("NAO ENCONTRADO");
    }

    return bmpDisponivel && icmDisponivel;
}

DadoSensor lerSensores() {

    DadoSensor dado{};

    dado.timestamp_us = micros();

    // ========================================================
    // BMP280
    // ========================================================

    if (bmpDisponivel) {

        dado.pressao_pa =
            bmp.readPressure();

        dado.temperatura_c =
            bmp.readTemperature();

        dado.altitude_m =
            bmp.readAltitude(1013.25f);
    }

    // ========================================================
    // DMP / FIFO
    // ========================================================

    if (icmDisponivel) {

        bool continuarLendo = true;

        // Protecao para evitar que a funcao fique presa
        // indefinidamente caso a FIFO continue recebendo dados.
        constexpr int MAX_PACOTES_FIFO = 32;
        int pacotesLidos = 0;

        while (
            continuarLendo &&
            pacotesLidos < MAX_PACOTES_FIFO
        ) {

            icm_20948_DMP_data_t pacote{};

            icm.readDMPdataFromFIFO(&pacote);

            pacotesLidos++;

            // =================================================
            // PACOTE VALIDO
            // =================================================

            if (
                icm.status == ICM_20948_Stat_Ok ||
                icm.status == ICM_20948_Stat_FIFOMoreDataAvail
            ) {

                // =============================================
                // QUAT6
                // =============================================

                if (
                    pacote.header &
                    DMP_header_bitmap_Quat6
                ) {

                    double q1 =
                        (double)pacote.Quat6.Data.Q1 /
                        1073741824.0;

                    double q2 =
                        (double)pacote.Quat6.Data.Q2 /
                        1073741824.0;

                    double q3 =
                        (double)pacote.Quat6.Data.Q3 /
                        1073741824.0;

                    // Para um quaternion normalizado:
                    //
                    // q0² + q1² + q2² + q3² = 1
                    //
                    // Portanto:
                    //
                    // q1² + q2² + q3² <= 1

                    double soma =
                        q1 * q1 +
                        q2 * q2 +
                        q3 * q3;

                    // Só aceita um quaternion fisicamente
                    // possível.
                    if (soma <= 1.0) {

                        double q0 =
                            sqrt(1.0 - soma);

                        q0Atual = q0;
                        q1Atual = q1;
                        q2Atual = q2;
                        q3Atual = q3;

                        quaternionValido = true;
                    }
                }

                // =============================================
                // ACELEROMETRO
                // =============================================

                if (
                    pacote.header &
                    DMP_header_bitmap_Accel
                ) {

                    axAtual =
                        ((float)pacote.Raw_Accel.Data.X /
                         ACC_LSB_POR_G)
                        * GRAVIDADE;

                    ayAtual =
                        ((float)pacote.Raw_Accel.Data.Y /
                         ACC_LSB_POR_G)
                        * GRAVIDADE;

                    azAtual =
                        ((float)pacote.Raw_Accel.Data.Z /
                         ACC_LSB_POR_G)
                        * GRAVIDADE;

                    aceleracaoValida = true;
                }
            }

            // =================================================
            // CONTROLE DA FIFO
            // =================================================

            // Se a biblioteca disser que ainda existem pacotes,
            // fazemos outra leitura imediatamente.
            //
            // Caso contrario, encerramos o while.

            if (
                icm.status !=
                ICM_20948_Stat_FIFOMoreDataAvail
            ) {

                continuarLendo = false;
            }
        }
    }

    // ========================================================
// GIROSCOPIO E MAGNETOMETRO
// ========================================================

    if (icmDisponivel && icm.dataReady()) {
    
        icm.getAGMT();
    
        dado.gx =
            icm.gyrX() * DEG_PARA_RAD;
    
        dado.gy =
            icm.gyrY() * DEG_PARA_RAD;
    
        dado.gz =
            icm.gyrZ() * DEG_PARA_RAD;
    
        dado.mx =
            icm.magX();
    
        dado.my =
            icm.magY();
    
        dado.mz =
            icm.magZ();
    }

    // ========================================================
    // ENTREGA A ULTIMA ACELERACAO VALIDA
    // ========================================================

    if (aceleracaoValida) {

        dado.ax = axAtual;
        dado.ay = ayAtual;
        dado.az = azAtual;
    }

    // ========================================================
    // ENTREGA O ULTIMO QUATERNION VALIDO
    // ========================================================

    if (quaternionValido) {

        dado.q0 = (float)q0Atual;
        dado.q1 = (float)q1Atual;
        dado.q2 = (float)q2Atual;
        dado.q3 = (float)q3Atual;

        dado.quaternion_valido = true;
    }
    else {

        dado.quaternion_valido = false;
    }

    return dado;
}

void testarTempoLeitura() {
    constexpr int N = 100;
    uint64_t soma = 0;

    for (int i = 0; i < N; i++){
        uint32_t inicio = micros();

        DadoSensor dado = lerSensores();
        (void)dado;

        soma += micros() - inicio;

        delay(10);
    }

    Serial.print("tempo_medio_leitura_us,");
    Serial.println(soma / N);
    
}