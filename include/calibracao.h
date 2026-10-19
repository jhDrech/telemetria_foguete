#pragma once

#include "tipos.h"

Calibracao calibrarSistema(int numeroAmostras, int intervaloMs);

DadoTratado aplicarCalibracao(
    const DadoSensor& dado,
    const Calibracao& calibracao
);

