#include "movimento.h"

// ============================================================
// CONSTANTES
// ============================================================

// Aceleracao gravitacional de referencia.
static constexpr float GRAVIDADE = 9.80665f;

// Bias do acelerometro no eixo X,
// obtido experimentalmente nos testes anteriores.
static constexpr float BIAS_ACCEL_X = -0.146f;


// ============================================================
// VARIAVEIS INTERNAS PARA A VELOCIDADE VERTICAL
// ============================================================

// Precisamos guardar a velocidade da amostra anterior,
// porque a velocidade depende do historico do movimento.
static float velocidadeVertical = 0.0f;

// Timestamp da ultima amostra valida utilizada na integracao.
static uint32_t timestampAnteriorUs = 0;


// ============================================================
// INICIALIZACAO DO MOVIMENTO
// ============================================================

void iniciarMovimento() {

    velocidadeVertical = 0.0f;
    timestampAnteriorUs = 0;
}


// ============================================================
// ACELERACAO LINEAR E ACELERACAO VERTICAL
// ============================================================

void calcularAceleracaoLinear(
    DadoSensor& dado,
    bool dmpPronto
) {

    // Toda nova amostra comeca como invalida.
    dado.aceleracao_linear_valida = false;
    dado.aceleracao_vertical_valida = false;

    dado.ax_linear = 0.0f;
    dado.ay_linear = 0.0f;
    dado.az_linear = 0.0f;

    dado.aceleracao_vertical = 0.0f;


    // ========================================================
    // VERIFICACAO DO DMP
    // ========================================================

    // Nao podemos remover a gravidade enquanto:
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
    // VETOR DA GRAVIDADE
    // ========================================================
    //
    // O quaternion informa a orientacao do sensor.
    //
    // A partir dele calculamos como a gravidade aparece
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


    // Neste ponto temos apenas a direcao da gravidade.
    // Transformamos o vetor unitario em m/s².

    gravidadeX *= GRAVIDADE;
    gravidadeY *= GRAVIDADE;
    gravidadeZ *= GRAVIDADE;


    // ========================================================
    // ACELERACAO LINEAR
    // ========================================================

    // Subtraimos a gravidade da aceleracao medida.

    dado.ax_linear =
        axCorrigido - gravidadeX;

    dado.ay_linear =
        ayCorrigido - gravidadeY;

    dado.az_linear =
        azCorrigido - gravidadeZ;

    dado.aceleracao_linear_valida = true;


    // ========================================================
    // ACELERACAO VERTICAL
    // ========================================================
    //
    // O vetor da gravidade aponta para baixo.
    //
    // Como definimos:
    //
    // positivo = para cima
    // negativo = para baixo
    //
    // usamos o sinal negativo na projecao.

    float direcaoGravidadeX =
        gravidadeX / GRAVIDADE;

    float direcaoGravidadeY =
        gravidadeY / GRAVIDADE;

    float direcaoGravidadeZ =
        gravidadeZ / GRAVIDADE;


    dado.aceleracao_vertical =
        -(
            dado.ax_linear * direcaoGravidadeX +
            dado.ay_linear * direcaoGravidadeY +
            dado.az_linear * direcaoGravidadeZ
        );

    dado.aceleracao_vertical_valida = true;
}


// ============================================================
// VELOCIDADE VERTICAL
// ============================================================

void atualizarVelocidadeVertical(
    DadoSensor& dado
) {

    dado.velocidade_vertical = 0.0f;
    dado.velocidade_vertical_valida = false;


    // So podemos integrar quando a aceleracao vertical
    // ja estiver disponivel.
    if (!dado.aceleracao_vertical_valida) {
        return;
    }


    // ========================================================
    // PRIMEIRA AMOSTRA VALIDA
    // ========================================================

    // Na primeira amostra nao existe delta de tempo anterior.
    // Apenas guardamos o timestamp e iniciamos a velocidade em zero.

    if (timestampAnteriorUs == 0) {

        timestampAnteriorUs =
            dado.timestamp_us;

        velocidadeVertical = 0.0f;

        dado.velocidade_vertical =
            velocidadeVertical;

        dado.velocidade_vertical_valida =
            true;

        return;
    }


    // ========================================================
    // DELTA DE TEMPO
    // ========================================================

    uint32_t deltaUs =
        dado.timestamp_us -
        timestampAnteriorUs;

    timestampAnteriorUs =
        dado.timestamp_us;


    float deltaT =
        deltaUs / 1000000.0f;


    // ========================================================
    // PROTECAO
    // ========================================================

    // Evita integrar intervalos invalidos ou pausas grandes
    // causadas por travamentos ou interrupcoes.

    if (
        deltaT <= 0.0f ||
        deltaT > 0.5f
    ) {
        return;
    }


    // ========================================================
    // INTEGRACAO
    // ========================================================
    //
    // v(k) = v(k-1) + a * dt

    velocidadeVertical +=
        dado.aceleracao_vertical *
        deltaT;


    dado.velocidade_vertical =
        velocidadeVertical;

    dado.velocidade_vertical_valida =
        true;
}