#include "servidor.h"
#include <WebServer.h>

WebServer servidor(80);

void iniciarServidor() {
    servidor.on("/", HTTP_GET, []() {
        servidor.send(200, "text/plain",
                      "Foguete pronto para transferencia!");
    });

    servidor.begin();

}

void atualizarServidor(){
        servidor.handleClient();
}