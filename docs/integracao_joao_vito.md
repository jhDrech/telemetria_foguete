# Integracao de Branch-joao e Branch-Vito-NoICM

A base de aquisicao e referencia barometrica vem de Joao; o fluxo de voo,
os detectores, o controle do servo, o filtro de Kalman, o armazenamento e
o Wi-Fi vem da branch NoICM do Vito.

## Versoes de origem e recuperacao

- `Branch-joao`: `a528c87d92241ca962ce844a4d5f8e585c90d226`.
- `Branch-Vito-NoICM`: `7e5f1e4cfe9ab5a491f65594ff595983914320a3`.
- `master` anterior: `5ddef8230350fb9288190352b72d279539adcfaf`.

As branches originais permanecem intactas. Foram criadas tambem as branches
`backup/pre-merge-2026-11-19-joao`,
`backup/pre-merge-2026-11-19-vito-noicm` e
`backup/pre-merge-2026-11-19-master`.
O merge mantem o historico das duas branches, sem squash ou rebase.
As versoes anteriores dos arquivos adaptados podem ser recuperadas pelos
commits acima. O programa de teste de Joao, antes em `src/main.cpp`, tambem
esta em `experimentos/main_joao_pre_integracao.cpp`.

## Adaptacoes

- `src/main.cpp` parte do main NoICM do Vito. Usa `lerSensores(dado)`,
  `atualizarPressaoInicial(pressaoInicial)` e
  `obterAlturaRelativa(pressaoInicial, dado.pressao)` de Joao.
- `DadoSensor` guarda os dados brutos e `DadoEstimado` recebe a altitude relativa.
  O CSV e `AmostraVoo` continuam com os mesmos campos da versao NoICM.
- A referencia de pressao e coletada no loop durante 3 segundos, como na branch
  de Joao, ignorando leituras invalidas. Os detectores e a coleta de amostras de
  voo so comecam apos essa referencia estar pronta. O servidor continua sendo
  atendido durante a preparacao.
- Corrigidos o nome da declaracao `obterALturaRelativa` e o retorno de uma
  variavel nao inicializada na funcao de altitude. Pressao invalida produz NaN.
- A ausencia de ICM nao impede o BMP. Quando presente, o ICM pode fornecer
  as leituras brutas de Joao; o fluxo NoICM continua tomando decisoes pela
  altitude, sem DMP. Amostras indisponiveis do ICM permanecem NaN.
- Mantidos os enderecos BMP 0x77/0x76 e a configuracao de amostragem usada pelo
  Vito. Os pinos I2C continuam SDA=8/SCL=9 e o servo continua no pino 10.
- Os modulos antigos ficam em `Alt`, fora da compilacao, conforme a organizacao
  de Joao. A atualizacao do Vito em calibracao foi preservada em
  `Alt/src/calibracao.cpp` pelo merge.
- `src/teste_wifi.cpp` foi preservado e separado em um ambiente PlatformIO
  proprio para evitar dois pares de `setup()`/`loop()` no mesmo firmware.

## Conferencia dos arquivos

Nenhum caminho de `Branch-joao` foi excluido. Fora dos quatro pontos de
integracao (`src/main.cpp`, `src/sensores.cpp`, `include/sensores.h` e
`platformio.ini`), todos os arquivos da `Branch-Vito-NoICM` foram preservados
byte a byte, no caminho original ou no caminho correspondente em `Alt`.

## Comandos de verificacao

```sh
pio run -e esp32-s3-super-mini
pio test -e native_test
pio run -e teste_wifi
```

O ambiente `native_test` usa sensores e relogio simulados em `test/support`,
exclusivamente para testes no computador. Inclui os testes existentes e
verifica leitura sem ICM, conversoes de unidades, amostras ausentes, calibracao,
entradas invalidas de pressao e a cadeia altitude/filtro/detectores/paraquedas.
A compilacao usa as bibliotecas reais no ambiente ESP32.

## Comportamento mantido da versao NoICM

O lancamento e detectado por altitude (limiar e contagem do Vito), o buffer
mantem 5 segundos anteriores e coleta ate 20 segundos apos o lancamento.
O Wi-Fi inicia no setup e e atendido antes do lancamento e depois da coleta.
O CSV continua incluindo altitude bruta e filtrada; esta integracao nao muda
esse formato nem implementa deteccao de pouso. O LittleFS continua sendo
aberto sem formatacao automatica, e o arquivo de saida continua sendo
`/ultimo_lancamento.csv`, substituido pelo proximo voo salvo.

Compilacao e simulacao nao validam os sensores fisicos, o servo ou um voo.
E necessario testar em bancada a referencia, a deteccao, a gravacao e o
download antes de usar o firmware em um lancamento.
