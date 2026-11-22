#include "detector_lancamento.h"

DetectorLancamento inicializarDetectorLancamento()
{
    DetectorLancamento detector;

    detector.leiturasAcimaDoLimite = 0;
    detector.lancamentoDetectado = false;

    return detector;
}

void atualizarDetectorLancamento(
    DetectorLancamento &detector,
    float aceleracaoTotal,
    bool aceleracaoValida)
{
    if (detector.lancamentoDetectado)
    {
        return;
    }

    if (aceleracaoValida &&
        aceleracaoTotal >
            ACELERACAO_MINIMA_LANCAMENTO_M_S2)
    {
        detector.leiturasAcimaDoLimite++;
    }
    else
    {
        detector.leiturasAcimaDoLimite = 0;
    }

    if (detector.leiturasAcimaDoLimite >=
        LEITURAS_PARA_DETECTAR_LANCAMENTO)
    {
        detector.lancamentoDetectado = true;
    }
}

void atualizarDetectorLancamentoBMP(DetectorLancamento &detector, float altitudeRelativa, bool altitudeValida)
{
    if (detector.lancamentoDetectado) {
        return;
    }

    if (altitudeValida && altitudeRelativa > ALTITUDE_MINIMA_LANCAMENTO_M) {
        detector.leiturasAcimaDoLimite++;
    }
    else {
        detector.leiturasAcimaDoLimite = 0;
    }

    if (detector.leiturasAcimaDoLimite >= LEITURAS_PARA_DETECTAR_LANCAMENTO) {
        detector.lancamentoDetectado = true;
    }
}
