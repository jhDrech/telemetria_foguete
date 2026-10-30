# Constantes pendentes de validação experimental

Os valores abaixo são provisórios e deverão ser revisados após testes com o sistema embarcado e, quando possível, com o foguete em condições reais de lançamento.

## 1. Detector de lançamento

- `LIMITE_ACELERACAO_LANCAMENTO = 15.0 m/s²`
  - Validar o limite de aceleração vertical necessário para reconhecer um lançamento.
- `LEITURAS_PARA_DETECTAR_LANCAMENTO = 3`
  - Avaliar a quantidade de leituras consecutivas necessárias para confirmar o lançamento.

## 2. Detector de apogeu

- `tolerancia_m = 0.3 m`
  - Validar a variação mínima de altitude considerada significativa.
- `leiturasEmQueda >= 5`
  - Avaliar quantas leituras consecutivas de queda são necessárias para identificar o apogeu.
- `leiturasSemQuedaSignificativa >= 3`
  - Validar quantas leituras sem queda significativa são necessárias para zerar o contador de queda.

## 3. Controle do paraquedas

- `TEMPO_ACIONAMENTO_MS = 500`
  - Validar o tempo de permanência do servo na posição de acionamento.
- `ANGULO_INICIAL = 180`
  - Validar a posição inicial do servo.
- `ANGULO_ACIONAMENTO = 60`
  - Validar a posição de acionamento do servo.

## 4. Aquisição de dados

- `TAXA_HZ = 50`
  - Avaliar se a frequência de 50 Hz é suficiente para registrar os eventos do voo.
- `delay(20)`
  - Verificar se o intervalo de aquisição efetivamente mantém a frequência desejada.
- `TEMPO_GRAVACAO_SEG = 40`
  - Avaliar se o período de gravação contempla todas as etapas relevantes do voo.
- `PRE_LANCAMENTO = 3 s`
  - Avaliar se o histórico anterior ao lançamento é suficiente.

## 5. Calibração e processamento

- `calibrarSistema(100, 10)`
  - Validar a quantidade de amostras de calibração e o intervalo utilizado.
- `tolerância de estabilidade do DMP`
  - Revisar os limites utilizados para considerar a orientação estável.
- `ruidoProcesso` e `ruidoMedicao` do filtro de Kalman
  - Ajustar conforme o comportamento observado pelos sensores, caso o filtro seja integrado ao fluxo principal.

## Observação

Os valores devem ser validados individualmente, considerando:
- Ruído dos sensores.
- Repetibilidade entre lançamentos.
- Tempo de resposta do sistema.
- Possibilidade de falsas detecções.
- Compatibilidade com o mecanismo físico utilizado.

A validação deve ser registrada com os valores testados, resultados observados e justificativa para cada alteração.