#include "detector_apogeu.h"

DetectorApogeu inicializarDetectorApogeu() {
    DetectorApogeu detector;

    detector.altitudeAtual_m = 0.0f;
    detector.altitudeAnterior_m = 0.0f;
    detector.maiorAltitude_m = 0.0f;
    detector.leiturasEmQueda = 0;
    detector.apogeuDetectado = false;

    return detector;
}

void atualizarDetectorApogeu(
    DetectorApogeu& detector, float altitudeMedida_m
) {
    detector.altitudeAnterior_m = detector.altitudeAtual_m;
    detector.altitudeAtual_m = altitudeMedida_m;

    if(detector.altitudeAtual_m > detector.maiorAltitude_m){
        detector.maiorAltitude_m = detector.altitudeAtual_m;
    }

    if(detector.altitudeAtual_m < detector.altitudeAnterior_m){
        detector.leiturasEmQueda++;
    } else {
        detector.leiturasEmQueda = 0;
    }

    if(detector.leiturasEmQueda >= 5 && detector.apogeuDetectado == false ){
        detector.apogeuDetectado = true;
    }
}

