#include <unity.h>
#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>

#include "sensores.h"
#include "detector_lancamento.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "filtro_kalman.h"

void setUp()
{
    tempoSimuladoMs = 0;

    bmpSimulado::endereco = 0x77;
    bmpSimulado::pressao = 100000.0f;
    bmpSimulado::temperatura = 25.0f;

    icmSimulado::presente = true;
    icmSimulado::pronto = true;
    icmSimulado::leituraOk = true;
}

void tearDown() {}

void sensoresObrigatoriosPrecisamEstarPresentes()
{
    TEST_ASSERT_TRUE(iniciarSensores());

    icmSimulado::presente = false;
    TEST_ASSERT_TRUE(iniciarSensores());

    icmSimulado::presente = true;
    bmpSimulado::endereco = 0;
    TEST_ASSERT_FALSE(iniciarSensores());
}

void lerAcelerometroECalcularModulo()
{
    TEST_ASSERT_TRUE(iniciarSensores());

    DadoSensor dado{};

    TEST_ASSERT_TRUE(lerSensores(dado));

    TEST_ASSERT_FLOAT_WITHIN(
        0.0001f,
        9.80665f,
        dado.ax);

    TEST_ASSERT_FLOAT_WITHIN(
        0.0001f,
        -9.80665f,
        dado.az);

    float aceleracaoTotal =
        obterAceleracaoTotal(dado);

    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        13.8687f,
        aceleracaoTotal);

    icmSimulado::pronto = false;

    DadoSensor semAmostra{};

    TEST_ASSERT_TRUE(
        lerSensores(semAmostra));

    TEST_ASSERT_TRUE(
        std::isnan(semAmostra.ax));

    TEST_ASSERT_TRUE(
        std::isnan(
            obterAceleracaoTotal(
                semAmostra)));
}

void alturaTemSinalERejeitaEntradaInvalida()
{
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f,
        0.0f,
        obterAlturaRelativa(
            100000,
            100000));

    TEST_ASSERT_FLOAT_WITHIN(
        0.05f,
        84.698f,
        obterAlturaRelativa(
            100000,
            99000));

    TEST_ASSERT_TRUE(
        obterAlturaRelativa(
            100000,
            101000) < 0.0f);

    TEST_ASSERT_TRUE(
        std::isnan(
            obterAlturaRelativa(
                0,
                100000)));
}

void detectorBMPReconheceSubida()
{
    auto lancamento = inicializarDetectorLancamento();

    atualizarDetectorLancamentoBMP(lancamento, 6.0f, true);
    atualizarDetectorLancamentoBMP(lancamento, 6.0f, true);
    TEST_ASSERT_FALSE(lancamento.lancamentoDetectado);

    atualizarDetectorLancamentoBMP(lancamento, 0.0f, true);
    TEST_ASSERT_EQUAL_INT(0, lancamento.leiturasAcimaDoLimite);

    for (int i = 0; i < LEITURAS_PARA_DETECTAR_LANCAMENTO; ++i)
    {
        atualizarDetectorLancamentoBMP(lancamento, 6.0f, true);
    }
    TEST_ASSERT_TRUE(lancamento.lancamentoDetectado);
}

void detectorICMExigeLeiturasConsecutivas()
{
    auto lancamento = inicializarDetectorLancamento();

    atualizarDetectorLancamento(lancamento, 20.0f, true);
    atualizarDetectorLancamento(lancamento, 20.0f, true);
    atualizarDetectorLancamento(lancamento, NAN, false);
    TEST_ASSERT_EQUAL_INT(0, lancamento.leiturasAcimaDoLimite);
    TEST_ASSERT_FALSE(lancamento.lancamentoDetectado);
}

void aceleracaoDisparaFluxoBarometrico()
{
    auto lancamento =
        inicializarDetectorLancamento();

    auto apogeu =
        inicializarDetectorApogeu();

    auto paraquedasEstado =
        inicializarControleParaquedas();

    auto filtro =
        inicializarFiltroKalman(
            0.0f,
            1.0f,
            0.01f,
            0.25f);

    // Tres amostras consecutivas acima de 2 g confirmam o lancamento.
    for (int i = 0;
         i < LEITURAS_PARA_DETECTAR_LANCAMENTO;
         ++i)
    {
        atualizarDetectorLancamento(
            lancamento,
            ACELERACAO_MINIMA_LANCAMENTO_M_S2 + 1.0f,
            true);
    }

    TEST_ASSERT_TRUE(
        lancamento.lancamentoDetectado);

    // Depois do lancamento, o restante do fluxo usa altitude.
    for (int i = 0; i < 160; ++i)
    {
        float altura =
            i < 80 ? 20.0f : 10.0f;

        atualizarFiltroKalman(
            filtro,
            altura);

        atualizarDetectorApogeu(
            apogeu,
            filtro.estimativa);

        paraquedas(
            paraquedasEstado,
            apogeu.apogeuDetectado);
    }

    TEST_ASSERT_TRUE(
        apogeu.apogeuDetectado);

    TEST_ASSERT_TRUE(
        paraquedasEstado.aberturaSolicitada);

    TEST_ASSERT_TRUE(
        paraquedasEstado.estado ==
        EstadoParaquedas::FINALIZADO);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(
        sensoresObrigatoriosPrecisamEstarPresentes);

    RUN_TEST(
        lerAcelerometroECalcularModulo);

    RUN_TEST(
        alturaTemSinalERejeitaEntradaInvalida);

    RUN_TEST(detectorBMPReconheceSubida);
    RUN_TEST(detectorICMExigeLeiturasConsecutivas);
    RUN_TEST(
        aceleracaoDisparaFluxoBarometrico);

    return UNITY_END();
}
