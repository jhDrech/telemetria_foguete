#pragma once
#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include "simulacao.h"

void iniciarHIL() {
    icmDisponivel = true;
    Serial.println("Modo HIL iniciado");
}

bool lerDadosHIL(DadoSensor& dado){
     static char linha[100];
    static int indice = 0;

    while (Serial.available() > 0) {
        char caractere = Serial.read();

        if (caractere == '\n') {
            linha[indice] = '\0';
            indice = 0;

            unsigned long timestamp;
            float pressao, temp, ax, ay, az;

            int campos = sscanf(
                linha,
                "%lu,%f,%f,%f,%f,%f",
                &timestamp, &pressao, &temp, &ax, &ay, &az
            );

            if (campos != 6) {
                return false;
            }

            if (!isfinite(pressao) || pressao <= 0 ||
                !isfinite(temp) ||
                !isfinite(ax) || !isfinite(ay) || !isfinite(az)) {
                return false;
            }

            dado.timestamp_ms = timestamp;
            dado.pressao = pressao;
            dado.temp = temp;
            dado.ax = ax;
            dado.ay = ay;
            dado.az = az;

            return true;
        }

        if (indice < sizeof(linha) - 1) {
            linha[indice++] = caractere;
        }
        else {
            indice = 0;
            return false;
        }
    }

    return false;

    //  if (Serial.available() == 0) {
    //     return false;
    // }

    // String linha = Serial.readStringUntil('\n');

    // unsigned long timestamp;
    // float pressao, temp, ax, ay, az;

    // int campos = sscanf(
    //     linha.c_str(),
    //     "%lu,%f,%f,%f,%f,%f",
    //     &timestamp, &pressao, &temp, &ax, &ay, &az
    // );

    // if (campos != 6) {
    //     return false;
    // }

    // dado.timestamp_ms = timestamp;
    // dado.pressao = pressao;
    // dado.temp = temp;
    // dado.ax = ax;
    // dado.ay = ay;
    // dado.az = az;

    // return true;
}

bool atualizarPressaoInicialHIL(float pressaoAtual, float& pressaoInicial) {
        static float somaPressao = 0;
    static int quantidadeLeituras = 0;
    static uint32_t inicio = 0;
    static bool calibracaoConcluida = false;

    if (calibracaoConcluida) {
        return true;
    }

    if (!isfinite(pressaoAtual) || pressaoAtual <= 0) {
        return false;
    }

    if (quantidadeLeituras == 0) {
        inicio = millis();
    }

    somaPressao += pressaoAtual;
    quantidadeLeituras++;

    if (millis() - inicio >= 3000) {
        pressaoInicial = somaPressao / quantidadeLeituras;
        calibracaoConcluida = true;
        return true;
    }

    return false;


    // static float somaPressao = 0;
    // static int quantidadeLeituras = 0;
    // static uint32_t inicio = millis();

    // if (pressaoAtual > 0) {
    //     somaPressao += pressaoAtual;
    //     quantidadeLeituras++;
    // }

    // if (millis() - inicio >= 3000 && quantidadeLeituras > 0) {
    //     pressaoInicial = somaPressao / quantidadeLeituras;
    //     return true;
    // }

    // return false;
}