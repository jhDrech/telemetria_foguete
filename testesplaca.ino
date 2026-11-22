// Teste isolado do BMP180 - sem ICM, sem Wi-Fi, sem o resto do sistema.
// Objetivo: confirmar que o sensor de pressão responde e os valores fazem sentido.

#include <Wire.h>
#include <Adafruit_BMP280.h>
// #include <Adafruit_ICM20948.h>
// #include <Adafruit_ICM20X.h>

#include <Adafruit_NeoPixel.h>


#define PIN_SDA 8
#define PIN_SCL 9
#define LED_PIN 48

#define SERVO_PIN 10
#define SERVO_FREQ 50     // 50 Hz = período de 20 ms
#define SERVO_RES 14     // resolução de 14 bits


Adafruit_BMP280 bmp;
// Adafruit_ICM20948 icm;

int posiIni = 20;
int posiFin = 150;

void servoAngulo(int angulo) {
  angulo = constrain(angulo, 0, 180);
  uint32_t us = map(angulo, 0, 180, 500, 2400);              // largura do pulso
  uint32_t duty = us * ((1UL << SERVO_RES) - 1) / 20000;     // converte µs em duty
  ledcWrite(SERVO_PIN, duty);
}

void servoSolta() {        // equivale ao detach: para de mandar pulsos
  ledcWrite(SERVO_PIN, 0);
}



Adafruit_NeoPixel rgbLed(1, LED_PIN, NEO_GRB + NEO_KHZ800);
void mudarCorLED(uint8_t vermelho, uint8_t verde, uint8_t azul) {
  rgbLed.setPixelColor(0, rgbLed.Color(vermelho, verde, azul));
  rgbLed.show();
}

void setup() {
  rgbLed.begin();
  Serial.begin(115200);
  mudarCorLED(0,255,0);
  // while (!Serial) delay(10);
  delay(2000);
  Serial.println("INICIANDO SETUP");

  Wire.begin(PIN_SDA, PIN_SCL);
  
  mudarCorLED(255,0,0);


  while(!bmp.begin(0x76)){
    delay(1000);
    Serial.println("Erro bmp ..");
  }

  Serial.println("BMP280 inicializado com sucesso.");
  mudarCorLED(0,255,0);

  
  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RES);
  Serial.println("Servo iniciado");
  delay(500);
  servoAngulo(posiIni);
  Serial.print("Servo na Posição: ");
  Serial.println(posiIni);
  delay(1000);
  servoSolta();
  uint32_t timestampMs = millis();

  // bool ok = false;
  // for (int tentativa = 1; tentativa <= 5 && !ok; tentativa++) {**
  //   Serial.print("Tentativa ");**
  //   Serial.print(tentativa);**
  //   Serial.println(" de inicializar o ICM20948...");**
  //   ok = icm.begin_I2C(0x68,&Wire);**
  //   if (!ok) delay(300);**
  // }**
  // if (!ok) {**
    //   Serial.println("ERRO: ICM20948 não encontrado após 5 tentativas. Confira VCC, GND, SDA, SCL, NCS(->3V3) e ADO(->GND).");
    //   while (1) delay(100);
    // }
    // Serial.println("ICM20948 inicializado com sucesso.");
  }
  void loop() {
    uint32_t timestampMs = millis();
    delay(3000);
    ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RES);
    delay(500);
    servoAngulo(posiFin);
    Serial.print("Servo na Posição: ");
    Serial.println(posiFin);
    delay(1000);
    servoSolta();

  
  Serial.println(timestampMs/1000);
  float temperatura = bmp.readTemperature();
  int32_t pressao = bmp.readPressure();               // em Pascal
  float altitude = bmp.readAltitude(1013.25f);           // 101325 Pa = pressão padrão ao nível do mar

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.print(" C\t Pressao: ");
  Serial.print(pressao);
  Serial.print(" Pa\t Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");
  

  servoAngulo(posiIni);
  Serial.print("Servo na Posição: ");
  Serial.println(posiIni);
  // Serial.println("20 graus");
  // servo.attach(10, 500, 2400);
  // delay(1000);
  // servo.write(20);
  // servo.detach();
  // Serial.println("90 graus");
  // servo.attach(10, 500, 2400);
  // delay(1000);
  // servo.write(90);
  // servo.detach();
  // delay(2000);

  // Serial.println("160 graus");
  // servo.attach(10, 500, 2400);
  // delay(1000);
  // servo.write(160);
  // servo.detach();
  // O que esperar:
  // - Temperatura perto da temperatura ambiente real.
  // - Altitude deve ficar relativamente estável (não variando loucamente) quando o sensor está parado.
  //   O valor absoluto pode não ser exatamente 0 (depende da pressão atmosférica do dia) - isso é normal,
  //   é para isso que existe a etapa de calibração/tara no firmware completo.
  // sensors_event_t accel, gyro, temp, mag;
  // icm.getEvent(&accel, &gyro, &temp, &mag);
  // Serial.print("Accel (m/s^2)  X: "); Serial.print(accel.acceleration.x);
  // Serial.print("  Y: "); Serial.print(accel.acceleration.y);
  // Serial.print("  Z: "); Serial.println(accel.acceleration.z);
  // Serial.print("Gyro (rad/s)   X: "); Serial.print(gyro.gyro.x);
  // Serial.print("  Y: "); Serial.print(gyro.gyro.y);
  // Serial.print("  Z: "); Serial.println(gyro.gyro.z);
  // Serial.print("Mag (uT)       X: "); Serial.print(mag.magnetic.x);
  // Serial.print("  Y: "); Serial.print(mag.magnetic.y);
  // Serial.print("  Z: "); Serial.println(mag.magnetic.z);

  // Serial.println("---");
  // delay(500);
}