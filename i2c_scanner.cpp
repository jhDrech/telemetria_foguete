// I2C Scanner - adaptado para o ESP32-S3 Super Mini (SDA=8, SCL=9)
// Varre todos os endereços I2C possíveis e imprime quais sensores responderam.
// Esperado no projeto: 0x68 (ICM20948) e 0x77 (BMP180)
#include <Arduino.h>
#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

void setup() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Serial.begin(115200);
  while (!Serial) delay(10); // espera o Serial abrir (necessário em alguns ESP32-S3 com USB CDC)
  delay(1000);
  Serial.println("\nIniciando I2C Scanner...");
}

void loop() {
  byte erro, endereco;
  int dispositivosEncontrados = 0;

  Serial.println("Escaneando...");

  for (endereco = 1; endereco < 127; endereco++) {
    Wire.beginTransmission(endereco);
    erro = Wire.endTransmission();

    if (erro == 0) {
      Serial.print("Dispositivo encontrado no endereço 0x");
      if (endereco < 16) Serial.print("0");
      Serial.print(endereco, HEX);

      if (endereco == 0x68) {
        Serial.print("  -> provavelmente o ICM20948");
      } else if (endereco == 0x77 || endereco == 0x76) {
        Serial.print("  -> provavelmente o BMP180");
      }

      Serial.println();
      dispositivosEncontrados++;
    } else if (erro == 4) {
      Serial.print("Erro desconhecido no endereço 0x");
      if (endereco < 16) Serial.print("0");
      Serial.println(endereco, HEX);
    }
  }

  if (dispositivosEncontrados == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado. Confira VCC, GND, SDA, SCL, NCS e ADO.");
  } else {
    Serial.print(dispositivosEncontrados);
    Serial.println(" dispositivo(s) encontrado(s). ");
  }

  delay(3000); // repete a cada 3 segundos
}
