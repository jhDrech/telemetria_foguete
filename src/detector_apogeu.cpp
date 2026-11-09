
#include "detector_apogeu.h"

DetectorApogeu inicializarDetectorApogeu()
{
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
    DetectorApogeu &detector,
    float altitudeMedida_m)
{
    const float tolerancia_m = 0.3f;

    if (detector.apogeuDetectado)
    {
        return;
    }

    // A primeira medição estabelece a referência inicial.
    if (!detector.primeiraLeituraRecebida)
    {
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
    if (detector.altitudeAtual_m > detector.maiorAltitude_m)
    {
        detector.maiorAltitude_m = detector.altitudeAtual_m;
    }

    // Verifica a queda acumulada em relacao a maior altitude.
    float quedaDesdeMaximo_m =
        detector.maiorAltitude_m - detector.altitudeAtual_m;

    if (quedaDesdeMaximo_m > tolerancia_m)
    {
        detector.leiturasEmQueda++;
    }
    else
    {
        detector.leiturasEmQueda = 0;
    }

    // Confirma apos cinco leituras abaixo do maximo,
    // com queda acumulada maior que a tolerancia.
    if (detector.leiturasEmQueda >= 5 &&
        !detector.apogeuDetectado &&
        detector.maiorAltitude_m >= ALTITUDE_MINIMA_PARA_APOGEU)
    {

        detector.apogeuDetectado = true;
    }
}
