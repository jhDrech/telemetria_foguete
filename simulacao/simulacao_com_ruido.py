
import serial
import time
import csv
import random
from serial import SerialException
from datetime import datetime
from pathlib import Path

PORTA = "/dev/ttyACM0"
BAUDRATE = 115200
INTERVALO = 0.02  # 20 ms = 50 Hz
DURACAO_SIMULACAO_S = 38  # Inclui repouso apos o voo para permitir o salvamento
TIMEOUT_PRONTO_S = 30

CABECALHO = [
    "timestamp_ms", "pressao", "temp",
    "ax", "ay", "az",
    "gx", "gy", "gz",
    "altitude_semKalman", "altitude_comKalman"
]

respostas = []
diagnosticos_filtro = []
hil_pronto = False
csv_salvo = False

PRESSAO_INICIAL = 100000.0

# Ruido em Pa, aproximado a partir do teste do BMP280 com IIR x4.
DESVIO_RUIDO_PRESSAO_PA = 2.7
SEMENTE_RUIDO = 20261123  # Mantem o mesmo ruido entre execucoes.
gerador_ruido = random.Random(SEMENTE_RUIDO)
referencias = []

def calcular_pressao(altitude):
    return PRESSAO_INICIAL * (1 - altitude / 44330.0) ** 5.255

def simular_voo(tempo):
    if tempo < 4:
        # Foguete parado: calibração
        altitude = 0
        az = 9.80665

    elif tempo < 5:
        # Lançamento
        altitude = 20 * (tempo - 4)
        az = 30.0

    elif tempo < 8:
        # Subida até o apogeu
        progresso = (tempo - 5) / 3
        altitude = 20 + 50 * (2 * progresso - progresso**2)
        az = 9.80665

    elif tempo < 12:
        # Descida
        progresso = (tempo - 8) / 4
        altitude = 70 * (1 - progresso**2)
        az = 9.80665

    else:
        # Pouso
        altitude = 0
        az = 9.80665

    return altitude, az

buffer_serial = bytearray()

def receber_respostas(esp):
    global buffer_serial, hil_pronto, csv_salvo

    quantidade = esp.in_waiting

    if quantidade == 0:
        return

    buffer_serial.extend(esp.read(quantidade))

    while b"\n" in buffer_serial:
        linha, _, restante = buffer_serial.partition(b"\n")
        buffer_serial = bytearray(restante)

        resposta = linha.decode(
            "utf-8", errors="replace"
        ).strip()

        if not resposta:
            continue

        if resposta.startswith("# HIL_FILTRO,"):
            try:
                _, timestamp, altitude_bruta, altitude_kalman = resposta.split(",")

                diagnosticos_filtro.append([
                    int(timestamp),
                    float(altitude_bruta),
                    float(altitude_kalman)
                ])

            except ValueError:
                print(f"Diagnostico invalido: {resposta}")

            continue

        if resposta == "# HIL_READY":
            hil_pronto = True
            print("ESP32: HIL pronto para receber amostras")
            continue

        if "CSV salvo:" in resposta or "CSV salvo e disponivel" in resposta:
            csv_salvo = True
            print(f"ESP32: {resposta}")
            continue

        # Mensagens de diagnóstico
        if resposta.startswith("#"):
            print(f"ESP32: {resposta}")
            continue

        # Dados CSV
        campos = resposta.split(",")

        if len(campos) != 11:
            print(f"Resposta invalida: {resposta}")
            continue

        try:
            int(campos[0])

            for campo in campos[1:]:
                float(campo)

        except ValueError:
            print(f"Resposta invalida: {resposta}")
            continue

        respostas.append(campos)


try:
    with serial.Serial(PORTA, BAUDRATE, timeout=0.1) as esp:
        # Evita enviar pressões de calibração enquanto o ESP32 ainda faz setup.
        print("Aguardando ESP32 iniciar o HIL...")
        prazo_pronto = time.monotonic() + TIMEOUT_PRONTO_S
        while not hil_pronto and time.monotonic() < prazo_pronto:
            receber_respostas(esp)
            time.sleep(0.01)

        if not hil_pronto:
            raise RuntimeError(
                "ESP32 nao enviou # HIL_READY. Confira a porta, "
                "o firmware MODO_HIL e reinicie a placa."
            )

        print("Iniciando simulacao de repouso...")
        inicio = time.monotonic()

        altitudes_verdadeiras = {}
        
        for i in range(int(DURACAO_SIMULACAO_S / INTERVALO)):
            #tempo da simulacao
            timestamp = i * 20
            tempo = timestamp / 1000.0

            # Calcula a altitude verdadeira e a aceleração
            altitude, az = simular_voo(tempo)

            # Guarda a altitude verdadeira associada ao timestamp
            altitudes_verdadeiras[timestamp] = altitude

            # Calcula a pressão que será enviada ao ESP32
            pressao_ideal = calcular_pressao(altitude)
            ruido_pa = gerador_ruido.gauss(0.0, DESVIO_RUIDO_PRESSAO_PA)
            pressao = pressao_ideal + ruido_pa

            # Registro independente para comparar com a trajetoria ideal.
            referencias.append([timestamp, altitude, pressao_ideal, pressao, ruido_pa])

            temperatura = 25.0
            ax = 0.0
            ay = 0.0

            # Monta a linha CSV
            linha = (
                f"{timestamp},{pressao:.2f},{temperatura:.2f},"
                f"{ax:.5f},{ay:.5f},{az:.5f}\n"
            )

            # Envia a amostra ao ESP32
            esp.write(linha.encode("utf-8"))

            # Recebe as respostas do ESP32
            receber_respostas(esp)

            # Mantém o intervalo de 20 ms (50 Hz)
            proximo_envio = inicio + (i + 1) * INTERVALO
            espera = proximo_envio - time.monotonic()

            if espera > 0:
                time.sleep(espera)


                # Recebe respostas que ainda ficaram pendentes
        fim_leitura = time.monotonic() + 1.0

        while time.monotonic() < fim_leitura:
            receber_respostas(esp)
            time.sleep(0.01)

except SerialException as erro:
    print(f"Erro de comunicacao Serial: {erro}")

finally:



    pasta = Path("simulacao/experimentos")
    pasta.mkdir(parents=True, exist_ok=True)
    
    data_hora = datetime.now().strftime("%Y-%m-%d_%H-%M-%S-%f")
    
    nome_arquivo = pasta / f"hil_voo_{data_hora}.csv"
    
    with open(nome_arquivo, "w", newline="") as arquivo:
        escritor = csv.writer(arquivo)
        escritor.writerow(CABECALHO)
        escritor.writerows(respostas)
    
    nome_referencia = pasta / f"hil_referencia_{data_hora}.csv"
    with open(nome_referencia, "w", newline="") as arquivo:
        escritor = csv.writer(arquivo)
        escritor.writerow([
            "timestamp_ms", "altitude_ideal_m", "pressao_ideal_pa",
            "pressao_enviada_pa", "ruido_pa"
        ])
        escritor.writerows(referencias)

    print(f"Referencia da simulacao salva em: {nome_referencia}")
    print(f"Ruido aplicado a pressao: sigma={DESVIO_RUIDO_PRESSAO_PA:.2f} Pa; semente={SEMENTE_RUIDO}")
    print(f"Amostras recebidas: {len(respostas)}")
    print(f"Salvamento no ESP32 confirmado pelo log: {csv_salvo}")
    print(f"Simulacao salva em: {nome_arquivo}")