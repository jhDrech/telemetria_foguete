#include <Arduino.h>
#include <ESP32Servo.h>

#include "sensores.h"
#include "orientacao.h"
#include "movimento.h"
#include "calibracao.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "detector_lancamento.h"

// Momento em que o experimento realmente começa.
// Assim o timestamp do CSV começa próximo de zero,
// independentemente do tempo gasto no setup.
static uint32_t inicioTesteMs = 0;
static Calibracao calibracao;
static DetectorApogeu detectorApogeu;
static ControleParaquedas controleParaquedas;
static Servo servoParaquedas;
static DetectorLancamento detectorLancamento;

static const int ANGULO_INICIAL = 180;
static const int ANGULO_ACIONAMENTO = 60;
static EstadoParaquedas ultimoEstado = EstadoParaquedas::AGUARDANDO;

void setup()
{

    Serial.begin(115200);

    // Mantemos o atraso que já estava sendo usado
    // para estabilização da comunicação USB.
    delay(8000);

    // ========================================================
    // INICIALIZACAO DOS SENSORES
    // ========================================================

    if (!iniciarSensores())
    {

        while (true)
        {
            delay(1000);
        }
    }

    calibracao = calibrarSistema(100, 10);

    // ========================================================
    // INICIALIZACAO DA ORIENTACAO / DMP
    // ========================================================

    iniciarOrientacao();

    // INICIALIZACAO DO MOVIMENTO

    iniciarMovimento();

    // ========================================================
    // INICIO EFETIVO DO EXPERIMENTO
    // ========================================================

    inicioTesteMs = millis();

    detectorApogeu = inicializarDetectorApogeu();

    controleParaquedas = inicializarControleParaquedas();

    detectorLancamento = inicializarDetectorLancamento();

    // configuração do servo
    servoParaquedas.attach(10);
    servoParaquedas.write(ANGULO_INICIAL);
    // ========================================================
    // CABECALHO CSV
    // ========================================================

    Serial.println(
        "timestamp_ms,"
        "pressao_pa,temperatura_c,altitude_m,"
        "ax,ay,az,"
        "gx,gy,gz,"
        "mx,my,mz,"
        "q0,q1,q2,q3,"
        "norma_q,"
        "quaternion_valido,"
        "roll_deg,pitch_deg,"
        "variacao_roll_deg,variacao_pitch_deg,"
        "janela_estabilidade,"
        "dmp_pronto,"
        "ax_linear,ay_linear,az_linear,"
        "aceleracao_linear_valida,"
        "pressao_inicial_pa,altitude_relativa_m,"
        "aceleracao_vertical,aceleracao_vertical_valida,"
        "velocidade_vertical,velocidade_vertical_valida,"
        "lancamento_detectado,"
        "estado_paraquedas");
}

