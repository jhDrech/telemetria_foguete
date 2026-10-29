
#include "detector_apogeu.h"

DetectorApogeu inicializarDetectorApogeu() {
    DetectorApogeu detector;

    detector.altitudeAtual_m = 0.0f;
    detector.altitudeAnterior_m = 0.0f;
    detector.maiorAltitude_m = 0.0f;

    detector.leiturasEmQueda = 0;
    detector.leiturasSemQuedaSignificativa = 0;

    detector.primeiraLeituraRecebida = false;
    detector.apogeuDetectado = false;

    return detector;
}

void atualizarDetectorApogeu(
    DetectorApogeu& detector,
    float altitudeMedida_m
) {
    const float tolerancia_m = 0.3f;

    // A primeira medição estabelece a referência inicial.
    if (!detector.primeiraLeituraRecebida) {
        detector.altitudeAtual_m = altitudeMedida_m;
        detector.altitudeAnterior_m = altitudeMedida_m;
        detector.maiorAltitude_m = altitudeMedida_m;

        detector.primeiraLeituraRecebida = true;

        return;
    }

    // Atualiza as altitudes e calcula a variação.
    detector.altitudeAnterior_m = detector.altitudeAtual_m;
    detector.altitudeAtual_m = altitudeMedida_m;

    float diferencaAltitude_m =
        detector.altitudeAtual_m - detector.altitudeAnterior_m;

    // Guarda a maior altitude registrada.
    if (detector.altitudeAtual_m > detector.maiorAltitude_m) {
        detector.maiorAltitude_m = detector.altitudeAtual_m;
    }

    // Verifica se houve queda significativa.
    if (diferencaAltitude_m < -tolerancia_m) {
        detector.leiturasEmQueda++;
        detector.leiturasSemQuedaSignificativa = 0;
    }
    else if (diferencaAltitude_m > tolerancia_m) {
        detector.leiturasEmQueda = 0;
        detector.leiturasSemQuedaSignificativa = 0;
    }
    else {
        detector.leiturasSemQuedaSignificativa++;

        if (detector.leiturasSemQuedaSignificativa >= 3) {
            detector.leiturasEmQueda = 0;
            detector.leiturasSemQuedaSignificativa = 0;
        }
    }

    // Confirma o apogeu após cinco leituras consecutivas de queda.
    if (detector.leiturasEmQueda >= 5 &&
        !detector.apogeuDetectado) {

        detector.apogeuDetectado = true;
    }
}
