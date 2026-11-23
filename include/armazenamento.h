#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H

#include <Arduino.h>
#include "sensores.h"
#include <LittleFS.h>

// ========================================================
// CONFIGURACAO DO REGISTRO EM MEMORIA
// ========================================================

constexpr uint32_t TAXA_AMOSTRAGEM_HZ = 50;

constexpr uint32_t TEMPO_PRE_LANCAMENTO_MS = 5000;
constexpr uint32_t TEMPO_POS_LANCAMENTO_MS = 30000;

constexpr uint32_t INTERVALO_AMOSTRAGEM_MS =
    1000 / TAXA_AMOSTRAGEM_HZ;

constexpr size_t CAPACIDADE_PRE_LANCAMENTO =
    (TAXA_AMOSTRAGEM_HZ * TEMPO_PRE_LANCAMENTO_MS + 999) / 1000;

constexpr size_t CAPACIDADE_POS_LANCAMENTO =
    (TAXA_AMOSTRAGEM_HZ * TEMPO_POS_LANCAMENTO_MS + 999) / 1000;

constexpr size_t CAPACIDADE_TOTAL =
    CAPACIDADE_PRE_LANCAMENTO +
    CAPACIDADE_POS_LANCAMENTO + 1;

// ========================================================
// FUNCOES DO ARMAZENAMENTO
// ========================================================

enum class EstadoArmazenamento
{
    AGUARDANDO,
    COLETANDO,
    FINALIZADO,
    ERRO_AO_SALVAR
};

bool iniciarArmazenamento();

bool criarArquivoNovoVoo(const char *cabecalho);

bool confirmarLancamentoArmazenamento();

bool registrarAmostra(const DadoSensor &amostra);

bool finalizarArmazenamento();

bool gravacaoEmAndamento();

EstadoArmazenamento obterEstadoArmazenamento();

bool tentarSalvarNovamente();
bool definirPressaoReferencia(float pressaoInicial);
bool escreverAmostra(File& arquivo, const DadoSensor& amostra, float altitudeBruta, float altitudeFiltrada);
#endif
