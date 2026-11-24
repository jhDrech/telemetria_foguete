#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>

#include "sensores.h"
// #include "orientacao.h"
// #include "movimento.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "detector_lancamento.h"
#include "filtro_kalman.h"
#include "armazenamento.h"
#include "rede_wifi.h"
#include "simulacao.h"

// Momento em que o experimento realmente começa.
// Assim o timestamp do CSV começa próximo de zero,
// independentemente do tempo gasto no setup.
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
static EstadoParaquedas ultimoEstado = EstadoParaquedas::AGUARDANDO;

const char *CABECALHO_CSV =
"timestamp_ms,"
"pressao,"
"temp,"
"ax,ay,az,"
"gx,gy,gz,"
"altitude_sem_kalman_m,"
"altitude_com_kalman_m";

void setup()
{

    Serial.begin(115200);

    // Mantemos o atraso que já estava sendo usado
    // para estabilização da comunicação USB.
    delay(8000);

    // ========================================================
    // INICIALIZACAO DOS SENSORES
    // ========================================================
#ifdef MODO_HIL
    iniciarHIL();
#else
    if (!iniciarSensores())
    {

        while (true)
        {
            delay(1000);
        }
    }
#endif
    // A referencia barometrica sera coletada sem bloquear no loop,
    // usando atualizarPressaoInicial() da implementacao do Joao.

    // ========================================================
    // INICIALIZACAO DA ORIENTACAO / DMP
    // ========================================================

    // iniciarOrientacao();

    // Filtro de kalman

    filtroAltitude = inicializarFiltroKalman(
        0.0f,  // estimativa inicial (m)
        1.0f,  // incerteza inicial (m²)
        0.01f, // ruidoProcesso (m²)
        0.06068f  // ruidoMedicao (m²)
    );

    // INICIALIZACAO DO MOVIMENTO

    // iniciarMovimento();

    // ========================================================
    // INICIO EFETIVO DO EXPERIMENTO
    // ========================================================

    detectorApogeu = inicializarDetectorApogeu();

    controleParaquedas = inicializarControleParaquedas();

    detectorLancamento = inicializarDetectorLancamento();
#ifndef MODO_HIL
    // configuração do servo
    servoParaquedas.attach(10);
    servoParaquedas.write(ANGULO_INICIAL);
#endif

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
#ifdef MODO_HIL
    Serial.println("# HIL_READY");
#endif
}

