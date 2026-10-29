#pragma once

struct DetectorApogeu {
    float altitudeAtual_m,
          altitudeAnterior_m,
          maiorAltitude_m;
    int leiturasEmQueda,
        leiturasSemQuedaSignificativa;
    bool apogeuDetectado;
};

DetectorApogeu inicializarDetectorApogeu();

void atualizarDetectorApogeu (DetectorApogeu& detector, float altitudeMedida_m);
