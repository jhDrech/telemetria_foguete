#pragma once
#include <cstdint>
namespace bmpSimulado {
    inline uint8_t endereco = 0x77;
    inline float pressao = 100000.0f;
    inline float temperatura = 25.0f;
}
class Adafruit_BMP280 {
public:
    enum { MODE_NORMAL, SAMPLING_X1, SAMPLING_X4, FILTER_OFF, STANDBY_MS_1 };
    bool begin(uint8_t endereco) { return endereco == bmpSimulado::endereco; }
    void setSampling(int, int, int, int, int) {}
    float readPressure() { return bmpSimulado::pressao; }
    float readTemperature() { return bmpSimulado::temperatura; }
};
