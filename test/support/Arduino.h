#pragma once
// Simulacao minima usada somente pelo ambiente native_test.
#include <cstdint>
#include <cstddef>
#include <cmath>
using std::isfinite;
constexpr float DEG_TO_RAD = 0.01745329251994329577f;
inline uint32_t tempoSimuladoMs = 0;
inline unsigned long millis() { return tempoSimuladoMs; }
inline void delay(unsigned long ms) { tempoSimuladoMs += ms; }
