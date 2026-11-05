#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H

#include <Arduino.h>

bool iniciarArmazenamento();

bool criarArquivoCSV(
    const char* caminho,
    const char* cabecalho
);

bool criarArquivoNovoVoo(const char* cabecalho);

bool confirmarLancamentoArmazenamento();

bool registrarDado(const String& linha);

void finalizarArmazenamento();

bool gravacaoEmAndamento();

#endif