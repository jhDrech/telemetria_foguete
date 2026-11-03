#include "wifi.h"
#include <WiFi.h>
#include <Arduino.h>

const char* NOME_REDE = "FOGUETE_ESP32";
const char* SENHA = "12345478";

bool iniciarWiFi() {
    bool sucesso = WiFi.softAP(NOME_REDE, SENHA);

    if (!sucesso) {
        Serial.println("Falha ao iniciar Wi-Fi!");
    } else {
        Serial.println("Ponto de acesso iniciado!");
        Serial.print("Endereco IP: ");
        Serial.println(WiFi.softAPIP());
    }



    return sucesso;
}