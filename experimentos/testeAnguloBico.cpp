#include <Wire.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#define PIN_SDA 8
#define PIN_SCL 9

Adafruit_ICM20948 icm;

float calcularAnguloVertical(
    float componente,
    float modulo
) {
    if (modulo == 0.0f) {
        return 0.0f;
    }

    float cosTheta = componente / modulo;

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

    Serial.println(
        "timestamp_ms,"
        "ax,ay,az,"
        "angulo_x_vertical,"
        "angulo_y_vertical,"
        "angulo_z_vertical"
    );
}


void loop() {

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

    float modulo =
        sqrt(
            ax * ax +
            ay * ay +
            az * az
        );

    float anguloX =
        calcularAnguloVertical(
            ax,
            modulo
        );

    float anguloY =
        calcularAnguloVertical(
            ay,
            modulo
        );

    float anguloZ =
        calcularAnguloVertical(
            az,
            modulo
        );


    Serial.print(millis());
    Serial.print(",");

    Serial.print(ax, 4);
    Serial.print(",");

    Serial.print(ay, 4);
    Serial.print(",");

    Serial.print(az, 4);
    Serial.print(",");

    Serial.print(anguloX, 2);
    Serial.print(",");

    Serial.print(anguloY, 2);
    Serial.print(",");

    Serial.println(anguloZ, 2);

    delay(100);
}