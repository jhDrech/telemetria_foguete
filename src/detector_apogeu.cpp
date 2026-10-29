#include "detector_apogeu.h"

DetectorApogeu inicializarDetectorApogeu()
{
    DetectorApogeu detector;

    detector.altitudeAtual_m = 0.0f;
    detector.altitudeAnterior_m = 0.0f;
    detector.maiorAltitude_m = 0.0f;
    detector.leiturasEmQueda = 0;
    detector.apogeuDetectado = false;
    detector.leiturasSemQuedaSignificativa = 0;

    return detector;
}

void atualizarDetectorApogeu(
    DetectorApogeu &detector, float altitudeMedida_m)
{
    float tolerancia_m = 0.3f;
    float diferencaAltitude_m = 0.0f;
    detector.altitudeAnterior_m = detector.altitudeAtual_m;
    detector.altitudeAtual_m = altitudeMedida_m;
    diferencaAltitude_m = detector.altitudeAtual_m - detector.altitudeAnterior_m;

    if (detector.altitudeAtual_m > detector.maiorAltitude_m)
    {
        detector.maiorAltitude_m = detector.altitudeAtual_m;
    }

    if (diferencaAltitude_m < -tolerancia_m)
    {
        detector.leiturasEmQueda++;
        detector.leiturasSemQuedaSignificativa = 0;
    }
    else if (diferencaAltitude_m > tolerancia_m)
    {
        detector.leiturasEmQueda = 0;
        detector.leiturasSemQuedaSignificativa = 0;
    }
    else
    {
        detector.leiturasSemQuedaSignificativa++;

        if (detector.leiturasSemQuedaSignificativa >= 3)
        {
            detector.leiturasEmQueda = 0;
            detector.leiturasSemQuedaSignificativa = 0;
        }
    }

    if (detector.leiturasEmQueda >= 5 && detector.apogeuDetectado == false)
    {
        detector.apogeuDetectado = true;
    }
}
