#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

#include "sensores.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "detector_lancamento.h"
#include "filtro_kalman.h"
#include "armazenamento.h"
#include "rede_wifi.h"

static uint32_t inicioTesteMs = 0;
static float pressaoInicial = NAN;
static bool pressaoPronta = false;

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
static uint8_t tentativasSalvamento = 0;
static uint32_t ultimaTentativaMs = 0;

constexpr uint8_t MAX_TENTATIVAS = 3;
constexpr uint32_t INTERVALO_TENTATIVAS_MS = 2000;

static const int ANGULO_INICIAL = 180;
static const int ANGULO_ACIONAMENTO = 60;

static EstadoParaquedas ultimoEstado =
    EstadoParaquedas::AGUARDANDO;

const char *CABECALHO_CSV =
    "timestamp_ms,"
    "pressao_pa,"
    "temperatura_c,"
    "aceleracao_total_m_s2,"
    "lancamento_detectado,"
    "altitude_relativa_bruta_m,"
    "altitude_relativa_filtrada_m";

void setup()
{
    Serial.begin(115200);
    delay(8000);

    // ========================================================
    // INICIALIZACAO DOS SENSORES
    // ========================================================

    if (!iniciarSensores())
    {
        Serial.println("Erro ao iniciar BMP280 ou ICM20948.");

        while (true)
        {
            delay(1000);
        }
    }

    // ========================================================
    // FILTRO DA ALTITUDE BAROMETRICA
    // ========================================================

    filtroAltitude = inicializarFiltroKalman(
        0.0f,
        1.0f,
        0.01f,
        0.25f);

    // ========================================================
    // DETECTORES E PARAQUEDAS
    // ========================================================

    detectorApogeu =
        inicializarDetectorApogeu();

    detectorLancamento =
        inicializarDetectorLancamento();

    controleParaquedas =
        inicializarControleParaquedas();

    servoParaquedas.attach(10);
    servoParaquedas.write(ANGULO_INICIAL);

    // ========================================================
    // ARMAZENAMENTO
    // ========================================================

    if (!iniciarArmazenamento())
    {
        Serial.println("Erro ao iniciar armazenamento!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(CABECALHO_CSV);

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
    // WI-FI
    // ========================================================

    if (!iniciarWiFi())
    {
        Serial.println(
            "Wi-Fi indisponivel. Gravacao local continua.");
    }

    linhaCSV.reserve(256);
    inicioTesteMs = millis();
}

void loop()
{
    uint32_t inicioCicloMs = millis();

    // ========================================================
    // LEITURA DOS SENSORES
    // ========================================================

    DadoSensor dado{};
    DadoEstimado estimado{};

    bool leituraBarometricaValida =
        lerSensores(dado);

    // A referencia de pressao e coletada antes de iniciar
    // o reconhecimento do voo.
    if (!pressaoPronta)
    {
        pressaoPronta =
            atualizarPressaoInicial(pressaoInicial);

        atualizarWiFi();

        delay(INTERVALO_AMOSTRAGEM_MS);

        inicioTesteMs = millis();
        return;
    }

    // ========================================================
    // ICM20948: SOMENTE DETECCAO DO LANCAMENTO
    // ========================================================

    float aceleracaoTotal =
        obterAceleracaoTotal(dado);

    bool aceleracaoValida =
        isfinite(aceleracaoTotal);

    // ========================================================
    // BMP280: ALTITUDE RELATIVA
    // ========================================================

    bool altitudeValida =
        leituraBarometricaValida &&
        isfinite(dado.pressao) &&
        dado.pressao > 0.0f &&
        isfinite(pressaoInicial) &&
        pressaoInicial > 0.0f;

    estimado.altitude_relativa = NAN;

    if (altitudeValida)
    {
        estimado.altitude_relativa =
            obterAlturaRelativa(
                pressaoInicial,
                dado.pressao);

        altitudeValida =
            isfinite(estimado.altitude_relativa);
    }

    // ========================================================
    // FILTRO DA ALTITUDE
    // ========================================================

    if (altitudeValida)
    {
        atualizarFiltroKalman(
            filtroAltitude,
            estimado.altitude_relativa);
    }

    altitudeValida =
        altitudeValida &&
        isfinite(filtroAltitude.estimativa);

    // ========================================================
    // DETECCAO DO LANCAMENTO: ICM OU BMP
    // ========================================================

    if (icmDisponivel)
    {
        atualizarDetectorLancamento(
            detectorLancamento,
            aceleracaoTotal,
            aceleracaoValida);
    }
    else
    {
        atualizarDetectorLancamentoBMP(
            detectorLancamento,
            estimado.altitude_relativa,
            altitudeValida);
    }

    // ========================================================
    // CONFIRMACAO DO LANCAMENTO
    // ========================================================

    if (detectorLancamento.lancamentoDetectado &&
        !contagemRegistroIniciada)
    {
        instanteLancamentoMs = millis();
        contagemRegistroIniciada = true;

        Serial.println(icmDisponivel ? "Lancamento detectado pelo ICM20948." : "Subida reconhecida pelo BMP280.");

        if (!confirmarLancamentoArmazenamento())
        {
            Serial.println(
                "Erro ao confirmar lancamento no armazenamento!");

            if (!gravacaoEmAndamento())
            {
                estadoGravacao =
                    EstadoGravacao::FINALIZADA;
            }
        }
    }

    // ========================================================
    // APOGEU: SOMENTE PELA ALTITUDE BAROMETRICA
    // ========================================================

    if (detectorLancamento.lancamentoDetectado &&
        altitudeValida)
    {
        atualizarDetectorApogeu(
            detectorApogeu,
            filtroAltitude.estimativa);
    }

    // ========================================================
    // PARAQUEDAS
    // ========================================================

    paraquedas(
        controleParaquedas,
        detectorApogeu.apogeuDetectado);

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
    // CSV
    // ========================================================

    uint32_t timestampMs =
        millis() - inicioTesteMs;

    linhaCSV = String(timestampMs) + ",";
    linhaCSV += String(dado.pressao, 2) + ",";
    linhaCSV += String(dado.temp, 2) + ",";
    linhaCSV += String(aceleracaoTotal, 4) + ",";
    linhaCSV += String(
        detectorLancamento.lancamentoDetectado ? 1 : 0) + ",";
    linhaCSV += String(
        estimado.altitude_relativa,
        3) + ",";
    linhaCSV += String(
        altitudeValida
            ? filtroAltitude.estimativa
            : NAN,
        3);

    Serial.println(linhaCSV);

    // ========================================================
    // ARMAZENAMENTO DO VOO
    // ========================================================

    if (estadoGravacao == EstadoGravacao::GRAVANDO)
    {
        AmostraVoo amostra;

        amostra.timestamp_ms = timestampMs;
        amostra.pressao_pa = dado.pressao;
        amostra.temperatura_c = dado.temp;
        amostra.aceleracao_total_m_s2 =
            aceleracaoTotal;
        amostra.lancamento_detectado =
            detectorLancamento.lancamentoDetectado;
        amostra.altitude_relativa_bruta_m =
            estimado.altitude_relativa;
        amostra.altitude_relativa_filtrada_m =
            altitudeValida
                ? filtroAltitude.estimativa
                : NAN;

        bool amostraRegistrada =
            registrarAmostra(amostra);

        bool tempoEncerrado =
            contagemRegistroIniciada &&
            millis() - instanteLancamentoMs >=
                TEMPO_POS_LANCAMENTO_MS;

        if (!amostraRegistrada ||
            tempoEncerrado)
        {
            estadoGravacao =
                EstadoGravacao::FINALIZADA;

            if (!amostraRegistrada)
            {
                Serial.println(
                    "Coleta interrompida: falha ao guardar amostra.");
            }
            else
            {
                Serial.println(
                    "Tempo de coleta encerrado.");
            }

            if (finalizarArmazenamento())
            {
                Serial.println(
                    "CSV salvo e disponivel para download.");
            }
            else
            {
                Serial.println(
                    "CSV nao foi salvo. Dados permanecem na RAM.");
                ultimaTentativaMs = millis();
            }
        }
    }

    // ========================================================
    // NOVAS TENTATIVAS AUTOMATICAS DE SALVAMENTO
    // ========================================================

    if (estadoGravacao == EstadoGravacao::FINALIZADA &&
        obterEstadoArmazenamento() == EstadoArmazenamento::ERRO_AO_SALVAR &&
        tentativasSalvamento < MAX_TENTATIVAS &&
        millis() - ultimaTentativaMs >= INTERVALO_TENTATIVAS_MS)
    {
        tentativasSalvamento++;
        ultimaTentativaMs = millis();

        Serial.print("Tentativa adicional de salvamento: ");
        Serial.println(tentativasSalvamento);

        if (tentarSalvarNovamente())
        {
            Serial.println("CSV salvo com sucesso.");
        }
        else if (tentativasSalvamento >= MAX_TENTATIVAS)
        {
            Serial.println(
                "Tentativas automaticas esgotadas. Dados permanecem na RAM.");
        }
    }

    // ========================================================
    // WI-FI
    // ========================================================

    if (!contagemRegistroIniciada ||
        estadoGravacao == EstadoGravacao::FINALIZADA)
    {
        atualizarWiFi();
    }

    // ========================================================
    // CONTROLE DA TAXA DE AMOSTRAGEM
    // ========================================================

    uint32_t duracaoCicloMs =
        millis() - inicioCicloMs;

    if (duracaoCicloMs < INTERVALO_AMOSTRAGEM_MS)
    {
        delay(
            INTERVALO_AMOSTRAGEM_MS -
            duracaoCicloMs);
    }
}
