#include "filtro_kalman.h"
#include <cmath>

FiltroKalmanAltitude inicializarFiltroKalman(
    float estimativa,
    float incerteza,
    float ruidoProcesso,
    float ruidoMedicao
) {

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

    if (std::isnan(altitudeMedida_m)) return; // proteção pra NaN


    float varianciaPrevista = filtro.incerteza + filtro.ruidoProcesso;
    float ganhoKalman = varianciaPrevista / (varianciaPrevista + filtro.ruidoMedicao);
    filtro.estimativa =  filtro.estimativa +( ganhoKalman * (altitudeMedida_m - filtro.estimativa));
    filtro.incerteza = (1.0f - ganhoKalman) * varianciaPrevista;
}