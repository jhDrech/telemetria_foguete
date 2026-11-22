#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>

#include "sensores.h"

namespace
{
    ICM_20948_I2C icm;
    Adafruit_BMP280 bmp;

    bool icmDisponivel = false;
    bool bmpDisponivel = false;

    constexpr uint8_t PIN_SDA = 8;
    constexpr uint8_t PIN_SCL = 9;
    constexpr uint8_t ENDERECO_BMP = 0x77;

    constexpr float CONVERSAO_ACCEL =
        9.80665f / 1000.0f; // mg -> m/s²
}

bool iniciarSensores()
{
    Wire.begin(PIN_SDA, PIN_SCL);

    bmpDisponivel = bmp.begin(ENDERECO_BMP);

    // AD0 = 0 corresponde ao endereco 0x68.
    icm.begin(Wire, 0);
    icmDisponivel =
        icm.status == ICM_20948_Stat_Ok;

    // Os dois sensores fazem parte do fluxo principal:
    // ICM detecta o lancamento e BMP acompanha o voo.
    return bmpDisponivel && icmDisponivel;
}

bool lerSensores(DadoSensor &dado)
{
    dado.timestamp_ms = millis();

    if (!bmpDisponivel)
    {
        return false;
    }

    dado.temp = bmp.readTemperature();
    dado.pressao = bmp.readPressure();

    // Cada DadoSensor e criado novamente no loop.
    // Se nao houver amostra nova do ICM, ax/ay/az permanecem NAN
    // e nao participam da deteccao do lancamento.
    if (icmDisponivel && icm.dataReady())
    {
        icm.getAGMT();

        dado.ax = icm.accX() * CONVERSAO_ACCEL;
        dado.ay = icm.accY() * CONVERSAO_ACCEL;
        dado.az = icm.accZ() * CONVERSAO_ACCEL;
    }

    return
        isfinite(dado.pressao) &&
        dado.pressao > 0.0f &&
        isfinite(dado.temp);
}

bool atualizarPressaoInicial(float &pressaoInicial)
{
    static float somaPressao = 0.0f;
    static uint32_t quantidadeAquisicoes = 0;
    static bool pressaoPronta = false;
    static uint32_t inicio = millis();

    if (pressaoPronta)
    {
        return true;
    }

    float pressaoAtual = bmp.readPressure();

    if (isfinite(pressaoAtual) &&
        pressaoAtual > 0.0f)
    {
        somaPressao += pressaoAtual;
        quantidadeAquisicoes++;
    }

    if (millis() - inicio >= 3000)
    {
        if (quantidadeAquisicoes == 0)
        {
            return false;
        }

        pressaoInicial =
            somaPressao / quantidadeAquisicoes;

        pressaoPronta = true;
        return true;
    }

    return false;
}

float obterAlturaRelativa(
    float pressaoInicial,
    float pressaoAtual)
{
    if (!isfinite(pressaoInicial) ||
        !isfinite(pressaoAtual) ||
        pressaoInicial <= 0.0f ||
        pressaoAtual <= 0.0f)
    {
        return NAN;
    }

    return 44330.0f *
           (1.0f -
            pow(
                pressaoAtual / pressaoInicial,
                0.1903f));
}

float obterAceleracaoTotal(const DadoSensor &dado)
{
    if (!isfinite(dado.ax) ||
        !isfinite(dado.ay) ||
        !isfinite(dado.az))
    {
        return NAN;
    }

    return sqrtf(
        dado.ax * dado.ax +
        dado.ay * dado.ay +
        dado.az * dado.az);
}
