#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050/mpu6050.h"

int main() {
    stdio_init_all(); // Inicializa comunicação serial padrão

    // -----------------------------
    // Inicialização do sensor (modo padrão)
    // Usa o barramento I2C0 com SDA=0 e SCL=1
    // Sensibilidade padrão: ±2g
    // -----------------------------
    MPU6050 sensor;
    mpu6050_init(&sensor, i2c0, 0, 1);

    // -----------------------------
    // Inicialização customizada (opcional)
    // Exemplo: usar outro barramento I2C e mudar a sensibilidade para ±4g
    // -----------------------------
    //MPU6050 sensor;
    //mpu6050_init_custom(&sensor, i2c1, 2, 3, 0x68, ACCEL_SENS_4G);

    printf("MPU6050 inicializado com sucesso!\n");
    printf("Sensibilidade atual: %.0f LSB/g\n\n", (float)sensor.accel_range);

    while (true) {
        // Lê os dados convertidos (g, °/s e °C)
        if (mpu6050_read(&sensor)) {
            printf("===== Leitura de dados convertidos =====\n");
            printf("Accel: X=%.2f g | Y=%.2f g | Z=%.2f g\n",
                   sensor.accel[0], sensor.accel[1], sensor.accel[2]);
            printf("Gyro : X=%.2f °/s | Y=%.2f °/s | Z=%.2f °/s\n",
                   sensor.gyro[0], sensor.gyro[1], sensor.gyro[2]);
            printf("Temp : %.2f °C\n\n", sensor.temperature);
        } else {
            printf("Erro na leitura convertida do MPU6050.\n");
        }

        // Lê os dados crus (sem conversão)
        if (mpu6050_read_raw(&sensor)) {
            printf("===== Leitura de dados crus =====\n");
            printf("Raw Accel: X=%d | Y=%d | Z=%d\n",
                   sensor.accel_raw[0], sensor.accel_raw[1], sensor.accel_raw[2]);
            printf("Raw Gyro : X=%d | Y=%d | Z=%d\n",
                   sensor.gyro_raw[0], sensor.gyro_raw[1], sensor.gyro_raw[2]);
            printf("Raw Temp : %d\n\n", sensor.temp_raw);
        } else {
            printf("Erro na leitura crua do MPU6050.\n");
        }

        sleep_ms(1000); // Aguarda 1 segundo entre leituras
    }
}
