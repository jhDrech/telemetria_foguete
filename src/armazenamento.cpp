#include "armazenamento.h"

#include <LittleFS.h>
#include <string.h>

namespace {

File arquivoCSV;

bool armazenamentoIniciado = false;
bool arquivoAberto = false;
bool experimentoTemporario = false;
bool lancamentoConfirmado = false;

unsigned int quantidadeRegistros = 0;

const char* CAMINHO_TEMPORARIO =
    "/experimento_temporario.csv";

const char* CAMINHO_ULTIMO =
    "/ultimo_lancamento.csv";

const char* CAMINHO_PENULTIMO =
    "/penultimo_lancamento.csv";

// Reabre um arquivo preservando o conteudo existente.
bool reabrirParaContinuar(const char* caminho) {

    if (!LittleFS.exists(caminho)) {
        Serial.println("Arquivo nao encontrado para continuar gravacao!");
        return false;
    }

    arquivoCSV = LittleFS.open(caminho, "a");

    arquivoAberto = static_cast<bool>(arquivoCSV);
    quantidadeRegistros = 0;

    if (!arquivoAberto) {
        Serial.println("Falha ao reabrir arquivo CSV!");
        return false;
    }

    return true;
}

}

// ========================================================
// INICIALIZACAO DO LITTLEFS
// ========================================================

bool iniciarArmazenamento() {

    if (armazenamentoIniciado) {
        return true;
    }

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

    if (caminho == nullptr || cabecalho == nullptr) {
        return false;
    }

    // Evita sobrescrever um arquivo existente.
    if (LittleFS.exists(caminho)) {
        Serial.println("O arquivo CSV ja existe!");
        return false;
    }

    arquivoCSV = LittleFS.open(caminho, "w");

    if (!arquivoCSV) {
        Serial.println("Falha ao criar arquivo CSV!");
        return false;
    }

    // println acrescenta os caracteres de fim de linha: \r\n.
    size_t tamanhoEsperado = strlen(cabecalho) + 2;

    if (arquivoCSV.println(cabecalho) != tamanhoEsperado) {
        arquivoCSV.close();

        if (!LittleFS.remove(caminho)) {
            Serial.println("Falha ao remover arquivo incompleto!");
        }

        Serial.println("Falha ao escrever cabecalho CSV!");
        return false;
    }

    arquivoCSV.flush();

    quantidadeRegistros = 0;
    arquivoAberto = true;
    experimentoTemporario = false;
    lancamentoConfirmado = false;

    Serial.print("Arquivo CSV criado: ");
    Serial.println(caminho);

    return true;
}

// ========================================================
// CRIACAO DO EXPERIMENTO TEMPORARIO
// ========================================================

bool criarArquivoNovoVoo(const char* cabecalho) {

    if (!armazenamentoIniciado || arquivoAberto) {
        return false;
    }

    if (cabecalho == nullptr) {
        return false;
    }

    // Descarta somente o temporario da inicializacao anterior.
    // Os dois lancamentos confirmados permanecem preservados.
    if (LittleFS.exists(CAMINHO_TEMPORARIO)) {

        if (!LittleFS.remove(CAMINHO_TEMPORARIO)) {
            Serial.println("Falha ao remover experimento temporario!");
            return false;
        }
    }

    if (!criarArquivoCSV(CAMINHO_TEMPORARIO, cabecalho)) {
        return false;
    }

    experimentoTemporario = true;

    Serial.println("Aguardando confirmacao do lancamento.");

    return true;
}

// ========================================================
// CONFIRMACAO DO LANCAMENTO E ROTACAO DOS ARQUIVOS
// ========================================================

bool confirmarLancamentoArmazenamento() {

    if (!armazenamentoIniciado) {
        return false;
    }

    // Evita repetir a rotacao para o mesmo lancamento.
    if (lancamentoConfirmado) {
        return arquivoAberto;
    }

    if (!arquivoAberto || !experimentoTemporario) {
        return false;
    }

    // Sincroniza e fecha antes de renomear.
    finalizarArmazenamento();

    bool ultimoMovido = false;

    if (LittleFS.exists(CAMINHO_ULTIMO)) {

        // Libera o nome do penultimo lancamento.
        if (LittleFS.exists(CAMINHO_PENULTIMO)) {

            if (!LittleFS.remove(CAMINHO_PENULTIMO)) {
                Serial.println("Falha ao remover penultimo lancamento!");

                reabrirParaContinuar(CAMINHO_TEMPORARIO);
                return false;
            }
        }

        // O antigo ultimo passa a ser o penultimo.
        if (!LittleFS.rename(
                CAMINHO_ULTIMO,
                CAMINHO_PENULTIMO)) {

            Serial.println("Falha ao renomear ultimo lancamento!");

            reabrirParaContinuar(CAMINHO_TEMPORARIO);
            return false;
        }

        ultimoMovido = true;
    }

    // O experimento atual passa a ser o ultimo lancamento.
    if (!LittleFS.rename(
            CAMINHO_TEMPORARIO,
            CAMINHO_ULTIMO)) {

        Serial.println("Falha ao promover experimento temporario!");

        // Tenta devolver o ultimo lancamento ao nome anterior.
        if (ultimoMovido) {

            if (!LittleFS.rename(
                    CAMINHO_PENULTIMO,
                    CAMINHO_ULTIMO)) {

                Serial.println("Falha ao restaurar nome do ultimo lancamento!");
            }
        }

        reabrirParaContinuar(CAMINHO_TEMPORARIO);
        return false;
    }

    experimentoTemporario = false;
    lancamentoConfirmado = true;

    // Continua escrevendo depois das amostras ja registradas.
    if (!reabrirParaContinuar(CAMINHO_ULTIMO)) {
        return false;
    }

    Serial.println("Lancamento confirmado no armazenamento.");

    return true;
}

// ========================================================
// REGISTRO DE DADOS
// ========================================================

bool registrarDado(const String& linha) {

    if (!arquivoAberto) {
        return false;
    }

    size_t tamanhoEsperado = linha.length() + 2;

    if (arquivoCSV.println(linha) != tamanhoEsperado) {
        Serial.println("Falha ao escrever no arquivo CSV!");
        return false;
    }

    quantidadeRegistros++;

    if (quantidadeRegistros >= 25) {

        arquivoCSV.flush();

        quantidadeRegistros = 0;
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
        quantidadeRegistros = 0;

        Serial.println("Arquivo CSV finalizado!");
    }
}

bool gravacaoEmAndamento() {
    return arquivoAberto;
}