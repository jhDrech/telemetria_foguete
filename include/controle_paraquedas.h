
#ifndef CONTROLE_PARAQUEDAS_H
#define CONTROLE_PARAQUEDAS_H

constexpr unsigned long TEMPO_ACIONAMENTO_MS = 500;

enum class EstadoParaquedas {
    AGUARDANDO,
    ACIONADO,
    FINALIZADO
};

struct ControleParaquedas {
    bool aberturaSolicitada;
    EstadoParaquedas estado;
    unsigned long instanteAcionamento;
};

ControleParaquedas inicializarControleParaquedas();

void paraquedas(ControleParaquedas& controle, bool apogeuDetectado);

#endif
