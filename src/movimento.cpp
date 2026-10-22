#include "movimento.h"

// Aceleracao gravitacional de referencia.
static constexpr float GRAVIDADE = 9.80665f;

// Bias do acelerometro no eixo X obtido
// experimentalmente nos testes anteriores.
static constexpr float BIAS_ACCEL_X = -0.146f;


void calcularAceleracaoLinear(
    DadoSensor& dado,
    bool dmpPronto
) {

    // Por seguranca, toda nova chamada comeca
    // considerando a aceleracao linear invalida.
    dado.aceleracao_linear_valida = false;

    dado.ax_linear = 0.0f;
    dado.ay_linear = 0.0f;
    dado.az_linear = 0.0f;


    // ========================================================
    // VERIFICACAO
    // ========================================================

    // Ainda nao podemos remover a gravidade se:
    //
    // - nao existe quaternion valido;
    // - o DMP ainda nao terminou a convergencia.

    if (
        !dado.quaternion_valido ||
        !dmpPronto
    ) {
        return;
    }


    // ========================================================
    // CORRECAO DO BIAS DO ACELEROMETRO
    // ========================================================

    float axCorrigido =
        dado.ax - BIAS_ACCEL_X;

    float ayCorrigido =
        dado.ay;

    float azCorrigido =
        dado.az;


    // ========================================================
    // DIRECAO DA GRAVIDADE
    // ========================================================
    //
    // O quaternion indica a orientacao atual do sensor.
    //
    // Com ele calculamos como o vetor da gravidade aparece
    // nos eixos X, Y e Z da placa.

    float gravidadeX =
        2.0f * (
            dado.q1 * dado.q3 -
            dado.q0 * dado.q2
        );

    float gravidadeY =
        2.0f * (
            dado.q0 * dado.q1 +
            dado.q2 * dado.q3
        );

    float gravidadeZ =
        dado.q0 * dado.q0 -
        dado.q1 * dado.q1 -
        dado.q2 * dado.q2 +
        dado.q3 * dado.q3;


    // Até aqui os valores representam a direcao
    // da gravidade como vetor unitario.
    //
    // Agora transformamos para m/s².

    gravidadeX *= GRAVIDADE;
    gravidadeY *= GRAVIDADE;
    gravidadeZ *= GRAVIDADE;


    // ========================================================
    // ACELERACAO LINEAR
    // ========================================================

    dado.ax_linear =
        axCorrigido - gravidadeX;

    dado.ay_linear =
        ayCorrigido - gravidadeY;

    dado.az_linear =
        azCorrigido - gravidadeZ;


    dado.aceleracao_linear_valida = true;
}