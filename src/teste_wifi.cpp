#include <Arduino.h>
#include "rede_wifi.h"

void setup()
{
    Serial.begin(115200);
    delay(3000);

    if (!iniciarWiFi())
    {
        Serial.println("Falha no teste de Wi-Fi.");
    }
}

void loop()
{
    atualizarWiFi();
    delay(2);
}