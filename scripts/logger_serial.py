import csv
import serial

from datetime import datetime
from pathlib import Path


PORTA = "/dev/ttyACM0"
BAUDRATE = 115200

PASTA_SAIDA = Path("dados")
PASTA_SAIDA.mkdir(exist_ok=True)

nome_arquivo = datetime.now().strftime(
    "teste_%Y-%m-%d_%H-%M-%S.csv"
)

caminho = PASTA_SAIDA / nome_arquivo


print(f"Conectando em {PORTA}...")
print(f"Arquivo: {caminho}")
print("Ctrl+C para encerrar.\n")


try:

    with serial.Serial(
        PORTA,
        BAUDRATE,
        timeout=2
    ) as ser:

        cabecalho = None

        with open(
            caminho,
            "w",
            newline="",
            encoding="utf-8"
        ) as arquivo:

            escritor = csv.writer(arquivo)

            while True:

                bruto = ser.readline()

                if not bruto:
                    continue

                linha = bruto.decode(
                    "utf-8",
                    errors="ignore"
                ).strip()

                if not linha:
                    continue


                # ============================================================
                # PROCURA O CABECALHO
                # ============================================================

                if cabecalho is None:

                    if linha.startswith("timestamp_ms"):

                        cabecalho = linha.split(",")

                        escritor.writerow(cabecalho)
                        arquivo.flush()

                        print("Cabecalho detectado.")
                        print(
                            f"Numero de colunas: "
                            f"{len(cabecalho)}"
                        )
                        print("Iniciando captura...\n")

                    continue


                # ============================================================
                # LEITURA DOS DADOS
                # ============================================================

                valores = linha.split(",")


                # Se aparecer alguma mensagem que nao seja CSV,
                # mostramos para diagnostico em vez de ignorar silenciosamente.
                if len(valores) != len(cabecalho):

                    print(
                        f"\n[IGNORADA] "
                        f"{len(valores)} valores "
                        f"(esperados {len(cabecalho)})"
                    )

                    print(linha)

                    continue


                # ============================================================
                # SALVA O CSV
                # ============================================================

                escritor.writerow(valores)
                arquivo.flush()


                # Transforma:
                #
                # cabecalho + valores
                #
                # em:
                #
                # dados["ax_linear"]
                # dados["dmp_pronto"]
                # etc.

                dados = dict(
                    zip(cabecalho, valores)
                )


                # ============================================================
                # VISUALIZACAO DO TESTE
                # ============================================================

                try:

                    tempo_s = (
                        float(
                            dados["timestamp_ms"]
                        )
                        / 1000.0
                    )

                except (ValueError, KeyError):

                    tempo_s = 0.0


                dmp = dados.get(
                    "dmp_pronto",
                    "?"
                )

                altitude = dados.get(
                    "altitude_relativa_m",
                    "?"
                )

                ax_linear = dados.get(
                    "ax_linear",
                    "?"
                )

                ay_linear = dados.get(
                    "ay_linear",
                    "?"
                )

                az_linear = dados.get(
                    "az_linear",
                    "?"
                )


                if dmp == "1":
                    estado_dmp = "PRONTO"
                else:
                    estado_dmp = "aguardando"


                print(
                    f"\r"
                    f"tempo={tempo_s:7.2f}s | "
                    f"DMP={estado_dmp:10} | "
                    f"alt={altitude:>8} m | "
                    f"ax={ax_linear:>8} | "
                    f"ay={ay_linear:>8} | "
                    f"az={az_linear:>8}",
                    end="",
                    flush=True
                )


except KeyboardInterrupt:

    print("\n\nCaptura encerrada.")
    print(f"CSV salvo em: {caminho}")


except serial.SerialException as erro:

    print("\nErro na comunicacao serial:")
    print(erro)