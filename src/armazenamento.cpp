#include "armazenamento.h"

#include <LittleFS.h>

namespace {

File arquivoCSV;

bool armazenamentoIniciado = false;
bool arquivoAberto = false;

}

// ========================================================
// INICIALIZACAO DO LITTLEFS
// ========================================================

bool iniciarArmazenamento() {

    armazenamentoIniciado = LittleFS.begin(false);

    if (!armazenamentoIniciado) {
        Serial.println("Falha ao inicializar LittleFS!");
        return false;
    }

    Serial.println("LittleFS inicializado!");

    return true;
}

// ========================================================
// CRIACAO DO ARQUIVO CSV
// ========================================================

bool criarArquivoCSV(
    const char* caminho,
    const char* cabecalho
) {

    if (!armazenamentoIniciado || arquivoAberto) {
        return false;
    }

    arquivoCSV = LittleFS.open(caminho, "w");

    if (!arquivoCSV) {
        Serial.println("Falha ao criar arquivo CSV!");
        return false;
    }

    if (arquivoCSV.println(cabecalho) == 0) {
        arquivoCSV.close();
        return false;
    }

    arquivoCSV.flush();

    arquivoAberto = true;

    Serial.println("Arquivo CSV criado!");

    return true;
}

// ========================================================
// REGISTRO DE DADOS
// ========================================================

bool registrarDado(const String& linha) {

    if (!arquivoAberto) {
        return false;
    }

    if (arquivoCSV.println(linha) == 0) {
        return false;
    }

    return true;
}

// ========================================================
// FINALIZACAO DO ARQUIVO
// ========================================================

void finalizarArmazenamento() {

    if (arquivoAberto) {

        arquivoCSV.flush();
        arquivoCSV.close();

        arquivoAberto = false;

        Serial.println("Arquivo CSV finalizado!");
    }
}