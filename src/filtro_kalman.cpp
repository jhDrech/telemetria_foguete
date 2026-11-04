#include "filtro_kalman.h"
#include <cmath>

FiltroKalmanAltitude inicializarFiltroKalman(
    float estimativa,
    float incerteza,
    float ruidoProcesso,
    float ruidoMedicao
) {
    if (!std::isfinite(estimativa) ||
        !std::isfinite(incerteza) ||
        !std::isfinite(ruidoProcesso) ||
        !std::isfinite(ruidoMedicao) ||
        incerteza < 0.0f ||
        ruidoProcesso < 0.0f ||
        ruidoMedicao <= 0.0f) {

        return {0.0f, 0.0f, 0.0f, 1.0f};
    }

    FiltroKalmanAltitude filtro;
    filtro.estimativa = estimativa;
    filtro.incerteza = incerteza;
    filtro.ruidoMedicao = ruidoMedicao;
    filtro.ruidoProcesso = ruidoProcesso;

    return filtro;
}
void atualizarFiltroKalman(
    FiltroKalmanAltitude& filtro, float altitudeMedida_m
) {

    if (!std::isfinite(altitudeMedida_m)) return;

    if (filtro.incerteza < 0.0f ||
        filtro.ruidoProcesso < 0.0f ||
        filtro.ruidoMedicao <= 0.0f) {
        return;
    }

    float varianciaPrevista = filtro.incerteza + filtro.ruidoProcesso;
    float ganhoKalman = varianciaPrevista / (varianciaPrevista + filtro.ruidoMedicao);
    filtro.estimativa =  filtro.estimativa +( ganhoKalman * (altitudeMedida_m - filtro.estimativa));
    filtro.incerteza = (1.0f - ganhoKalman) * varianciaPrevista;
}