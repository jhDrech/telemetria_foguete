#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#define PIN_SDA 8
#define PIN_SCL 9

Adafruit_ICM20948 icm;

constexpr float G = 9.80665f;
constexpr float TOLERANCIA = 1.0f;

const int angulosAlvo[] = {85, 80, 75, 70, 65, 60};
constexpr int QTD_ANGULOS = 6;

int indiceAtual = 0;

float axReferencia = 0.0f;
float anguloInicialVertical = 0.0f;

bool referenciaDefinida = false;

unsigned long inicioEstavel = 0;

constexpr unsigned long TEMPO_ESTAVEL_MS = 3000;
constexpr unsigned long TEMPO_MEDICAO_MS = 5000;

bool medindo = false;
unsigned long inicioMedicao = 0;


float calcularAnguloVerticalX(
    float ax,
    float ay,
    float az
) {
    float modulo = sqrt(
        ax * ax +
        ay * ay +
        az * az
    );

    float cosTheta = ax / modulo;

    cosTheta = constrain(
        cosTheta,
        -1.0f,
        1.0f
    );

    return acos(cosTheta) * 180.0f / PI;
}


void setup() {

    Serial.begin(115200);
    delay(1000);

    Wire.begin(PIN_SDA, PIN_SCL);

    if (!icm.begin_I2C(0x68, &Wire)) {

        Serial.println("ICM20948 nao encontrado");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("POSICIONE EM 85 GRAUS DO SOLO");
}


void loop() {

    if (indiceAtual >= QTD_ANGULOS) {

        Serial.println("TESTE FINALIZADO");

        while (true) {
            delay(1000);
        }
    }


    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    sensors_event_t mag;

    icm.getEvent(
        &accel,
        &gyro,
        &temp,
        &mag
    );

    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;

    float anguloVertical =
        calcularAnguloVerticalX(
            ax,
            ay,
            az
        );

    float anguloSolo =
        90.0f - anguloVertical;

    int alvo =
        angulosAlvo[indiceAtual];


    // -----------------------------------------------------
    // ESPERA VOCE CHEGAR AO ANGULO
    // -----------------------------------------------------

    if (!medindo) {

        Serial.print("ALVO=");
        Serial.print(alvo);

        Serial.print(" | atual=");
        Serial.println(anguloSolo, 2);


        if (
            abs(anguloSolo - alvo)
            <= TOLERANCIA
        ) {

            if (inicioEstavel == 0) {
                inicioEstavel = millis();
            }

            if (
                millis() - inicioEstavel
                >= TEMPO_ESTAVEL_MS
            ) {

                medindo = true;
                inicioMedicao = millis();

                if (!referenciaDefinida) {

                    axReferencia = ax;

                    anguloInicialVertical =
                        anguloVertical;

                    referenciaDefinida = true;
                }

                Serial.println();
                Serial.println("MEDINDO");

                Serial.println(
                    "timestamp_ms,"
                    "angulo_alvo,"
                    "angulo_vertical,"
                    "angulo_solo,"
                    "ax_bruto,"
                    "ax_referencia,"
                    "erro_medido,"
                    "erro_teorico"
                );
            }

        } else {

            inicioEstavel = 0;
        }

        delay(100);
        return;
    }


    // -----------------------------------------------------
    // REGISTRA DADOS
    // -----------------------------------------------------

    float erroMedido =
        ax - axReferencia;

    float thetaInicial =
        anguloInicialVertical *
        PI / 180.0f;

    float thetaAtual =
        anguloVertical *
        PI / 180.0f;

    float erroTeorico =
        G *
        (
            cos(thetaAtual) -
            cos(thetaInicial)
        );


    Serial.print(millis());
    Serial.print(",");

    Serial.print(alvo);
    Serial.print(",");

    Serial.print(anguloVertical, 2);
    Serial.print(",");

    Serial.print(anguloSolo, 2);
    Serial.print(",");

    Serial.print(ax, 5);
    Serial.print(",");

    Serial.print(axReferencia, 5);
    Serial.print(",");

    Serial.print(erroMedido, 5);
    Serial.print(",");

    Serial.println(erroTeorico, 5);


    // -----------------------------------------------------
    // TERMINA ESTE ANGULO
    // -----------------------------------------------------

    if (
        millis() - inicioMedicao
        >= TEMPO_MEDICAO_MS
    ) {

        medindo = false;
        inicioEstavel = 0;

        indiceAtual++;

        if (indiceAtual < QTD_ANGULOS) {

            Serial.println();
            Serial.print("AGORA MUDE PARA ");
            Serial.print(
                angulosAlvo[indiceAtual]
            );
            Serial.println(" GRAUS DO SOLO");
        }
    }

    delay(50);
}