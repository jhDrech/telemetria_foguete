#include <Arduino.h>
#include<Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include "sensores.h"
#include <math.h>

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

namespace {
    double somaPressao = 0.0;
    uint32_t quantidadePressao = 0;
    uint32_t inicioPressaoMs = 0;
    bool coletaPressaoIniciada = false;
    bool pressaoPronta = false;
    float referenciaPressao = NAN;
}

bool iniciarSensores(){
    bmpDisponivel = false;
    icmDisponivel = false;
    somaPressao = 0.0;
    quantidadePressao = 0;
    coletaPressaoIniciada = false;
    pressaoPronta = false;
    referenciaPressao = NAN;
    Wire.begin(PIN_SDA, PIN_SCL);
    icm.begin(Wire, 0);
    icmDisponivel = (icm.status == ICM_20948_Stat_Ok);
    
    // Mantem os dois enderecos aceitos pela versao NoICM do Vito.
    if(!bmp.begin(ENDERECO_BMP) && !bmp.begin(0x76)){
        return false;
    }else{
        bmpDisponivel = true;
    }

    // Mantem a configuracao do BMP da versao NoICM.
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X1,
                    Adafruit_BMP280::SAMPLING_X4,
                    Adafruit_BMP280::FILTER_OFF,
                    Adafruit_BMP280::STANDBY_MS_1);
    
    return true;
}

bool lerSensores(DadoSensor& dado){
    // Nao reaproveita valores antigos quando o ICM estiver sem amostra.
    dado = DadoSensor{};
    dado.timestamp_ms = millis();
    if (!bmpDisponivel) {
        return false;
    }
    dado.temp = bmp.readTemperature();
    dado.pressao = bmp.readPressure();
    
    if(icmDisponivel == true){
        if(icm.dataReady()){
            icm.getAGMT();
            if (icm.status != ICM_20948_Stat_Ok) {
                return isfinite(dado.pressao) && dado.pressao > 0.0f;
            }
        
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
    
    return isfinite(dado.pressao) && dado.pressao > 0.0f;
};

bool atualizarPressaoInicial(float& pressaoInicial){
    if(pressaoPronta){
        pressaoInicial = referenciaPressao;
        return true;
    }

    if (!bmpDisponivel) {
        return false;
    }
    const float pressao = bmp.readPressure();
    if (!isfinite(pressao) || pressao <= 0.0f) {
        return false;
    }
    if (!coletaPressaoIniciada) {
        inicioPressaoMs = millis();
        coletaPressaoIniciada = true;
    }
    somaPressao += pressao;
    quantidadePressao++;

    // desse jeito não para o código pra esperar só o bmp
    if(millis() - inicioPressaoMs >= 3000){
        referenciaPressao = somaPressao / quantidadePressao;
        pressaoInicial = referenciaPressao;
        pressaoPronta = true;
        return true;
    }
    
    return false;
};

float obterAlturaRelativa(float pressaoInicial, float pressaoAtual){
    if (!isfinite(pressaoInicial) || !isfinite(pressaoAtual) ||
        pressaoInicial <= 0.0f || pressaoAtual <= 0.0f) {
        return NAN;
    }
    return 44330.0f * (1.0f - powf(pressaoAtual / pressaoInicial, 0.1903f));
}

