#include <Arduino.h>
#include <ESP32Servo.h>

#include "sensores.h"
#include "orientacao.h"
#include "movimento.h"
#include "calibracao.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "detector_lancamento.h"
#include "filtro_kalman.h"
#include "armazenamento.h"

// Momento em que o experimento realmente começa.
// Assim o timestamp do CSV começa próximo de zero,
// independentemente do tempo gasto no setup.
static uint32_t inicioTesteMs = 0;
static Calibracao calibracao;
static DetectorApogeu detectorApogeu;
static ControleParaquedas controleParaquedas;
static Servo servoParaquedas;
static DetectorLancamento detectorLancamento;
static FiltroKalmanAltitude filtroAltitude;
static String linhaCSV;

enum class EstadoGravacao {
    AGUARDANDO,
    GRAVANDO,
    FINALIZADA
};

static EstadoGravacao estadoGravacao = EstadoGravacao::AGUARDANDO;
static const uint32_t TEMPO_REGISTRO_APOS_LANCAMENTO_MS = 40000;
static uint32_t instanteLancamentoMs = 0;
static bool contagemRegistroIniciada = false;

static const int ANGULO_INICIAL = 180;
static const int ANGULO_ACIONAMENTO = 60;
static EstadoParaquedas ultimoEstado = EstadoParaquedas::AGUARDANDO;

const char *CABECALHO_CSV =
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
    "altitude_filtrada_m,"
    "aceleracao_vertical,aceleracao_vertical_valida,"
    "velocidade_vertical,velocidade_vertical_valida,"
    "lancamento_detectado,"
    "estado_paraquedas";

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

    // Filtro de kalman

    filtroAltitude = inicializarFiltroKalman(
        0.0f,  // estimativa inicial (m)
        1.0f,  // incerteza inicial (m²)
        0.01f, // ruidoProcesso (m²)
        0.25f  // ruidoMedicao (m²)
    );

    // INICIALIZACAO DO MOVIMENTO

    iniciarMovimento();

    // ========================================================
    // INICIO EFETIVO DO EXPERIMENTO
    // ========================================================

    detectorApogeu = inicializarDetectorApogeu();

    controleParaquedas = inicializarControleParaquedas();

    detectorLancamento = inicializarDetectorLancamento();

    // configuração do servo
    servoParaquedas.attach(10);
    servoParaquedas.write(ANGULO_INICIAL);

    if (!iniciarArmazenamento())
    {
        Serial.println("Erro ao iniciar armazenamento!");

        while (true)
        {
            delay(1000);
        }
    }

    // ========================================================
    // CABECALHO CSV
    // ========================================================

    Serial.println(CABECALHO_CSV);

    if (!criarArquivoCSV("/experimento.csv", CABECALHO_CSV))
    {
        Serial.println("Erro ao criar arquivo CSV!");

        while (true)
        {
            delay(1000);
        }
    }

    linhaCSV.reserve(512);

    inicioTesteMs = millis();
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
        dado.aceleracao_vertical_valida);

    if (detectorLancamento.lancamentoDetectado &&
        !contagemRegistroIniciada)
    {
        instanteLancamentoMs = millis();
        contagemRegistroIniciada = true;
    }

    // ALTITUDE RELATIVA

    dado.altitude_relativa_m = calcularAltitudeRelativa(dado.pressao_pa, calibracao.pressaoInicial_pa);

    // Aplicação do Filtro

    atualizarFiltroKalman(
        filtroAltitude,
        dado.altitude_relativa_m);

    // Passando no detector de apogeu

    if (detectorLancamento.lancamentoDetectado)
    {
        atualizarDetectorApogeu(
            detectorApogeu,
            filtroAltitude.estimativa);
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

    linhaCSV = "";

    linhaCSV = String(timestampMs) + ",";
    linhaCSV += String(dado.pressao_pa, 2) + ",";
    linhaCSV += String(dado.temperatura_c, 2) + ",";
    linhaCSV += String(dado.altitude_m, 3) + ",";

    // ACELEROMETRO
    linhaCSV += String(dado.ax, 5) + ",";
    linhaCSV += String(dado.ay, 5) + ",";
    linhaCSV += String(dado.az, 5) + ",";

    // GIROSCOPIO
    linhaCSV += String(dado.gx, 6) + ",";
    linhaCSV += String(dado.gy, 6) + ",";
    linhaCSV += String(dado.gz, 6) + ",";

    // MAGNETOMETRO
    linhaCSV += String(dado.mx, 3) + ",";
    linhaCSV += String(dado.my, 3) + ",";
    linhaCSV += String(dado.mz, 3) + ",";

    // QUATERNION
    linhaCSV += String(dado.q0, 7) + ",";
    linhaCSV += String(dado.q1, 7) + ",";
    linhaCSV += String(dado.q2, 7) + ",";
    linhaCSV += String(dado.q3, 7) + ",";

    // DIAGNOSTICO DO QUATERNION
    linhaCSV += String(estadoDMP.norma_q, 7) + ",";
    linhaCSV += String(dado.quaternion_valido ? 1 : 0) + ",";

    // ORIENTACAO
    linhaCSV += String(estadoDMP.roll_deg, 4) + ",";
    linhaCSV += String(estadoDMP.pitch_deg, 4) + ",";

    // CONVERGENCIA
    linhaCSV += String(estadoDMP.variacao_roll_deg, 4) + ",";
    linhaCSV += String(estadoDMP.variacao_pitch_deg, 4) + ",";
    linhaCSV += String(estadoDMP.janela_estabilidade ? 1 : 0) + ",";
    linhaCSV += String(estadoDMP.dmp_pronto ? 1 : 0) + ",";

    // ACELERACAO LINEAR
    linhaCSV += String(dado.ax_linear, 5) + ",";
    linhaCSV += String(dado.ay_linear, 5) + ",";
    linhaCSV += String(dado.az_linear, 5) + ",";
    linhaCSV += String(dado.aceleracao_linear_valida ? 1 : 0) + ",";

    // CALIBRACAO E ALTITUDE
    linhaCSV += String(calibracao.pressaoInicial_pa, 2) + ",";
    linhaCSV += String(dado.altitude_relativa_m, 3) + ",";
    linhaCSV += String(filtroAltitude.estimativa, 3) + ",";

    // ACELERACAO VERTICAL
    linhaCSV += String(dado.aceleracao_vertical, 5) + ",";
    linhaCSV += String(dado.aceleracao_vertical_valida ? 1 : 0) + ",";

    // VELOCIDADE VERTICAL
    linhaCSV += String(dado.velocidade_vertical, 5) + ",";
    linhaCSV += String(dado.velocidade_vertical_valida ? 1 : 0) + ",";

    // DETECCAO DE LANCAMENTO
    linhaCSV += String(detectorLancamento.lancamentoDetectado ? 1 : 0) + ",";

    // ESTADO DO PARAQUEDAS
    linhaCSV += String(static_cast<int>(controleParaquedas.estado));

    Serial.println(linhaCSV);

    if (contagemRegistroIniciada &&
    millis() - instanteLancamentoMs >= TEMPO_REGISTRO_APOS_LANCAMENTO_MS)
    {
        if (!registrarDado(linhaCSV))
        {
            Serial.println("Erro ao registrar dado no armazenamento!");
        }

        if (controleParaquedas.estado == EstadoParaquedas::FINALIZADO)
        {
            finalizarArmazenamento();
            gravacaoFinalizada = true;

            Serial.println("Gravacao encerrada.");
        }
    }

    // Aproximadamente 50 Hz.
    delay(20);
};