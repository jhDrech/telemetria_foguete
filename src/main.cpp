#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

#include "sensores.h"
// #include "orientacao.h"
// #include "movimento.h"
#include "calibracao.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "detector_lancamento.h"
#include "filtro_kalman.h"
#include "armazenamento.h"
#include "rede_wifi.h"

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

enum class EstadoGravacao
{
    AGUARDANDO,
    GRAVANDO,
    FINALIZADA
};

static EstadoGravacao estadoGravacao = EstadoGravacao::AGUARDANDO;
static uint32_t instanteLancamentoMs = 0;
static bool contagemRegistroIniciada = false;
static const int ANGULO_INICIAL = 180;
static const int ANGULO_ACIONAMENTO = 60;
static EstadoParaquedas ultimoEstado = EstadoParaquedas::AGUARDANDO;

const char *CABECALHO_CSV =
    "timestamp_ms,"
    "pressao_pa,"
    "temperatura_c,"
    "altitude_relativa_bruta_m,"
    "altitude_relativa_filtrada_m";

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

    // iniciarOrientacao();

    // Filtro de kalman

    filtroAltitude = inicializarFiltroKalman(
        0.0f,  // estimativa inicial (m)
        1.0f,  // incerteza inicial (m²)
        0.01f, // ruidoProcesso (m²)
        0.25f  // ruidoMedicao (m²)
    );

    // INICIALIZACAO DO MOVIMENTO

    // iniciarMovimento();

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

    // if (!criarArquivoCSV("/experimento.csv", CABECALHO_CSV))
    // {
    //     Serial.println("Erro ao criar arquivo CSV!");

    //     while (true)
    //     {
    //         delay(1000);
    //     }
    // }

    // ========================================================
    // PREPARACAO DO BUFFER EM MEMORIA
    // ========================================================

    if (!criarArquivoNovoVoo(CABECALHO_CSV))
    {
        Serial.println("Erro ao preparar buffer em memoria!");
        while (true)
        {
            delay(1000);
        }
    }

    estadoGravacao = EstadoGravacao::GRAVANDO;

    // ========================================================
    // INICIALIZACAO DO WI-FI
    // ========================================================

    if (!iniciarWiFi())
    {
        Serial.println("Wi-Fi indisponivel. Gravacao local continua.");
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

    // EstadoDMP estadoDMP = atualizarEstadoDMP(dado);

    // ========================================================
    // ACELERACAO LINEAR
    // ========================================================

    // calcularAceleracaoLinear(dado, estadoDMP.dmp_pronto);

    // VELOCIDADE VERTICAL

    // atualizarVelocidadeVertical(dado);

    // atualizarDetectorLancamento(
    //     detectorLancamento,
    //     dado.aceleracao_vertical,
    //     dado.aceleracao_vertical_valida);

    // ALTITUDE RELATIVA

    bool altitudeValida =
        isfinite(dado.pressao_pa) &&
        dado.pressao_pa > 0.0f &&
        isfinite(calibracao.pressaoInicial_pa) &&
        calibracao.pressaoInicial_pa > 0.0f;

    dado.altitude_relativa_m = NAN;

    if (altitudeValida)
    {
        dado.altitude_relativa_m = calcularAltitudeRelativa(
            dado.pressao_pa,
            calibracao.pressaoInicial_pa);

        altitudeValida = isfinite(dado.altitude_relativa_m);
    }
    // Aplicação do Filtro

    if (altitudeValida)
    {
        atualizarFiltroKalman(
            filtroAltitude,
            dado.altitude_relativa_m);
    }

    altitudeValida =
        altitudeValida && isfinite(filtroAltitude.estimativa);

    // DETECCAO DO LANCAMENTO POR ALTITUDE

    atualizarDetectorLancamento(
        detectorLancamento,
        filtroAltitude.estimativa,
        altitudeValida);

    // INICIO DA CONTAGEM E CONFIRMACAO DO LANCAMENTO

    if (detectorLancamento.lancamentoDetectado &&
        !contagemRegistroIniciada)
    {
        instanteLancamentoMs = millis();
        contagemRegistroIniciada = true;

        if (!confirmarLancamentoArmazenamento())
        {
            Serial.println(
                "Erro ao confirmar lancamento no armazenamento!");

            if (!gravacaoEmAndamento())
            {
                estadoGravacao = EstadoGravacao::FINALIZADA;
            }
        }
    }
    // Passando no detector de apogeu

    if (detectorLancamento.lancamentoDetectado && altitudeValida)
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

    // CSV
    linhaCSV = String(timestampMs) + ",";
    linhaCSV += String(dado.pressao_pa, 2) + ",";
    linhaCSV += String(dado.temperatura_c, 2) + ",";
    linhaCSV += String(dado.altitude_relativa_m, 3) + ",";
    linhaCSV += String(
        altitudeValida ? filtroAltitude.estimativa : NAN,
        3);
    // // ACELEROMETRO
    // linhaCSV += String(dado.ax, 5) + ",";
    // linhaCSV += String(dado.ay, 5) + ",";
    // linhaCSV += String(dado.az, 5) + ",";

    // // GIROSCOPIO
    // linhaCSV += String(dado.gx, 6) + ",";
    // linhaCSV += String(dado.gy, 6) + ",";
    // linhaCSV += String(dado.gz, 6) + ",";

    // // MAGNETOMETRO
    // linhaCSV += String(dado.mx, 3) + ",";
    // linhaCSV += String(dado.my, 3) + ",";
    // linhaCSV += String(dado.mz, 3) + ",";

    // // QUATERNION
    // linhaCSV += String(dado.q0, 7) + ",";
    // linhaCSV += String(dado.q1, 7) + ",";
    // linhaCSV += String(dado.q2, 7) + ",";
    // linhaCSV += String(dado.q3, 7) + ",";

    // // DIAGNOSTICO DO QUATERNION
    // linhaCSV += String(estadoDMP.norma_q, 7) + ",";
    // linhaCSV += String(dado.quaternion_valido ? 1 : 0) + ",";

    // // ORIENTACAO
    // linhaCSV += String(estadoDMP.roll_deg, 4) + ",";
    // linhaCSV += String(estadoDMP.pitch_deg, 4) + ",";

    // // CONVERGENCIA
    // linhaCSV += String(estadoDMP.variacao_roll_deg, 4) + ",";
    // linhaCSV += String(estadoDMP.variacao_pitch_deg, 4) + ",";
    // linhaCSV += String(estadoDMP.janela_estabilidade ? 1 : 0) + ",";
    // linhaCSV += String(estadoDMP.dmp_pronto ? 1 : 0) + ",";

    // // ACELERACAO LINEAR
    // linhaCSV += String(dado.ax_linear, 5) + ",";
    // linhaCSV += String(dado.ay_linear, 5) + ",";
    // linhaCSV += String(dado.az_linear, 5) + ",";
    // linhaCSV += String(dado.aceleracao_linear_valida ? 1 : 0) + ",";

    // // CALIBRACAO E ALTITUDE
    // linhaCSV += String(calibracao.pressaoInicial_pa, 2) + ",";
    // linhaCSV += String(dado.altitude_relativa_m, 3) + ",";
    // linhaCSV += String(filtroAltitude.estimativa, 3) + ",";

    // // ACELERACAO VERTICAL
    // linhaCSV += String(dado.aceleracao_vertical, 5) + ",";
    // linhaCSV += String(dado.aceleracao_vertical_valida ? 1 : 0) + ",";

    // // VELOCIDADE VERTICAL
    // linhaCSV += String(dado.velocidade_vertical, 5) + ",";
    // linhaCSV += String(dado.velocidade_vertical_valida ? 1 : 0) + ",";

    // // DETECCAO DE LANCAMENTO
    // linhaCSV += String(detectorLancamento.lancamentoDetectado ? 1 : 0) + ",";

    // // ESTADO DO PARAQUEDAS
    // linhaCSV += String(static_cast<int>(controleParaquedas.estado));

    Serial.println(linhaCSV);
    if (estadoGravacao == EstadoGravacao::GRAVANDO)
    {
        // Monta a amostra que sera guardada no array.
        AmostraVoo amostra;

        amostra.timestamp_ms = timestampMs;
        amostra.pressao_pa = dado.pressao_pa;
        amostra.temperatura_c = dado.temperatura_c;
        amostra.altitude_relativa_bruta_m = dado.altitude_relativa_m;
        amostra.altitude_relativa_filtrada_m =
            altitudeValida ? filtroAltitude.estimativa : NAN;

        bool amostraRegistrada = registrarAmostra(amostra);

        bool tempoEncerrado =
            contagemRegistroIniciada &&
            millis() - instanteLancamentoMs >= TEMPO_POS_LANCAMENTO_MS;

        if (!amostraRegistrada || tempoEncerrado)
        {
            estadoGravacao = EstadoGravacao::FINALIZADA;

            if (!amostraRegistrada)
            {
                Serial.println("Coleta interrompida: falha ao guardar amostra.");
            }
            else
            {
                Serial.println("Tempo de coleta encerrado.");
            }

            if (finalizarArmazenamento())
            {
                Serial.println("CSV salvo e disponivel para download.");
            }
            else
            {
                Serial.println("CSV nao foi salvo. Dados permanecem na RAM.");
            }
        }
    }

    // Atende o navegador antes do lancamento e apos a coleta.
    if (!contagemRegistroIniciada ||
        estadoGravacao == EstadoGravacao::FINALIZADA)
    {
        atualizarWiFi();
    }

    // Aproximadamente 50 Hz.
    delay(INTERVALO_AMOSTRAGEM_MS);
};