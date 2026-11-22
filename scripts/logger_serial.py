import csv
import serial

from datetime import datetime
from pathlib import Path


PORTA = "/dev/ttyACM0"
BAUDRATE = 115200


# ============================================================
# PASTA DE SAIDA
# ============================================================

PASTA_SCRIPT = Path(__file__).resolve().parent
PASTA_PROJETO = PASTA_SCRIPT.parent

PASTA_DADOS = PASTA_PROJETO / "dados"
PASTA_DADOS.mkdir(parents=True, exist_ok=True)

nome = datetime.now().strftime(
    "teste_%Y-%m-%d_%H-%M-%S.csv"
)

caminho = PASTA_DADOS / nome


# ============================================================
# CRIA O ARQUIVO IMEDIATAMENTE
# ============================================================

arquivo = open(
    caminho,
    "w",
    newline="",
    encoding="utf-8"
)

escritor = csv.writer(arquivo)

print("Arquivo criado em:")
print(caminho.resolve())
print()


# ============================================================
# SERIAL
# ============================================================

try:

    ser = serial.Serial(
        PORTA,
        BAUDRATE,
        timeout=2
    )

    print(f"Serial conectada em {PORTA}")
    print("Esperando cabecalho...\n")

    cabecalho = None

    while True:

        linha = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if not linha:
            continue


        # ----------------------------------------------------
        # AINDA NAO ENCONTROU O CABECALHO
        # ----------------------------------------------------

        if cabecalho is None:

            print(f"[SERIAL] {linha}")

            if linha.startswith("timestamp_ms"):

                cabecalho = linha.split(",")

                escritor.writerow(cabecalho)
                arquivo.flush()

                print("\nCABECALHO ENCONTRADO")
                print(cabecalho)
                print("\nGravando...\n")

            continue


        # ----------------------------------------------------
        # DADOS
        # ----------------------------------------------------

        valores = linha.split(",")

        if len(valores) != len(cabecalho):

            print(f"[IGNORADA] {linha}")

            continue


        escritor.writerow(valores)

        # força a gravacao no disco
        arquivo.flush()

        print(linha)


except KeyboardInterrupt:

    print("\nCaptura encerrada.")


except serial.SerialException as erro:

    print("\nERRO SERIAL:")
    print(erro)


finally:

    arquivo.flush()
    arquivo.close()

    try:
        ser.close()
    except:
        pass

    print("\nCSV salvo em:")
    print(caminho.resolve())