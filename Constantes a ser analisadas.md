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

Complemento — atualização de 09/11/2026
Este complemento atualiza os nomes e critérios da versão sem ICM-20948. As referências anteriores à aceleração, ao DMP e ao contador de leituras sem queda deixam de se aplicar ao fluxo atual.
Lançamento por altitude
- ALTITUDE_MINIMA_LANCAMENTO_M = 5.0f: confirmar se o limiar diferencia ruído e movimentação na base de um lançamento real. A altitude é relativa à pressão de referência do solo e passa pelo Kalman.
- LEITURAS_PARA_DETECTAR_LANCAMENTO = 3: validar o compromisso entre falsas detecções e atraso na confirmação. O código exige valores acima de 5 m, não iguais ao limite.
- LIMITE_ACELERACAO_LANCAMENTO deixa de ser usado. Nomes de parâmetros que ainda mencionem aceleração devem ser atualizados para altitude, sem reativar o ICM.
Apogeu por queda acumulada
- tolerancia_m = 0.3f: passa a representar a diferença entre a maior altitude registrada e a altitude filtrada atual. Nao representa a queda entre duas amostras consecutivas.
- leiturasEmQueda >= 5: exige cinco leituras consecutivas mais de 0,3 m abaixo do máximo. As cinco leituras não precisam ser estritamente decrescentes; validar resistência ao ruído e atraso de acionamento.
- ALTITUDE_MINIMA_PARA_APOGEU = 5.0f: verificar a coerência com o limiar de lançamento e o comportamento em voos baixos.
- O critério antigo leiturasSemQuedaSignificativa >= 3 foi retirado do algoritmo revisado e não precisa mais ser ajustado.
Aquisição e duração do registro
Configuração atual	Valor inicial	Verificação nos testes
TAXA_AMOSTRAGEM_HZ	50 Hz	Medir a frequência real pelos timestamps.
INTERVALO_AMOSTRAGEM_MS	20 ms, calculado	Conferir variação do intervalo e ciclos que ultrapassam 20 ms.
TEMPO_PRE_LANCAMENTO_MS	3.000 ms	Avaliar se as últimas 150 amostras são suficientes.
TEMPO_POS_LANCAMENTO_MS	40.000 ms	Cobrir o voo desejado; reduzir temporariamente em testes de bancada e restaurar antes dos ensaios de voo.


O tempo posterior começa quando o lançamento é confirmado, acima do limiar de altitude. O histórico anterior também é relativo a essa confirmação, não ao instante físico de saída da base.

A espera no loop desconta o tempo de processamento. Se uma operação ultrapassar o intervalo, a taxa real diminui; 150 amostras não garantem exatamente 3 segundos.

CAPACIDADE_PRE_LANCAMENTO, CAPACIDADE_POS_LANCAMENTO e CAPACIDADE_TOTAL são calculadas a partir dessas configurações. Não devem ser ajustadas isoladamente. Com os valores atuais, a capacidade total é de 2.151 amostras, aproximadamente 42 KiB para a estrutura atual no ESP32. Aumentar a duração aumenta a RAM reservada.
Kalman de altitude
O filtro já está integrado ao fluxo principal.
Parâmetro	Valor inicial	O que avaliar
estimativa	0,0 m	Coerência com a referência de solo.
incerteza	1,0 m²	Convergência inicial.
ruidoProcesso	0,01 m²	Capacidade de acompanhar a subida e a descida.
ruidoMedicao	0,25 m²	Atenuação do ruído sem atraso excessivo.


Comparar as colunas de altitude bruta e filtrada do CSV. Como o filtro afeta os dois detectores, repetir os testes de lançamento e apogeu após alterar seus parâmetros. Reavaliar também os parâmetros se mudar a frequência de amostragem.
Calibração e servo
- calibrarSistema(100, 10): validar 100 tentativas de leitura com intervalo de 10 ms. A média revisada usa apenas as amostras válidas; a duração real inclui o processamento de cada leitura. Avaliar se é necessário exigir um número mínimo de amostras válidas, além de rejeitar a ausência total delas.
- TEMPO_ACIONAMENTO_MS = 500, ANGULO_INICIAL = 180 e ANGULO_ACIONAMENTO = 60: permanecem provisórios. Confirmar curso e esforço do mecanismo, tempo suficiente para liberar o paraquedas e adequação do retorno à posição inicial.
Registro de cada teste
Anotar data, versão do código, constantes utilizadas, condições do teste, resultado esperado, resultado observado e motivo de qualquer alteração. Mudar preferencialmente um parâmetro por vez. Ainda não houve validação física completa desta versão.