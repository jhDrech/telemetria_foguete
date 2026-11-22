#include "armazenamento.h"

#include <LittleFS.h>

namespace
{
    DadoSensor amostras[CAPACIDADE_TOTAL];

    size_t indicePreLancamento = 0;
    size_t quantidadePreLancamento = 0;
    size_t quantidadePosLancamento = 0;

    bool armazenamentoIniciado = false;
    bool coletaEmAndamento = false;
    bool lancamentoConfirmado = false;

    EstadoArmazenamento estadoArmazenamento =
        EstadoArmazenamento::AGUARDANDO;

    bool arquivoSalvo = false;

    String cabecalhoCSV;

    const char *CAMINHO_CSV =
        "/ultimo_lancamento.csv";

    const char *CAMINHO_TEMPORARIO =
        "/experimento_temporario.csv";

    bool escreverAmostra(
        File &arquivo,
        const DadoSensor &amostra)
    {
        // Registra os dados brutos na ordem do cabecalho CSV.
        return
            arquivo.print(amostra.timestamp_ms) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.pressao, 2) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.temp, 2) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.ax, 5) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.ay, 5) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.az, 5) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.gx, 6) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.gy, 6) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.gz, 6) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.mx, 3) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.my, 3) > 0 &&
            arquivo.print(",") > 0 &&
            arquivo.print(amostra.mz, 3) > 0 &&
            arquivo.println() == 2;
    }
}

bool iniciarArmazenamento()
{
    if (armazenamentoIniciado)
    {
        return true;
    }

    armazenamentoIniciado =
        LittleFS.begin(false);

    if (!armazenamentoIniciado)
    {
        Serial.println(
            "Falha ao inicializar LittleFS!");
    }

    return armazenamentoIniciado;
}

bool criarArquivoNovoVoo(
    const char *cabecalho)
{
    if (!armazenamentoIniciado ||
        coletaEmAndamento ||
        cabecalho == nullptr ||
        cabecalho[0] == '\0')
    {
        return false;
    }

    if (lancamentoConfirmado &&
        !arquivoSalvo)
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

    estadoArmazenamento =
        EstadoArmazenamento::COLETANDO;

    Serial.println(
        "Buffer preparado. Aguardando lancamento.");

    return true;
}

bool confirmarLancamentoArmazenamento()
{
    if (!coletaEmAndamento)
    {
        return false;
    }

    lancamentoConfirmado = true;

    return true;
}

bool registrarAmostra(
    const DadoSensor &amostra)
{
    if (!coletaEmAndamento)
    {
        return false;
    }

    if (!lancamentoConfirmado)
    {
        amostras[indicePreLancamento] =
            amostra;

        indicePreLancamento =
            (indicePreLancamento + 1) %
            CAPACIDADE_PRE_LANCAMENTO;

        if (quantidadePreLancamento <
            CAPACIDADE_PRE_LANCAMENTO)
        {
            quantidadePreLancamento++;
        }
    }
    else
    {
        size_t indice =
            CAPACIDADE_PRE_LANCAMENTO +
            quantidadePosLancamento;

        if (indice >= CAPACIDADE_TOTAL)
        {
            return false;
        }

        amostras[indice] = amostra;

        quantidadePosLancamento++;
    }

    return true;
}

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

    estadoArmazenamento =
        EstadoArmazenamento::ERRO_AO_SALVAR;

    File arquivo =
        LittleFS.open(
            CAMINHO_TEMPORARIO,
            "w");

    if (!arquivo)
    {
        Serial.println(
            "Falha ao criar CSV!");

        return false;
    }

    bool sucesso =
        arquivo.println(cabecalhoCSV) ==
        cabecalhoCSV.length() + 2;

    size_t inicio = 0;

    if (quantidadePreLancamento ==
        CAPACIDADE_PRE_LANCAMENTO)
    {
        inicio = indicePreLancamento;
    }

    for (size_t i = 0;
         i < quantidadePreLancamento &&
         sucesso;
         i++)
    {
        size_t indice =
            (inicio + i) %
            CAPACIDADE_PRE_LANCAMENTO;

        sucesso =
            escreverAmostra(
                arquivo,
                amostras[indice]);
    }

    for (size_t i = 0;
         i < quantidadePosLancamento &&
         sucesso;
         i++)
    {
        size_t indice =
            CAPACIDADE_PRE_LANCAMENTO + i;

        sucesso =
            escreverAmostra(
                arquivo,
                amostras[indice]);
    }

    arquivo.flush();
    arquivo.close();

    if (!sucesso)
    {
        Serial.println(
            "Falha ao escrever CSV! Dados continuam na RAM.");

        return false;
    }

    if (!LittleFS.rename(
            CAMINHO_TEMPORARIO,
            CAMINHO_CSV))
    {
        Serial.println(
            "Falha ao substituir CSV anterior!");

        return false;
    }

    arquivoSalvo = true;

    estadoArmazenamento =
        EstadoArmazenamento::FINALIZADO;

    Serial.println(
        "CSV salvo: /ultimo_lancamento.csv");

    return true;
}

bool gravacaoEmAndamento()
{
    return coletaEmAndamento;
}

EstadoArmazenamento obterEstadoArmazenamento()
{
    return estadoArmazenamento;
}

bool tentarSalvarNovamente()
{
    if (estadoArmazenamento !=
        EstadoArmazenamento::ERRO_AO_SALVAR)
    {
        return false;
    }

    return finalizarArmazenamento();
}
