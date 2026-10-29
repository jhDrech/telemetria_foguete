#include "controle_paraquedas.h"

ControleParaquedas inicializarControleParaquedas () {
    ControleParaquedas controle;

    controle.aberturaSolicitada = false;

    return controle;
}

void paraquedas(ControleParaquedas& controle, bool apogeuDetectado){
    if(apogeuDetectado){
        controle.aberturaSolicitada = true;
    }
}