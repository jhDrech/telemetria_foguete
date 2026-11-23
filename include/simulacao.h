#include <Arduino.h>
#include <sensores.h>

void iniciarHIL();
bool lerDadosHIL(DadoSensor& dado);
bool atualizarPressaoInicialHIL(float pressaoAtual, float& pressaoInicial);

