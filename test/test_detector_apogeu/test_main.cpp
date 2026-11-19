
#include <unity.h>
#include "detector_apogeu.h"

void setUp() {}
void tearDown() {}

void testarInicializacao() {
    DetectorApogeu detector = inicializarDetectorApogeu();

    TEST_ASSERT_FALSE(detector.apogeuDetectado);
    TEST_ASSERT_EQUAL_INT(0, detector.leiturasEmQueda);
    TEST_ASSERT_EQUAL_INT(0, detector.leiturasSemQuedaSignificativa);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, detector.maiorAltitude_m);
}

void testarMaiorAltitude() {
    DetectorApogeu detector = inicializarDetectorApogeu();

    atualizarDetectorApogeu(detector, 10.0f);
    atualizarDetectorApogeu(detector, 15.0f);
    atualizarDetectorApogeu(detector, 12.0f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, detector.maiorAltitude_m);
}

void testarDeteccaoApogeu() {
    DetectorApogeu detector = inicializarDetectorApogeu();

    atualizarDetectorApogeu(detector, 10.0f);
    atualizarDetectorApogeu(detector, 20.0f);

    atualizarDetectorApogeu(detector, 19.0f);
    atualizarDetectorApogeu(detector, 18.0f);
    atualizarDetectorApogeu(detector, 17.0f);
    atualizarDetectorApogeu(detector, 16.0f);

    TEST_ASSERT_FALSE(detector.apogeuDetectado);

    atualizarDetectorApogeu(detector, 15.0f);

    TEST_ASSERT_TRUE(detector.apogeuDetectado);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(testarInicializacao);
    RUN_TEST(testarMaiorAltitude);
    RUN_TEST(testarDeteccaoApogeu);

    return UNITY_END();
}
