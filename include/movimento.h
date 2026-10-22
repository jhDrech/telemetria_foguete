#pragma once

#include "tipos.h"

// Calcula a aceleracao linear usando:
// - aceleracao medida pelo acelerometro;
// - bias conhecido do eixo X;
// - quaternion fornecido pelo DMP.
//
// A funcao modifica os campos:
// ax_linear
// ay_linear
// az_linear
// aceleracao_linear_valida
//
// dentro do proprio DadoSensor.
void iniciarMovimento();

void calcularAceleracaoLinear( DadoSensor& dado, bool dmpPronto);

void atualizarVelocidadeVertical( DadoSensor& dado);
