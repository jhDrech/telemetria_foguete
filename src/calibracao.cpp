#include "calibracao.h"
#include "sensores.h"

static constexpr float BIAS_ACCEL_X = -0.146f;

Calibracao calibrarSistema(int numeroAmostras, int intervaloMs) {
    Calibracao calibracao{};

    double somaAltitude = 0.0;

    double somaGx = 0.0;
    double somaGy = 0.0;
    double somaGz = 0.0;

    for (int i = 0; i < numeroAmostras; i++) {
        DadoSensor dado = lerSensores();

        Serial.print("Calibracao ");
        Serial.print(i);
        Serial.print(" | Gx: ");
        Serial.print(dado.gx, 5);
        Serial.print(" | Gy: ");
        Serial.print(dado.gy, 5);
        Serial.print(" | Gz: ");
        Serial.println(dado.gz, 5);

        somaAltitude += dado.altitude_m;

        somaGx += dado.gx;
        somaGy += dado.gy;
        somaGz += dado.gz;

        delay(intervaloMs);
    }

    calibracao.altitudeInicial_m = somaAltitude / numeroAmostras;
    
    calibracao.giroBiasX = somaGx / numeroAmostras;
    calibracao.giroBiasY = somaGy / numeroAmostras;
    calibracao.giroBiasZ = somaGz / numeroAmostras;

    return calibracao;
};

DadoTratado aplicarCalibracao(const DadoSensor& dado,const Calibracao& calibracao){
    DadoTratado tratado{};

    tratado.timestamp_us = dado.timestamp_us;

    tratado.altitude_relativa_m = dado.altitude_m - calibracao.altitudeInicial_m;

    tratado.ax = dado.ax - BIAS_ACCEL_X;
    tratado.ay = dado.ay;
    tratado.az = dado.az;

    tratado.gx_corrigido = dado.gx - calibracao.giroBiasX;
    tratado.gy_corrigido = dado.gy - calibracao.giroBiasY;
    tratado.gz_corrigido = dado.gz - calibracao.giroBiasZ;

    tratado.mx = dado.mx;
    tratado.my = dado.my;
    tratado.mz = dado.mz;

    return tratado;
}