void loop()
{
    uint32_t inicioCicloMs = millis();
    // ========================================================
    // LEITURA DOS SENSORES
    // ========================================================

    DadoSensor dado{};
#ifdef MODO_HIL
        bool leituraValida = lerDadosHIL(dado);
#else
        bool leituraValida = lerSensores(dado);
#endif

#ifdef MODO_HIL
    if(!leituraValida){
        atualizarWiFi();
        return;
    }
#endif

    DadoEstimado estimado{};

    if (!pressaoPronta)
    {
    #ifdef MODO_HIL
        pressaoPronta = atualizarPressaoInicialHIL(dado.pressao, pressaoInicial);
    #else
        pressaoPronta = atualizarPressaoInicial(pressaoInicial);
    #endif
        // Mantem o servidor acessivel enquanto a referencia e calculada.
        atualizarWiFi();
    #ifndef MODO_HIL
        delay(INTERVALO_AMOSTRAGEM_MS);
    #endif
        // O tempo do experimento comeca depois da referencia estar pronta.
        inicioTesteMs = millis();
        return;
    }

    if (!definirPressaoReferencia(pressaoInicial)){
        Serial.println("Erro ao definir pressão de referência!");
        return;
    }


    // ALTITUDE RELATIVA

    bool altitudeValida =
        leituraValida &&
        dado.pressao &&
        dado.pressao > 0.0f &&
        pressaoInicial &&
        pressaoInicial > 0.0f;

    estimado.altitude_relativa = NAN;

    if (altitudeValida)
    {
        estimado.altitude_relativa = obterAlturaRelativa(
            pressaoInicial,
            dado.pressao);

        altitudeValida = isfinite(estimado.altitude_relativa);
    }
    // Aplicação do Filtro

    if (altitudeValida)
    {
        atualizarFiltroKalman( filtroAltitude, estimado.altitude_relativa);
    }

#ifdef MODO_HIL
    if(altitudeValida){
        Serial.print("# HIL_FILTRO,");
        Serial.print(dado.timestamp_ms);
        Serial.print(",");
        Serial.print(estimado.altitude_relativa, 5);
        Serial.print(",");
        Serial.println(filtroAltitude.estimativa, 5);
    }
#endif

    // altitudeValida =
    //     altitudeValida && isfinite(filtroAltitude.estimativa);

    // DETECCAO DO LANCAMENTO

    float aceleracaoTotal = obterAceleracaoTotal(dado);
    bool aceleracaoValida = isfinite(aceleracaoTotal);

    if (icmDisponivel) {

        atualizarDetectorLancamento(
            detectorLancamento,
            aceleracaoTotal,
            aceleracaoValida);
    }
    else {

        atualizarDetectorLancamentoBMP(
            detectorLancamento,
            estimado.altitude_relativa,
            altitudeValida);
    }

    // INICIO DA CONTAGEM E CONFIRMACAO DO LANCAMENTO

    if (detectorLancamento.lancamentoDetectado &&
        !contagemRegistroIniciada)
    {
    #ifdef MODO_HIL
        Serial.println("#HIL: LANCAMENTO DETECTADO");
    #endif
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
        atualizarDetectorApogeu(detectorApogeu, filtroAltitude.estimativa);
    #ifdef MODO_HIL
        static bool apogeuNotificado = false;
           
        if (detectorApogeu.apogeuDetectado && !apogeuNotificado) {
            Serial.println("# HIL: APOGEU DETECTADO");
            Serial.printf("# HIL: APOGEU EM %lu ms\n", (unsigned long)millis());
            apogeuNotificado = true;
        }
    #endif
    }

    paraquedas(controleParaquedas, detectorApogeu.apogeuDetectado);

    if (controleParaquedas.estado != ultimoEstado)
    {
        switch (controleParaquedas.estado)
        {
        case EstadoParaquedas::ACIONADO:
        #ifdef MODO_HIL
            Serial.println("# HIL: PARAQUEDAS ACIONADO (VIRTUAL)");
        #endif
        #ifndef MODO_HIL
            servoParaquedas.write(ANGULO_ACIONAMENTO);
        #endif
            break;
        case EstadoParaquedas::FINALIZADO:
        #ifndef MODO_HIL
            servoParaquedas.write(ANGULO_INICIAL);
        #endif
            break;
        default:
            break;
        }

        ultimoEstado = controleParaquedas.estado;
    }

    // ========================================================
    // TEMPO RELATIVO DO EXPERIMENTO
    // ========================================================

#ifdef MODO_HIL
    uint32_t timestampMs = dado.timestamp_ms;
#else
    uint32_t timestampMs = millis() - inicioTesteMs;
#endif

    // CSV dos dados brutos do sensor
    linhaCSV = String(timestampMs) + ",";
    linhaCSV += String(dado.pressao, 2) + ",";
    linhaCSV += String(dado.temp, 2) + ",";
    linhaCSV += String(dado.ax, 5) + ",";
    linhaCSV += String(dado.ay, 5) + ",";
    linhaCSV += String(dado.az, 5) + ",";
    linhaCSV += String(dado.gx, 6) + ",";
    linhaCSV += String(dado.gy, 6) + ",";
    linhaCSV += String(dado.gz, 6) + ",";

    // ALTITUDE RELATIVA
    linhaCSV += String(estimado.altitude_relativa, 3) + ",";

    // ALTITUDE FILTRADA PELO KALMAN
    linhaCSV += String(filtroAltitude.estimativa, 3);


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
    // linhaCSV += String(pressaoInicial, 2) + ",";
    // linhaCSV += String(estimado.altitude_relativa, 3) + ",";
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
        // Guarda a estrutura de dados brutos lida dos sensores.
        // O timestamp do registro usa o inicio do experimento.
        DadoSensor amostra = dado;
        amostra.timestamp_ms = timestampMs;

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
                ultimaTentativaMs = millis();
            }
        }
    }

    // ========================================================
    // TENTATIVAS AUTOMATICAS DE SALVAMENTO
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
            Serial.println("Tentativas automaticas esgotadas. Dados permanecem na RAM.");
        }
    }

    // Atende o navegador antes do lancamento e apos a coleta.
    if (!contagemRegistroIniciada ||
        estadoGravacao == EstadoGravacao::FINALIZADA)
    {
        atualizarWiFi();
    }

#ifndef MODO_HIL
    // Aguarda somente o tempo restante para completar o intervalo.
    uint32_t duracaoCicloMs = millis() - inicioCicloMs;

    if (duracaoCicloMs < INTERVALO_AMOSTRAGEM_MS)
    {
        delay(INTERVALO_AMOSTRAGEM_MS - duracaoCicloMs);
    }
#endif
};