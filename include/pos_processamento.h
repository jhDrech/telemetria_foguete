
#pragma once

#include <Arduino.h>
#include "sensores.h"
#include "filtro_kalman.h"

// Converte uma linha do CSV bruto em DadoSensor.
bool converterLinhaParaDadoSensor( const String& linha, DadoSensor& dado);

// Processa as amostras em ordem cronologica.
// O filtro deve manter seu estado entre as chamadas.
size_t processarBuffer( const DadoSensor* buffer, size_t quantidade, float pressaoInicial, FiltroKalmanAltitude& filtro, DadoEstimado* resultados);

// Le o CSV bruto no LittleFS e gera um novo CSV processado.
bool processarArquivoCSV( const char* caminhoEntrada, const char* caminhoSaida, float pressaoInicial);
