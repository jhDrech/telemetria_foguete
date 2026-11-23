
#include "pos_processamento.h"

#include <LittleFS.h>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>

bool converterLinhaParaDadoSensor(
    const String& linha,
    DadoSensor& dado
) {
    DadoSensor convertido{};

    const char* cursor = linha.c_str();
    char* fim = nullptr;

    // Primeiro campo: timestamp_ms
    if (*cursor == '-') return false;

    unsigned long timestamp = strtoul(cursor, &fim, 10);

    if (fim == cursor ||
        *fim != ',' ||
        timestamp > UINT32_MAX) {
        return false;
    }

    convertido.timestamp_ms =
        static_cast<uint32_t>(timestamp);

    cursor = fim + 1;

    // Os 11 campos float, na ordem do CSV bruto.
    float* campos[] = {
        &convertido.pressao,
        &convertido.temp,
        &convertido.ax,
        &convertido.ay,
        &convertido.az,
        &convertido.gx,
        &convertido.gy,
        &convertido.gz,
        &convertido.mx,
        &convertido.my,
        &convertido.mz
    };

    constexpr size_t TOTAL_CAMPOS = 11;

    for (size_t i = 0; i < TOTAL_CAMPOS; i++) {
        float valor = strtof(cursor, &fim);

        if (fim == cursor) {
            return false;
        }

        *campos[i] = valor;

        if (i < TOTAL_CAMPOS - 1) {
            if (*fim != ',') {
                return false;
            }

            cursor = fim + 1;
        } else {
            // Nenhum campo adicional e permitido.
            if (*fim != '\0' && *fim != '\r') {
                return false;
            }
        }
    }

    dado = convertido;
    return true;
}


size_t processarBuffer(
    const DadoSensor* buffer,
    size_t quantidade,
    float pressaoInicial,
    FiltroKalmanAltitude& filtro,
    DadoEstimado* resultados
) {
    if (buffer == nullptr ||
        resultados == nullptr ||
        !std::isfinite(pressaoInicial) ||
        pressaoInicial <= 0.0f) {
        return 0;
    }

    for (size_t i = 0; i < quantidade; i++) {
        DadoEstimado estimado{};

        float pressao = buffer[i].pressao;

        if (std::isfinite(pressao) && pressao > 0.0f) {
            float altitudeBarometrica =
                obterAlturaRelativa(
                    pressaoInicial,
                    pressao
                );

            if (std::isfinite(altitudeBarometrica)) {
                atualizarFiltroKalman(
                    filtro,
                    altitudeBarometrica
                );

                estimado.altitude_relativa =
                    filtro.estimativa;
            }
        }

        // A velocidade vertical permanece NAN.
        resultados[i] = estimado;
    }

    return quantidade;
}


bool processarArquivoCSV(
    const char* caminhoEntrada,
    const char* caminhoSaida,
    float pressaoInicial
) {
    if (caminhoEntrada == nullptr ||
        caminhoSaida == nullptr ||
        strcmp(caminhoEntrada, caminhoSaida) == 0 ||
        !std::isfinite(pressaoInicial) ||
        pressaoInicial <= 0.0f) {
        return false;
    }

    File entrada = LittleFS.open(caminhoEntrada, "r");

    if (!entrada) {
        return false;
    }

    File saida = LittleFS.open(caminhoSaida, "w");

    if (!saida) {
        entrada.close();
        return false;
    }

    // Mesmos parametros do Kalman online da master.
    FiltroKalmanAltitude filtro =
        inicializarFiltroKalman(
            0.0f,
            1.0f,
            0.01f,
            0.25f
        );

    bool sucesso = true;

    // Ignora o cabecalho do arquivo original.
    if (!entrada.available()) {
        sucesso = false;
    } else {
        entrada.readStringUntil('\n');
    }

    if (sucesso) {
        saida.println(
            "timestamp_ms,"
            "altitude_barometrica,"
            "altitude_kalman"
        );
    }

    while (sucesso && entrada.available()) {
        String linha = entrada.readStringUntil('\n');
        linha.trim();

        if (linha.length() == 0) {
            continue;
        }

        DadoSensor dado{};

        if (!converterLinhaParaDadoSensor(linha, dado)) {
            sucesso = false;
            break;
        }

        DadoEstimado estimado{};

        if (processarBuffer(
                &dado,
                1,
                pressaoInicial,
                filtro,
                &estimado
            ) != 1) {
            sucesso = false;
            break;
        }

        float altitudeBarometrica =
            obterAlturaRelativa(
                pressaoInicial,
                dado.pressao
            );

        // Registra uma linha no CSV processado.
        saida.print(dado.timestamp_ms);
        saida.print(",");
        saida.print(altitudeBarometrica, 5);
        saida.print(",");
        saida.println(estimado.altitude_relativa, 5);

        if (saida.getWriteError()) {
            sucesso = false;
            break;
        }
    }

    saida.flush();

    if (saida.getWriteError()) {
        sucesso = false;
    }

    entrada.close();
    saida.close();

    // Nao deixa um arquivo parcial parecer valido.
    if (!sucesso) {
        LittleFS.remove(caminhoSaida);
    }

    return sucesso;
}
