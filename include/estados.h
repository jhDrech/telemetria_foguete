#pragma oncea
#include <Arduino.h>

struct EstadosDoSistema{
    bool preparando;
    bool pronto;
    bool subindo;
    bool descendo;
    bool pousado;
};