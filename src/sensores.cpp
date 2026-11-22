#include <Arduino.h>
#include<Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include "sensores.h"

ICM_20948_I2C icm;
Adafruit_BMP280 bmp;

bool icmDisponivel;
bool bmpDisponivel;

constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;
constexpr uint8_t ENDERECO_ICM = 0x68;
constexpr uint8_t ENDERECO_BMP = 0x77;
constexpr float CONVERCAO_ACCEL = (9.80665f / 1000.0f); // converte milig da accel para m/s²
constexpr float CONVERCAO_GIRO = DEG_TO_RAD; // converte milig da accel para m/s²


bool iniciarSensores(){
    Wire.begin(PIN_SDA, PIN_SCL);
    icm.begin(Wire, 0);
    
    if(!bmp.begin(ENDERECO_BMP)){
        return false;
    }else{
        bmpDisponivel = true;
    }

    if(icm.status != ICM_20948_Stat_Ok){
        icmDisponivel = false;
    }else{
        icmDisponivel = true;
    }
    
    return true;
}

bool lerSensores(DadoSensor& dado){
    dado.timestamp_ms = millis();
    dado.temp = bmp.readTemperature();
    dado.pressao = bmp.readPressure();
    
    if(icmDisponivel == true){
        if(icm.dataReady()){
            icm.getAGMT();
        
            dado.ax = icm.accX() * CONVERCAO_ACCEL;
            dado.ay = icm.accY() * CONVERCAO_ACCEL;
            dado.az = icm.accZ() * CONVERCAO_ACCEL;
            
            dado.gx = icm.gyrX() * CONVERCAO_GIRO;
            dado.gy = icm.gyrY() * CONVERCAO_GIRO;
            dado.gz = icm.gyrZ() * CONVERCAO_GIRO;
            
            dado.mx = icm.magX();
            dado.my = icm.magY();
            dado.mz = icm.magZ();
        }
    }
    
    return true;
};

bool atualizarPressaoInicial(float& pressaoInicial){
    static float soma_pressao = 0;
    static int qtd_aquisicoes = 0;
    static bool pressao_pronta = false; 
    static uint32_t inicio = millis();

    if(pressao_pronta == true){
        return true;
    }

    soma_pressao += bmp.readPressure();
    qtd_aquisicoes++;

    // desse jeito não para o código pra esperar só o bmp
    if(millis() - inicio >= 3000){
        pressaoInicial = soma_pressao / qtd_aquisicoes;
        pressao_pronta = true;
        return true;
    }
    
    return false;
};

float obterAlturaRelativa(float pressaoInicial, float pressaoAtual){
    float alturaRelativa = NAN;
    if(pressaoInicial > 0 && pressaoAtual > 0){
        alturaRelativa = 44330 * (1 - (pow((pressaoAtual / pressaoInicial),0.1903)));
    }
    return alturaRelativa;
}


float obterAceleracaoTotal(DadoSensor& dado){
    return sqrt(pow(dado.ax, 2) + pow(dado.ay, 2) + pow(dado.az, 2));
}
