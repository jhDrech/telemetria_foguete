#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H

#include <Arduino.h>

// ========================================================
// CONFIGURACAO DO REGISTRO EM MEMORIA
// ========================================================

constexpr uint32_t TAXA_AMOSTRAGEM_HZ = 50;

constexpr uint32_t TEMPO_PRE_LANCAMENTO_MS = 3000;
constexpr uint32_t TEMPO_POS_LANCAMENTO_MS = 40000;

constexpr uint32_t INTERVALO_AMOSTRAGEM_MS =
    1000 / TAXA_AMOSTRAGEM_HZ;

constexpr size_t CAPACIDADE_PRE_LANCAMENTO =
    (TAXA_AMOSTRAGEM_HZ * TEMPO_PRE_LANCAMENTO_MS + 999) / 1000;

constexpr size_t CAPACIDADE_POS_LANCAMENTO =
    (TAXA_AMOSTRAGEM_HZ * TEMPO_POS_LANCAMENTO_MS + 999) / 1000;

// Inclui uma posicao para a amostra da deteccao.
constexpr size_t CAPACIDADE_TOTAL =
    CAPACIDADE_PRE_LANCAMENTO +
    CAPACIDADE_POS_LANCAMENTO + 1;

// ========================================================
// DADOS DE CADA AMOSTRA
// ========================================================

struct AmostraVoo
{
    uint32_t timestamp_ms;
    float pressao_pa;
    float temperatura_c;
    float altitude_relativa_bruta_m;
    float altitude_relativa_filtrada_m;
};

// ========================================================
// FUNCOES DO ARMAZENAMENTO
// ========================================================

bool iniciarArmazenamento();

// Prepara o array sem criar arquivos.
bool criarArquivoNovoVoo(const char* cabecalho);

bool confirmarLancamentoArmazenamento();

bool registrarAmostra(const AmostraVoo& amostra);

// Encerra a coleta e salva o CSV.
bool finalizarArmazenamento();

bool gravacaoEmAndamento();

#endif