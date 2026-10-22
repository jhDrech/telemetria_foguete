import csv
import serial
from datetime import datetime
from pathlib import Path

PORTA = "/dev/ttyACM0"
BAUDRATE = 115200

PASTA_SAIDA = Path("dados")
PASTA_SAIDA.mkdir(exist_ok=True)

nome = datetime.now().strftime("teste_%Y-%m-%d_%H-%M-%S.csv")
caminho = PASTA_SAIDA / nome

print(f"Conectando em {PORTA}...")
print(f"Arquivo: {caminho}")
print("Ctrl+C para encerrar.\n")

try:
    with serial.Serial(PORTA, BAUDRATE, timeout=1) as ser:
        cabecalho = None

        with open(caminho, "w", newline="", encoding="utf-8") as arquivo:
            escritor = None

            while True:
                linha = ser.readline().decode("utf-8", errors="ignore").strip()

                if not linha:
                    continue

                # Detecta o cabeçalho CSV
                if cabecalho is None:
                    if linha.startswith("timestamp_ms"):
                        cabecalho = linha.split(",")
                        escritor = csv.writer(arquivo)
                        escritor.writerow(cabecalho)
                        arquivo.flush()

                        print("Cabeçalho detectado.")
                        print("Iniciando captura...\n")

                    continue

                valores = linha.split(",")

                # Ignora linhas incompatíveis
                if len(valores) != len(cabecalho):
                    continue

                escritor.writerow(valores)
                arquivo.flush()

                dados = dict(zip(cabecalho, valores))

                # ============================================================
                # VISUALIZAÇÃO RESUMIDA DO TESTE
                # ============================================================

                tempo_ms = dados.get("timestamp_ms", "?")
                dmp = dados.get("dmp_pronto", "?")

                ax_lin = dados.get("ax_linear", "?")
                ay_lin = dados.get("ay_linear", "?")
                az_lin = dados.get("az_linear", "?")

                try:
                    tempo_s = float(tempo_ms) / 1000.0
                    tempo_texto = f"{tempo_s:7.2f}s"
                except ValueError:
                    tempo_texto = str(tempo_ms)

                estado_dmp = (
                    "PRONTO" if dmp == "1"
                    else "aguardando"
                )

                print(
                    f"\rtempo={tempo_texto} | "
                    f"DMP={estado_dmp:10} | "
                    f"ax_lin={ax_lin:>8} | "
                    f"ay_lin={ay_lin:>8} | "
                    f"az_lin={az_lin:>8}",
                    end="",
                    flush=True
                )

except KeyboardInterrupt:
    print("\n\nCaptura encerrada.")
    print(f"CSV salvo em: {caminho}")

except serial.SerialException as erro:
    print(f"\nErro serial: {erro}")