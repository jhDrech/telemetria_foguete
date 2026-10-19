#include <Arduino.h>

#include "sensores.h"
#include "calibracao.h"

void setup() {
    Serial.begin(115200);
    delay(8000);

    Serial.println();
    Serial.println("=== TESTE DE CALIBRACAO ===");

    if (!iniciarSensores()) {
        Serial.println("Erro ao inicializar sensores.");
        return;
    }

    Serial.println("Mantenha o sistema completamente parado.");
    delay(3000);

    Calibracao calibracao =
        calibrarSistema(100, 20);

    Serial.println();
    Serial.println("=== PARAMETROS CALCULADOS ===");

    Serial.print("Altitude inicial: ");
    Serial.print(calibracao.altitudeInicial_m, 3);
    Serial.println(" m");

    Serial.print("Bias Gx: ");
    Serial.print(calibracao.giroBiasX, 5);
    Serial.println(" rad/s");

    Serial.print("Bias Gy: ");
    Serial.print(calibracao.giroBiasY, 5);
    Serial.println(" rad/s");

    Serial.print("Bias Gz: ");
    Serial.print(calibracao.giroBiasZ, 5);
    Serial.println(" rad/s");

    Serial.println();
    Serial.println("=== COMPARACAO BRUTO x CORRIGIDO ===");

    for (int i = 0; i < 20; i++) {

        DadoSensor bruto =
            lerSensores();

        DadoTratado tratado =
            aplicarCalibracao(
                bruto,
                calibracao
            );

        Serial.print("Gy bruto: ");
        Serial.print(bruto.gy, 5);

        Serial.print(" | Gy corrigido: ");
        Serial.print(
            tratado.gy_corrigido,
            5
        );

        Serial.print(" | Alt abs: ");
        Serial.print(
            bruto.altitude_m,
            3
        );

        Serial.print(" | Alt rel: ");
        Serial.println(
            tratado.altitude_relativa_m,
            3
        );

        delay(200);
    }
}

void loop() {
}