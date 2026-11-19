#pragma once 

struct FiltroKalmanAltitude{
    float estimativa,
            incerteza,
            ruidoProcesso,
            ruidoMedicao;
};

FiltroKalmanAltitude inicializarFiltroKalman (float estimativa, float incerteza, float ruidoProcesso, float ruidoMedicao);

    void atualizarFiltroKalman (FiltroKalmanAltitude& filtro, float altitudeMedida_m);