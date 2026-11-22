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
        // Mantem o criterio ja adotado no projeto:
        // amostras validas acima do limiar sao acumuladas.
        detector.leiturasAcimaDoLimite++;
    }

    if (detector.leiturasAcimaDoLimite >=
        LEITURAS_PARA_DETECTAR_LANCAMENTO)
    {
        detector.lancamentoDetectado = true;
    }
}
