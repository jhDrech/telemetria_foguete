#include <Arduino.h>
#include "sensores.h"

struct BiasGyro {
    float gx;
    float gy;
    float gz;
};

BiasGyro medirBias(int numeroAmostras, int intervaloMs) {
    double somaGx = 0.0;
    double somaGy = 0.0;
    double somaGz = 0.0;

    for (int i = 0; i < numeroAmostras; i++) {
        DadoSensor dado = lerSensores();

        somaGx += dado.gx;
        somaGy += dado.gy;
        somaGz += dado.gz;

        delay(intervaloMs);
    }

    BiasGyro bias;

    bias.gx = somaGx / numeroAmostras;
    bias.gy = somaGy / numeroAmostras;
    bias.gz = somaGz / numeroAmostras;

    return bias;
}

void setup() {
    Serial.begin(115200);
    delay(8000);

    Serial.println("=== TESTE DE ESTABILIDADE DO BIAS ===");

    if (!iniciarSensores()) {
        Serial.println("Erro ao inicializar sensores.");
        return;
    }

    Serial.println("Mantenha o sistema completamente parado.");
    Serial.println();

    const int numeroAmostras = 100;
    const int intervaloMs = 20;

    const int temposSegundos[] = {
        0,
        30,
        60,
        90,
        120
    };

    const int numeroJanelas =
        sizeof(temposSegundos) /
        sizeof(temposSegundos[0]);

    unsigned long inicio = millis();

    Serial.println("tempo_s;bias_gx;bias_gy;bias_gz");

    for (int janela = 0; janela < numeroJanelas; janela++) {

        unsigned long alvo =
            inicio +
            temposSegundos[janela] * 1000UL;

        while (millis() < alvo) {
            delay(10);
        }

        BiasGyro bias =
            medirBias(
                numeroAmostras,
                intervaloMs
            );

        Serial.print(temposSegundos[janela]);
        Serial.print(";");

        Serial.print(bias.gx, 6);
        Serial.print(";");

        Serial.print(bias.gy, 6);
        Serial.print(";");

        Serial.println(bias.gz, 6);
    }

    Serial.println();
    Serial.println("=== TESTE FINALIZADO ===");
}

void loop() {
}