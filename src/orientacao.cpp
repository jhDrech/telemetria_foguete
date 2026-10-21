#include "orientacao.h"

#include <Arduino.h>
#include <math.h>

// ============================================================================
// CONFIGURACOES DO CRITERIO DE CONVERGENCIA
// ============================================================================

// Tempo minimo antes de começar a avaliar a estabilidade do DMP.
static constexpr uint32_t TEMPO_MINIMO_DMP_MS = 30000;

// Tempo durante o qual roll e pitch precisam permanecer estaveis.
static constexpr uint32_t JANELA_ESTABILIDADE_MS = 5000;

// Variacao maxima permitida durante a janela.
static constexpr float LIMITE_VARIACAO_GRAUS = 0.10f;


// ============================================================================
// ESTADO INTERNO DO MODULO
// ============================================================================

static uint32_t inicioDmpMs = 0;
static uint32_t inicioJanelaMs = 0;

static bool janelaAtiva = false;
static bool dmpPronto = false;

static float rollMin = 0.0f;
static float rollMax = 0.0f;

static float pitchMin = 0.0f;
static float pitchMax = 0.0f;


// ============================================================================
// INICIALIZACAO
// ============================================================================

void iniciarOrientacao() {

    inicioDmpMs = millis();
    inicioJanelaMs = 0;

    janelaAtiva = false;
    dmpPronto = false;

    rollMin = 0.0f;
    rollMax = 0.0f;

    pitchMin = 0.0f;
    pitchMax = 0.0f;
}


// ============================================================================
// ATUALIZACAO DO ESTADO DO DMP
// ============================================================================

EstadoDMP atualizarEstadoDMP(const DadoSensor& dado) {

    EstadoDMP estado{};

    // ------------------------------------------------------------------------
    // Norma do quaternion
    // ------------------------------------------------------------------------

    estado.norma_q = sqrtf(
        dado.q0 * dado.q0 +
        dado.q1 * dado.q1 +
        dado.q2 * dado.q2 +
        dado.q3 * dado.q3
    );

    // Se ainda nao existe quaternion valido, nao ha orientacao para analisar.
    if (!dado.quaternion_valido) {

        estado.janela_estabilidade = false;
        estado.dmp_pronto = false;

        return estado;
    }

    // ------------------------------------------------------------------------
    // Roll
    // ------------------------------------------------------------------------

    float rollRad = atan2f(
        2.0f * (
            dado.q0 * dado.q1 +
            dado.q2 * dado.q3
        ),
        1.0f - 2.0f * (
            dado.q1 * dado.q1 +
            dado.q2 * dado.q2
        )
    );

    // ------------------------------------------------------------------------
    // Pitch
    // ------------------------------------------------------------------------

    float sinPitch =
        2.0f * (
            dado.q0 * dado.q2 -
            dado.q3 * dado.q1
        );

    // Protecao contra pequenos erros numericos.
    if (sinPitch > 1.0f) {
        sinPitch = 1.0f;
    }

    if (sinPitch < -1.0f) {
        sinPitch = -1.0f;
    }

    float pitchRad = asinf(sinPitch);

    estado.roll_deg =
        rollRad * 180.0f / PI;

    estado.pitch_deg =
        pitchRad * 180.0f / PI;

    // Se o DMP ja foi considerado pronto, nao precisamos repetir
    // todo o processo de convergencia.
    if (dmpPronto) {

        estado.dmp_pronto = true;
        estado.janela_estabilidade = false;

        return estado;
    }

    uint32_t agoraMs = millis();

    // ------------------------------------------------------------------------
    // Tempo minimo de convergencia
    // ------------------------------------------------------------------------

    if (
        agoraMs - inicioDmpMs <
        TEMPO_MINIMO_DMP_MS
    ) {

        estado.janela_estabilidade = false;
        estado.dmp_pronto = false;

        return estado;
    }

    // ------------------------------------------------------------------------
    // Inicio da janela de estabilidade
    // ------------------------------------------------------------------------

    if (!janelaAtiva) {

        janelaAtiva = true;
        inicioJanelaMs = agoraMs;

        rollMin = estado.roll_deg;
        rollMax = estado.roll_deg;

        pitchMin = estado.pitch_deg;
        pitchMax = estado.pitch_deg;
    }

    // ------------------------------------------------------------------------
    // Atualiza os valores minimo e maximo observados
    // ------------------------------------------------------------------------

    if (estado.roll_deg < rollMin) {
        rollMin = estado.roll_deg;
    }

    if (estado.roll_deg > rollMax) {
        rollMax = estado.roll_deg;
    }

    if (estado.pitch_deg < pitchMin) {
        pitchMin = estado.pitch_deg;
    }

    if (estado.pitch_deg > pitchMax) {
        pitchMax = estado.pitch_deg;
    }

    estado.variacao_roll_deg =
        rollMax - rollMin;

    estado.variacao_pitch_deg =
        pitchMax - pitchMin;

    estado.janela_estabilidade = true;

    // ------------------------------------------------------------------------
    // Avaliacao da janela
    // ------------------------------------------------------------------------

    if (
        agoraMs - inicioJanelaMs >=
        JANELA_ESTABILIDADE_MS
    ) {

        bool rollEstavel =
            estado.variacao_roll_deg <=
            LIMITE_VARIACAO_GRAUS;

        bool pitchEstavel =
            estado.variacao_pitch_deg <=
            LIMITE_VARIACAO_GRAUS;

        if (
            rollEstavel &&
            pitchEstavel
        ) {

            dmpPronto = true;

            estado.dmp_pronto = true;
            estado.janela_estabilidade = false;
        }
        else {

            // A janela nao passou.
            // Na proxima atualizacao começa uma nova janela de 5 segundos.
            janelaAtiva = false;

            estado.dmp_pronto = false;
        }
    }
    else {

        estado.dmp_pronto = false;
    }

    return estado;
}