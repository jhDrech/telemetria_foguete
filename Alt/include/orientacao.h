#pragma once

#include "tipos.h"

// Informacoes produzidas pelo modulo de orientacao.
// Algumas existem principalmente para diagnostico e para os graficos dos testes.
struct EstadoDMP {

    float roll_deg;
    float pitch_deg;

    float variacao_roll_deg;
    float variacao_pitch_deg;

    float norma_q;

    bool janela_estabilidade;
    bool dmp_pronto;
};

// Reinicia o controle de convergencia.
// Deve ser chamado uma vez depois da inicializacao dos sensores.
void iniciarOrientacao();

// Atualiza roll, pitch e o estado de convergencia usando o quaternion atual.
EstadoDMP atualizarEstadoDMP(const DadoSensor& dado);