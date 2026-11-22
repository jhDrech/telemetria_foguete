#ifndef DETECTOR_LANCAMENTO_H
#define DETECTOR_LANCAMENTO_H

struct DetectorLancamento
{
    int leiturasAcimaDoLimite;
    bool lancamentoDetectado;
};

// A aceleracao total em repouso fica proxima de 1 g.
// O lancamento e confirmado quando o modulo ultrapassa 2 g
// em cinco amostras validas.
constexpr float GRAVIDADE_M_S2 = 9.80665f;
constexpr float ACELERACAO_MINIMA_LANCAMENTO_M_S2 =
    2.0f * GRAVIDADE_M_S2;

constexpr int LEITURAS_PARA_DETECTAR_LANCAMENTO = 3;

// Valor inicial para reconhecer uma subida pelo BMP280.
constexpr float ALTITUDE_MINIMA_LANCAMENTO_M = 5.0f;

DetectorLancamento inicializarDetectorLancamento();

void atualizarDetectorLancamento(
    DetectorLancamento &detector,
    float aceleracaoTotal,
    bool aceleracaoValida);

void atualizarDetectorLancamentoBMP(
    DetectorLancamento &detector,
    float altitudeRelativa,
    bool altitudeValida);

#endif