void loop()
{

    // ========================================================
    // LEITURA DOS SENSORES
    // ========================================================

    DadoSensor dado = lerSensores();

    // ========================================================
    // ORIENTACAO / CONVERGENCIA DO DMP
    // ========================================================

    EstadoDMP estadoDMP = atualizarEstadoDMP(dado);

    // ========================================================
    // ACELERACAO LINEAR
    // ========================================================

    calcularAceleracaoLinear(dado, estadoDMP.dmp_pronto);

    // VELOCIDADE VERTICAL

    atualizarVelocidadeVertical(dado);

    atualizarDetectorLancamento(
        detectorLancamento,
        dado.aceleracao_vertical,
        dado.aceleracao_vertical_valida
    );

    // ALTITUDE RELATIVA

    dado.altitude_relativa_m = calcularAltitudeRelativa(dado.pressao_pa, calibracao.pressaoInicial_pa);

    // Passando no detector de apogeu

    if (detectorLancamento.lancamentoDetectado) {
        atualizarDetectorApogeu(detectorApogeu, dado.altitude_relativa_m);
    }

    paraquedas(controleParaquedas, detectorApogeu.apogeuDetectado);

    if (controleParaquedas.estado != ultimoEstado)
    {
        switch (controleParaquedas.estado)
        {
        case EstadoParaquedas::ACIONADO:
            servoParaquedas.write(ANGULO_ACIONAMENTO);
            break;
        case EstadoParaquedas::FINALIZADO:
            servoParaquedas.write(ANGULO_INICIAL);
            break;
        default:
            break;
        }

        ultimoEstado = controleParaquedas.estado;
    }

    // ========================================================
    // TEMPO RELATIVO DO EXPERIMENTO
    // ========================================================

    uint32_t timestampMs = millis() - inicioTesteMs;

    // ========================================================
    // CSV
    // ========================================================

    Serial.print(timestampMs);
    Serial.print(",");

    Serial.print(dado.pressao_pa, 2);
    Serial.print(",");

    Serial.print(dado.temperatura_c, 2);
    Serial.print(",");

    Serial.print(dado.altitude_m, 3);
    Serial.print(",");

    // ACELEROMETRO

    Serial.print(dado.ax, 5);
    Serial.print(",");

    Serial.print(dado.ay, 5);
    Serial.print(",");

    Serial.print(dado.az, 5);
    Serial.print(",");

    // GIROSCOPIO

    Serial.print(dado.gx, 6);
    Serial.print(",");

    Serial.print(dado.gy, 6);
    Serial.print(",");

    Serial.print(dado.gz, 6);
    Serial.print(",");

    // MAGNETOMETRO

    Serial.print(dado.mx, 3);
    Serial.print(",");

    Serial.print(dado.my, 3);
    Serial.print(",");

    Serial.print(dado.mz, 3);
    Serial.print(",");

    // QUATERNION

    Serial.print(dado.q0, 7);
    Serial.print(",");

    Serial.print(dado.q1, 7);
    Serial.print(",");

    Serial.print(dado.q2, 7);
    Serial.print(",");

    Serial.print(dado.q3, 7);
    Serial.print(",");

    // DIAGNOSTICO DO QUATERNION

    Serial.print(estadoDMP.norma_q, 7);
    Serial.print(",");

    Serial.print(
        dado.quaternion_valido ? 1 : 0);
    Serial.print(",");

    // ORIENTACAO

    Serial.print(
        estadoDMP.roll_deg,
        4);
    Serial.print(",");

    Serial.print(
        estadoDMP.pitch_deg,
        4);
    Serial.print(",");

    // CONVERGENCIA

    Serial.print(
        estadoDMP.variacao_roll_deg,
        4);
    Serial.print(",");

    Serial.print(
        estadoDMP.variacao_pitch_deg,
        4);
    Serial.print(",");

    Serial.print(
        estadoDMP.janela_estabilidade ? 1 : 0);
    Serial.print(",");

    Serial.print(
        estadoDMP.dmp_pronto ? 1 : 0);
    Serial.print(",");

    // ========================================================
    // ACELERACAO LINEAR
    // ========================================================

    Serial.print(dado.ax_linear, 5);
    Serial.print(",");

    Serial.print(dado.ay_linear, 5);
    Serial.print(",");

    Serial.print(dado.az_linear, 5);
    Serial.print(",");

    Serial.print(
        dado.aceleracao_linear_valida ? 1 : 0);
    Serial.print(",");

    Serial.print(
        calibracao.pressaoInicial_pa,
        2);
    Serial.print(",");

    Serial.print(
        dado.altitude_relativa_m,
        3);
    Serial.print(",");

    // ACELERACAO

    Serial.print(dado.aceleracao_vertical, 5);
    Serial.print(",");

    Serial.print(
        dado.aceleracao_vertical_valida ? 1 : 0);
    Serial.print(",");

    // VELOCIDADE

    Serial.print(dado.velocidade_vertical, 5);
    Serial.print(",");

    Serial.print(
        dado.velocidade_vertical_valida ? 1 : 0);
    Serial.print(",");

    Serial.print(detectorLancamento.lancamentoDetectado ? 1 : 0);
        Serial.print(",");

    // PARAQUEDAS

    Serial.println(
        static_cast<int>(controleParaquedas.estado));

    // Aproximadamente 50 Hz.
    delay(20);
};