#ifndef DETECTOR_APOGEU_H
#define DETECTOR_APOGEU_H

constexpr float ALTITUDE_MINIMA_PARA_APOGEU = 5.0f; // metros

struct DetectorApogeu {
    float altitudeAtual_m;
    float altitudeAnterior_m;
    float maiorAltitude_m;

    int leiturasEmQueda;
    int leiturasSemQuedaSignificativa;

    bool primeiraLeituraRecebida;
    bool apogeuDetectado;
};

DetectorApogeu inicializarDetectorApogeu();

void atualizarDetectorApogeu(
    DetectorApogeu& detector,
    float altitudeMedida_m
);

#endif