#include <Arduino.h>
#include<Wire.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include <Adafruit_AHRS.h>
#include "sensores.h"

ICM_20948_I2C icm;
Adafruit_BMP280 bmp;
Adafruit_Madgwick filtroAccel;

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
        bmpDisponivel = false;
    }else{
        bmpDisponivel = true;
    }

    if(icm.status != ICM_20948_Stat_Ok){
        icmDisponivel = false;
    }else{
        icmDisponivel = true;
    }

    // define 100 hz como valor inicial do filtro de accel
    filtroAccel.begin(62.5f);
    
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

void atualizarOrientacao(const DadoSensor& dado, float dt, float biasGx, float biasGy, float biasGz){
    float gxCorrigido = dado.gx - biasGx;
    float gyCorrigido = dado.gy - biasGy;
    float gzCorrigido = dado.gz - biasGz;
 
    
    // converte nossos dados de orientação de rad/s para graus/s padrão da bibliteca
    float gxGraus = dado.gx * 180.0f / PI;
    float gyGraus = dado.gy * 180.0f / PI;
    float gzGraus = dado.gz * 180.0f / PI;

    filtroAccel.updateIMU(gxGraus, gyGraus, gzGraus, dado.ax, dado.ay, dado.az, dt);
}

float obterAlturaRelativa(float pressaoInicial, float pressaoAtual){
    float alturaRelativa;
    if(pressaoInicial >= 0){
        alturaRelativa = 44330 * (1 - (pow((pressaoAtual / pressaoInicial),0.1903)));
    }
    return alturaRelativa;
}

float obterAceleracaoTotal(DadoSensor& dado){
    float accelTotal;
    accelTotal = sqrt(pow(dado.ax, 2) + pow(dado.ay, 2) + pow(dado.az, 2));
    return accelTotal;
}

// void obterAceleracaoLinear(const DadoSensor& dado, float& axLinear, float& ayLinear, float& azLinear){
//     float gravX, gravY, gravZ;
//     filtroAccel.getGravityVector(&gravX, &gravY, &gravZ);

//     gravX *= 9.8;
//     gravY *= 9.8;
//     gravZ *= 9.8;

//     axLinear = dado.ax - gravX;
//     ayLinear = dado.ay - gravY;
//     azLinear = dado.az - gravZ;
// }

// float obterAceleracaoVertical(float axLinear, float ayLinear, float azLinear){
//     float qw, qx, qy, qz;

//     filtroAccel.getQuaternion(&qw, &qx, &qy, &qz);
//     float aceleracaoVertical = 2.0f * (qx * qz - qw * qy) * axLinear + 2.0f * (qy * qz + qw * qx) * ayLinear + (1.0f - 2.0f * (pow(qx,2) + pow(qy,2))) * azLinear;
//     return aceleracaoVertical;
// }

// float calibrarBiasAceleracaoVertical(DadoSensor amostras[],int quantidadeAmostras){
//     float soma = 0.0f;
//     int validas = 0;

//     for(int i = 1; i < quantidadeAmostras; i++){

//         float dt =
//             (amostras[i].timestamp_ms - amostras[i - 1].timestamp_ms)
//             / 1000.0f;

//         // atualizarOrientacao(amostras[i], dt);

//         float axLinear, ayLinear, azLinear;

//         obterAceleracaoLinear(
//             amostras[i],
//             axLinear,
//             ayLinear,
//             azLinear
//         );

//         float aceleracaoVertical =
//             obterAceleracaoVertical(
//                 axLinear,
//                 ayLinear,
//                 azLinear
//             );

//         soma += aceleracaoVertical;
//         validas++;
//     }

//     if(validas == 0){
//         return 0.0f;
//     }

//     return soma / validas;
// }

// float obterVelocidadeVertical(float aceleracaoVertical, float dt){
//     static float velocidadeVertical = 0.0f;
//     velocidadeVertical += aceleracaoVertical * dt;

//     return velocidadeVertical;
// }

bool reconhecerDisparo(float accelTotal, float alturaRelativa){
    static int qtdAmostras = 0;
    if(accelTotal > 2 * 9.8f && alturaRelativa > 5){
        qtdAmostras++;
    }
    if(qtdAmostras >= 5){
        return true;
    }
    return false;
}

void processarBuffer(DadoSensor buffer[], int quantidadeDados){
    for (int i = 1; i < quantidadeDados; i++){
        float dt = (buffer[i].timestamp_ms - buffer[i - 1].timestamp_ms) / 1000.0f;

        DadoEstimado estimado;

        // atualizarOrientacao(buffer[i], dt);
    }
}

void calibrarGiroscopio(
    float& biasGx,
    float& biasGy,
    float& biasGz
){
    float somaGx = 0.0f;
    float somaGy = 0.0f;
    float somaGz = 0.0f;

    int quantidade = 0;

    uint32_t inicio = millis();

    while(millis() - inicio < 5000){

        DadoSensor dado;

        lerSensores(dado);

        if(!isnan(dado.gx) &&
           !isnan(dado.gy) &&
           !isnan(dado.gz)){

            somaGx += dado.gx;
            somaGy += dado.gy;
            somaGz += dado.gz;

            quantidade++;
        }

        delay(16);
    }

    if(quantidade > 0){
        biasGx = somaGx / quantidade;
        biasGy = somaGy / quantidade;
        biasGz = somaGz / quantidade;
    }
}

bool processarDadosBaixados(const String& linha,DadoSensor& dado){
    int inicio = 0;
    int fim;

    String campos[12];

    for(int i = 0; i < 11; i++){
        fim = linha.indexOf(',', inicio);

        if(fim == -1){
            return false;
        }

        campos[i] = linha.substring(inicio, fim);
        inicio = fim + 1;
    }

    campos[11] = linha.substring(inicio);

    dado.timestamp_ms = campos[0].toInt();

    dado.pressao = campos[1].toFloat();
    dado.temp = campos[2].toFloat();

    dado.ax = campos[3].toFloat();
    dado.ay = campos[4].toFloat();
    dado.az = campos[5].toFloat();

    dado.gx = campos[6].toFloat();
    dado.gy = campos[7].toFloat();
    dado.gz = campos[8].toFloat();

    return true;
}


bool reconhcerIcmDisponível(){

}
 