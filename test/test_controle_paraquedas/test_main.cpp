
#include <unity.h>
#include "controle_paraquedas.h"

void setUp() {}
void tearDown() {}

// Testa se o controle começa com a abertura desativada.
void testarInicializacao() {
    ControleParaquedas controle = inicializarControleParaquedas();

    TEST_ASSERT_FALSE(controle.aberturaSolicitada);
}

// Testa se a abertura é solicitada quando o apogeu é detectado.
void testarSolicitacaoAbertura() {
    ControleParaquedas controle = inicializarControleParaquedas();

    paraquedas(controle, true);

    TEST_ASSERT_TRUE(controle.aberturaSolicitada);
}

// Testa se a abertura permanece solicitada quando o apogeu
// não está detectado em uma chamada posterior.
void testarManutencaoAbertura() {
    ControleParaquedas controle = inicializarControleParaquedas();

    paraquedas(controle, true);
    paraquedas(controle, false);

    TEST_ASSERT_TRUE(controle.aberturaSolicitada);
}

// Testa se a abertura continua desativada sem detecção do apogeu.
void testarSemApogeu() {
    ControleParaquedas controle = inicializarControleParaquedas();

    paraquedas(controle, false);

    TEST_ASSERT_FALSE(controle.aberturaSolicitada);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(testarInicializacao);
    RUN_TEST(testarSolicitacaoAbertura);
    RUN_TEST(testarManutencaoAbertura);
    RUN_TEST(testarSemApogeu);

    return UNITY_END();
}
