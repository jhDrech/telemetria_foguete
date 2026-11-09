#include "armazenamento.h"

#include <LittleFS.h>

namespace {

// ========================================================
// BUFFER DE AMOSTRAS EM RAM
// ========================================================

AmostraVoo amostras[CAPACIDADE_TOTAL];

size_t indicePreLancamento = 0;
size_t quantidadePreLancamento = 0;
size_t quantidadePosLancamento = 0;

// ========================================================
// CONTROLE DO ARMAZENAMENTO
// ========================================================

bool armazenamentoIniciado = false;
bool coletaEmAndamento = false;
bool lancamentoConfirmado = false;
bool arquivoSalvo = false;

String cabecalhoCSV;

const char* CAMINHO_CSV = "/ultimo_lancamento.csv";
const char* CAMINHO_TEMPORARIO = "/experimento_temporario.csv";

// ========================================================
// ESCRITA DE UMA AMOSTRA NO CSV
// ========================================================

bool escreverAmostra(File& arquivo, const AmostraVoo& amostra)
{
    // Interrompe a escrita se algum campo nao puder ser escrito.
    return
        arquivo.print(amostra.timestamp_ms) > 0 &&
        arquivo.print(",") > 0 &&
        arquivo.print(amostra.pressao_pa, 2) > 0 &&
        arquivo.print(",") > 0 &&
        arquivo.print(amostra.temperatura_c, 2) > 0 &&
        arquivo.print(",") > 0 &&
        arquivo.print(amostra.altitude_relativa_bruta_m, 3) > 0 &&
        arquivo.print(",") > 0 &&
        arquivo.print(amostra.altitude_relativa_filtrada_m, 3) > 0 &&
        arquivo.println() == 2;
}

}

// ========================================================
// INICIALIZACAO DO LITTLEFS
// ========================================================

bool iniciarArmazenamento()
{
    if (armazenamentoIniciado)
    {
        return true;
    }

    armazenamentoIniciado = LittleFS.begin(false);

    if (!armazenamentoIniciado)
    {
        Serial.println("Falha ao inicializar LittleFS!");
    }

    return armazenamentoIniciado;
}

// ========================================================
// PREPARACAO DO REGISTRO EM MEMORIA
// ========================================================

bool criarArquivoNovoVoo(const char* cabecalho)
{
    if (!armazenamentoIniciado ||
        coletaEmAndamento ||
        cabecalho == nullptr ||
        cabecalho[0] == '\0')
    {
        return false;
    }

    // Evita descartar um voo que ainda nao foi salvo.
    if (lancamentoConfirmado && !arquivoSalvo)
    {
        return false;
    }

    cabecalhoCSV = cabecalho;

    if (cabecalhoCSV != cabecalho)
    {
        return false;
    }

    indicePreLancamento = 0;
    quantidadePreLancamento = 0;
    quantidadePosLancamento = 0;

    lancamentoConfirmado = false;
    arquivoSalvo = false;
    coletaEmAndamento = true;

    Serial.println("Buffer preparado. Aguardando lancamento.");

    return true;
}

// ========================================================
// CONFIRMACAO DO LANCAMENTO
// ========================================================

bool confirmarLancamentoArmazenamento()
{
    if (!coletaEmAndamento)
    {
        return false;
    }

    // Preserva o buffer anterior e passa a registrar depois dele.
    lancamentoConfirmado = true;

    return true;
}

// ========================================================
// REGISTRO DE DADOS EM MEMORIA
// ========================================================

bool registrarAmostra(const AmostraVoo& amostra)
{
    if (!coletaEmAndamento)
    {
        return false;
    }

    if (!lancamentoConfirmado)
    {
        // Antes do lancamento, sobrescreve a amostra mais antiga.
        amostras[indicePreLancamento] = amostra;

        indicePreLancamento =
            (indicePreLancamento + 1) % CAPACIDADE_PRE_LANCAMENTO;

        if (quantidadePreLancamento < CAPACIDADE_PRE_LANCAMENTO)
        {
            quantidadePreLancamento++;
        }
    }
    else
    {
        // Depois do lancamento, nao sobrescreve dados.
        size_t indice =
            CAPACIDADE_PRE_LANCAMENTO + quantidadePosLancamento;

        if (indice >= CAPACIDADE_TOTAL)
        {
            return false;
        }

        amostras[indice] = amostra;
        quantidadePosLancamento++;
    }

    return true;
}

// ========================================================
// FINALIZACAO DA COLETA E GERACAO DO CSV
// ========================================================

bool finalizarArmazenamento()
{
    coletaEmAndamento = false;

    if (arquivoSalvo)
    {
        return true;
    }

    if (!armazenamentoIniciado ||
        !lancamentoConfirmado ||
        quantidadePosLancamento == 0)
    {
        return false;
    }

    // O CSV anterior permanece enquanto o novo e escrito.
    File arquivo = LittleFS.open(CAMINHO_TEMPORARIO, "w");

    if (!arquivo)
    {
        Serial.println("Falha ao criar CSV!");
        return false;
    }

    bool sucesso =
        arquivo.println(cabecalhoCSV) == cabecalhoCSV.length() + 2;

    // Localiza a amostra mais antiga do buffer circular.
    size_t inicio = 0;

    if (quantidadePreLancamento == CAPACIDADE_PRE_LANCAMENTO)
    {
        inicio = indicePreLancamento;
    }

    // Escreve o pre-lancamento em ordem cronologica.
    for (size_t i = 0; i < quantidadePreLancamento && sucesso; i++)
    {
        size_t indice =
            (inicio + i) % CAPACIDADE_PRE_LANCAMENTO;

        sucesso = escreverAmostra(arquivo, amostras[indice]);
    }

    // Escreve a amostra da deteccao e as posteriores.
    for (size_t i = 0; i < quantidadePosLancamento && sucesso; i++)
    {
        size_t indice = CAPACIDADE_PRE_LANCAMENTO + i;

        sucesso = escreverAmostra(arquivo, amostras[indice]);
    }

    arquivo.flush();
    arquivo.close();

    if (!sucesso)
    {
        Serial.println("Falha ao escrever CSV! Dados continuam na RAM.");
        return false;
    }

    // Substitui o CSV anterior pelo novo.
    if (!LittleFS.rename(CAMINHO_TEMPORARIO, CAMINHO_CSV))
    {
        Serial.println("Falha ao substituir CSV anterior!");
        return false;
    }

    arquivoSalvo = true;

    Serial.println("CSV salvo: /ultimo_lancamento.csv");

    return true;
}

bool gravacaoEmAndamento()
{
    return coletaEmAndamento;
}