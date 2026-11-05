#include "wifi.h"
#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>

const char *NOME_REDE = "FOGUETE_ESP32";
const char *SENHA = "12345478";

static WebServer servidor(80);
static bool servidorIniciado = false;

bool iniciarWiFi()
{
    bool sucesso = WiFi.softAP(NOME_REDE, SENHA);

    if (!sucesso)
    {
        Serial.println("Falha ao iniciar Wi-Fi!");
        return false;
    }

    // ========================================================
    // PAGINA INICIAL DO SERVIDOR
    // ========================================================

    servidor.on("/", HTTP_GET, []()
                { servidor.send(
                      200,
                      "text/html; charset=utf-8",
                      "<!DOCTYPE html>"
                      "<html lang='pt-BR'>"
                      "<head>"
                      "<meta charset='utf-8'>"
                      "<meta name='viewport' "
                      "content='width=device-width, initial-scale=1'>"
                      "<title>Telemetria do foguete</title>"
                      "</head>"
                      "<body>"
                      "<h1>Telemetria do foguete</h1>"
                      "<p>Conexao com o ESP32 funcionando.</p>"
                      "</body>"
                      "</html>"); });

    // ========================================================
    // DOWNLOAD DO ULTIMO LANCAMENTO
    // ========================================================

    servidor.on("/download/ultimo", HTTP_GET, []()
                {
        File arquivo = LittleFS.open(
            "/ultimo_lancamento.csv",
            "r"
        );

        if (!arquivo)
        {
            servidor.send(
                404,
                "text/plain; charset=utf-8",
                "Nenhum ultimo lancamento disponivel."
            );

            return;
        }

        servidor.sendHeader(
            "Content-Disposition",
            "attachment; filename=\"ultimo_lancamento.csv\""
        );

        servidor.streamFile(arquivo, "text/csv");
        arquivo.close(); });

    servidor.begin();
    servidorIniciado = true;

    Serial.println("Servidor HTTP iniciado!");

    Serial.println("Ponto de acesso iniciado!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.softAPIP());

    return true;
}

// ========================================================
// ATENDIMENTO DAS REQUISICOES DO NAVEGADOR
// ========================================================

void atualizarWiFi()
{
    if (servidorIniciado)
    {
        servidor.handleClient();
    }
}