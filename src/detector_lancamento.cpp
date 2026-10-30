#include "detector_lancamento.h"

DetectorLancamento inicializarDetectorLancamento()
{
    DetectorLancamento detectorLancamento;

    detectorLancamento.leiturasAcimaDoLimite = 0;
    detectorLancamento.lancamentoDetectado = false;

    return detectorLancamento;
}

void atualizarDetectorLancamento(DetectorLancamento &detectorLancamento,
                                 float aceleracao_vertical,
                                 bool aceleracao_vertical_valida)
{

    if (detectorLancamento.lancamentoDetectado) {
        return;
    }
    if (aceleracao_vertical_valida &&
        aceleracao_vertical > LIMITE_ACELERACAO_LANCAMENTO) {

        detectorLancamento.leiturasAcimaDoLimite++;
    }
    else{

        detectorLancamento.leiturasAcimaDoLimite = 0;
    }

    if (detectorLancamento.leiturasAcimaDoLimite >= LEITURAS_PARA_DETECTAR_LANCAMENTO) {
        detectorLancamento.lancamentoDetectado = true;
    }
}
