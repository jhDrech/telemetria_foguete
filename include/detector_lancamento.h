
#ifndef DETECTOR_LANCAMENTO_H
#define DETECTOR_LANCAMENTO_H

struct DetectorLancamento {
    int leiturasAcimaDoLimite;
    bool lancamentoDetectado;
};

constexpr float ALTITUDE_MINIMA_LANCAMENTO_M = 5.0f; // metros
constexpr int LEITURAS_PARA_DETECTAR_LANCAMENTO = 3;

DetectorLancamento inicializarDetectorLancamento();

void atualizarDetectorLancamento(
    DetectorLancamento& detector,
    float aceleracaoVertical,
    bool aceleracaoValida
);

#endif
