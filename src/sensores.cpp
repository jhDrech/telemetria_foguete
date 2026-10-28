#include <Arduino.h>
#include<Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include "sensores.h"

ICM_20948_I2C icm;
Adafruit_BMP280 bmp;

constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;
constexpr uint8_t ENDERECO_ICM = 0x68;
constexpr uint8_t ENDERECO_BMP = 0x77;
constexpr uint16_t CONVERCAO_ACCEL = (9.80665f / 1000.0f); // converte milig da accel para m/s²
constexpr uint16_t CONVERCAO_GIRO = DEG_TO_RAD; // converte milig da accel para m/s²


bool iniciarSensores(){
    Wire.begin(PIN_SDA, PIN_SCL);
    icm.begin(Wire, 0);

    if(icm.status != ICM_20948_Stat_Ok){
        return false;
    }

    if(!bmp.begin(ENDERECO_BMP)){
        return false;
    }
    
    return true;
}

void loop(){
    DadoSensor dados;
    dados.temp = lerTemperatura();
    dados.pressao = lerPressao();
    Serial.println(dados.temp);
    Serial.println(dados.pressao);
}

float lerPressao(){
    float pressao = bmp.readPressure();
    return pressao;
}

float lerTemperatura(){
    float temp = bmp.readTemperature();
    return temp;
}

namespace sensor {
    bool lerSensores(DadoSensor& dado){
        if(!icm.dataReady()){
            return false;
        }
        
        dado.temp = bmp.readTemperature();
        dado.pressao = bmp.readPressure();
    
        icm.getAGMT();
        dado.timestamp_ms = millis();

        dado.ax = icm.accX() * CONVERCAO_ACCEL;
        dado.ay = icm.accY() * CONVERCAO_ACCEL;
        dado.az = icm.accZ() * CONVERCAO_ACCEL;
    
        dado.gx = icm.gyrX() * CONVERCAO_GIRO;
        dado.gy = icm.gyrY() * CONVERCAO_GIRO;
        dado.gz = icm.gyrZ() * CONVERCAO_GIRO;
    
        dado.mx = icm.magX();
        dado.my = icm.magY();
        dado.mz = icm.magZ();

    
        return true;
    }
}

