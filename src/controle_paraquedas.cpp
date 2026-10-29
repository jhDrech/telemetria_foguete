
#include "controle_paraquedas.h"
#include <Arduino.h>

ControleParaquedas inicializarControleParaquedas() {
    ControleParaquedas controle;

    controle.aberturaSolicitada = false;
    controle.estado = EstadoParaquedas::AGUARDANDO;
    controle.instanteAcionamento = 0;

    return controle;
}

void paraquedas(ControleParaquedas& controle, bool apogeuDetectado) {
    if (apogeuDetectado &&
        controle.estado == EstadoParaquedas::AGUARDANDO) {

        controle.aberturaSolicitada = true;
        controle.estado = EstadoParaquedas::ACIONADO;
        controle.instanteAcionamento = millis();
    }

    if (controle.estado == EstadoParaquedas::ACIONADO &&
        millis() - controle.instanteAcionamento >= TEMPO_ACIONAMENTO_MS) {

        controle.estado = EstadoParaquedas::FINALIZADO;
    }
}
