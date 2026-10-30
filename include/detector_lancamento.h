
#ifndef DETECTOR_LANCAMENTO_H
#define DETECTOR_LANCAMENTO_H

struct DetectorLancamento {
    int leiturasAcimaDoLimite;
    bool lancamentoDetectado;
};

constexpr float LIMITE_ACELERACAO_LANCAMENTO = 15.0f; // m/s²
constexpr int LEITURAS_PARA_DETECTAR_LANCAMENTO = 3;

DetectorLancamento inicializarDetectorLancamento();

void atualizarDetectorLancamento(
    DetectorLancamento& detector,
    float aceleracaoVertical,
    bool aceleracaoValida
);

#endif
