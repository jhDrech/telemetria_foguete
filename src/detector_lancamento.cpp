#include "detector_lancamento.h"

DetectorLancamento inicializarDetectorLancamento()
{
    DetectorLancamento detectorLancamento;

    detectorLancamento.leiturasAcimaDoLimite = 0;
    detectorLancamento.lancamentoDetectado = false;

    return detectorLancamento;
}

void atualizarDetectorLancamento(DetectorLancamento &detectorLancamento, float aceleracaoTotal, bool aceleracaoValida)
{
    if (detectorLancamento.lancamentoDetectado) {
        return;
    }

    if (aceleracaoValida &&
        aceleracaoTotal > ACELERACAO_MINIMA_LANCAMENTO) {

        detectorLancamento.leiturasAcimaDoLimite++;
    }
    else {
        detectorLancamento.leiturasAcimaDoLimite = 0;
    }

    if (detectorLancamento.leiturasAcimaDoLimite >= LEITURAS_PARA_DETECTAR_LANCAMENTO) {
        detectorLancamento.lancamentoDetectado = true;
    }
}

void atualizarDetectorLancamentoBMP(DetectorLancamento &detectorLancamento, float altitudeRelativa, bool altitudeValida)
{
    if(detectorLancamento.lancamentoDetectado){
        return;
    }

    if(altitudeValida && altitudeRelativa > ALTITUDE_MINIMA_LANCAMENTO){
        detectorLancamento.leiturasAcimaDoLimite++;
    }
    else{
        detectorLancamento.leiturasAcimaDoLimite = 0;
    }

    if(detectorLancamento.leiturasAcimaDoLimite >= LEITURAS_PARA_DETECTAR_LANCAMENTO){
        detectorLancamento.lancamentoDetectado = true;
    }
}


