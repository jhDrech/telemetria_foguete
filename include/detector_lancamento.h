
#ifndef DETECTOR_LANCAMENTO_H
#define DETECTOR_LANCAMENTO_H

struct DetectorLancamento {
    int leiturasAcimaDoLimite;
    bool lancamentoDetectado;
};

constexpr float ACELERACAO_MINIMA_LANCAMENTO = 19.6133f; // m/s²
constexpr float ALTITUDE_MINIMA_LANCAMENTO = 5.0f; // metros
constexpr int LEITURAS_PARA_DETECTAR_LANCAMENTO = 3;

void atualizarDetectorLancamentoBMP(DetectorLancamento &detectorLancamento, float altitudeRelativa, bool altitudeValida);
DetectorLancamento inicializarDetectorLancamento();

void atualizarDetectorLancamento( DetectorLancamento& detector, float aceleracaoVertical, bool aceleracaoValida);
#endif
