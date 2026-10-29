#ifndef CONTROLE_PARAQUEDAS_H
#define CONTROLE_PARAQUEDAS_H


struct ControleParaquedas {
    bool aberturaSolicitada;
};

ControleParaquedas inicializarControleParaquedas();

void paraquedas (ControleParaquedas& controle, bool apogeuDetectado);

#endif