#include <unity.h>
#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <ICM_20948.h>
#include "sensores.h"
#include "detector_lancamento.h"
#include "detector_apogeu.h"
#include "controle_paraquedas.h"
#include "filtro_kalman.h"

void setUp() {
    tempoSimuladoMs = 0;
    bmpSimulado::endereco = 0x77;
    bmpSimulado::pressao = 100000.0f;
    icmSimulado::presente = false;
    icmSimulado::pronto = false;
    icmSimulado::leituraOk = true;
}
void tearDown() {}

void operarSomenteComBMP() {
    TEST_ASSERT_TRUE(iniciarSensores());
    DadoSensor dado{};
    TEST_ASSERT_TRUE(lerSensores(dado));
    TEST_ASSERT_EQUAL_FLOAT(100000.0f, dado.pressao);
    TEST_ASSERT_TRUE(std::isnan(dado.ax));
    TEST_ASSERT_TRUE(std::isnan(dado.gx));
    bmpSimulado::endereco = 0x76;
    TEST_ASSERT_TRUE(iniciarSensores());
    bmpSimulado::endereco = 0;
    TEST_ASSERT_FALSE(iniciarSensores());
    TEST_ASSERT_FALSE(lerSensores(dado));
}

void converterICMSemReaproveitarAmostra() {
    icmSimulado::presente = true;
    icmSimulado::pronto = true;
    TEST_ASSERT_TRUE(iniciarSensores());
    DadoSensor dado{};
    TEST_ASSERT_TRUE(lerSensores(dado));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 9.80665f, dado.ax);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -9.80665f, dado.az);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.14159265f, dado.gx);
    TEST_ASSERT_EQUAL_FLOAT(12.0f, dado.mx);
    icmSimulado::pronto = false;
    TEST_ASSERT_TRUE(lerSensores(dado));
    TEST_ASSERT_TRUE(std::isnan(dado.ax));
    icmSimulado::pronto = true;
    icmSimulado::leituraOk = false;
    TEST_ASSERT_TRUE(lerSensores(dado));
    TEST_ASSERT_TRUE(std::isnan(dado.ax));
}

void referenciaIgnoraPressaoInvalidaEReinicia() {
    TEST_ASSERT_TRUE(iniciarSensores());
    float referencia = NAN;
    TEST_ASSERT_FALSE(atualizarPressaoInicial(referencia));
    tempoSimuladoMs = 1000;
    bmpSimulado::pressao = NAN;
    TEST_ASSERT_FALSE(atualizarPressaoInicial(referencia));
    tempoSimuladoMs = 3000;
    bmpSimulado::pressao = 98000.0f;
    TEST_ASSERT_TRUE(atualizarPressaoInicial(referencia));
    TEST_ASSERT_EQUAL_FLOAT(99000.0f, referencia);
    float outraSaida = NAN;
    TEST_ASSERT_TRUE(atualizarPressaoInicial(outraSaida));
    TEST_ASSERT_EQUAL_FLOAT(referencia, outraSaida);
    TEST_ASSERT_TRUE(iniciarSensores());
    TEST_ASSERT_FALSE(atualizarPressaoInicial(referencia));
    tempoSimuladoMs += 3000;
    TEST_ASSERT_TRUE(atualizarPressaoInicial(referencia));
    TEST_ASSERT_EQUAL_FLOAT(98000.0f, referencia);
}

void alturaTemSinalERejeitaEntradaInvalida() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, obterAlturaRelativa(100000, 100000));
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 84.698f, obterAlturaRelativa(100000, 99000));
    TEST_ASSERT_TRUE(obterAlturaRelativa(100000, 101000) < 0.0f);
    TEST_ASSERT_TRUE(std::isnan(obterAlturaRelativa(0, 100000)));
    TEST_ASSERT_TRUE(std::isnan(obterAlturaRelativa(100000, 0)));
    TEST_ASSERT_TRUE(std::isnan(obterAlturaRelativa(NAN, 100000)));
    TEST_ASSERT_TRUE(std::isnan(obterAlturaRelativa(100000, INFINITY)));
}

void pressaoAlimentaDetectoresEParaquedasSemICM() {
    TEST_ASSERT_TRUE(iniciarSensores());
    float referencia = NAN;
    atualizarPressaoInicial(referencia);
    tempoSimuladoMs = 3000;
    TEST_ASSERT_TRUE(atualizarPressaoInicial(referencia));
    auto lancamento = inicializarDetectorLancamento();
    auto apogeu = inicializarDetectorApogeu();
    auto paraquedasEstado = inicializarControleParaquedas();
    auto filtro = inicializarFiltroKalman(0.0f, 1.0f, 0.01f, 0.25f);
    // Primeiro patamar acima do limiar, depois descida: exercita o fluxo real.
    for (int i = 0; i < 160; ++i) {
        float altura = i < 80 ? 20.0f : 10.0f;
        bmpSimulado::pressao = referencia * powf(1.0f - altura / 44330.0f, 1.0f / 0.1903f);
        DadoSensor dado{};
        TEST_ASSERT_TRUE(lerSensores(dado));
        float h = obterAlturaRelativa(referencia, dado.pressao);
        atualizarFiltroKalman(filtro, h);
        atualizarDetectorLancamento(lancamento, filtro.estimativa, isfinite(h));
        if (lancamento.lancamentoDetectado) {
            atualizarDetectorApogeu(apogeu, filtro.estimativa);
        }
        paraquedas(paraquedasEstado, apogeu.apogeuDetectado);
        tempoSimuladoMs += 20;
    }
    TEST_ASSERT_TRUE(lancamento.lancamentoDetectado);
    TEST_ASSERT_TRUE(apogeu.apogeuDetectado);
    TEST_ASSERT_TRUE(paraquedasEstado.aberturaSolicitada);
    TEST_ASSERT_TRUE(paraquedasEstado.estado == EstadoParaquedas::FINALIZADO);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(operarSomenteComBMP);
    RUN_TEST(converterICMSemReaproveitarAmostra);
    RUN_TEST(referenciaIgnoraPressaoInvalidaEReinicia);
    RUN_TEST(alturaTemSinalERejeitaEntradaInvalida);
    RUN_TEST(pressaoAlimentaDetectoresEParaquedasSemICM);
    return UNITY_END();
}
