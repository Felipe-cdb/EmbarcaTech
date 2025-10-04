# Biblioteca MPU6050 – Acelerômetro e Giroscópio

Biblioteca adaptada por Felipe Correia  
Baseada na biblioteca oficial da BitDogLab  
https://github.com/BitDogLab/BitDogLab-C/tree/main/Basic_peripherals/sensor_acelerometro_giroscopio/mpu6050  

---

## Introdução

O **MPU6050** é um sensor de **aceleração (3 eixos)**, **giroscópio (3 eixos)** e **temperatura** integrado.  
Esta biblioteca fornece funções para inicialização, configuração do range do acelerômetro e leitura dos valores tratados (convertidos) e crus (raw).

---

## Estrutura do Sensor

```c
typedef struct {
    i2c_inst_t *i2c;
    uint8_t sda, scl;
    uint8_t addr;

    // Valores convertidos
    float accel[3];      // valores em g
    float gyro[3];       // valores em °/s
    float temperature;   // temperatura em °C

    // Valores crus
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temp_raw;
    
    // range configurado (0=±2g, 1=±4g, 2=±8g, 3=±16g)
    uint16_t accel_range;
} MPU6050;
```

---

## Funções Disponíveis

| Função                                  | Descrição                                              |
| --------------------------------------- | ------------------------------------------------------ |
| `bool mpu6050_init(...)`                | Inicializa o sensor padrão (`0x68`)                    |
| `bool mpu6050_init_custom(...)`         | Inicializa com endereço e sensibilidade personalizados |
| `void mpu6050_reset(...)`               | Reseta o sensor                                        |
| `uint16_t mpu6050_get_accel_range(...)` | Retorna a sensibilidade atual                          |
| `void mpu6050_set_accel_range(...)`     | Define a sensibilidade desejada                        |
| `bool mpu6050_read(...)`                | Lê valores tratados (convertidos)                      |
| `bool mpu6050_read_raw(...)`            | Lê valores crus (raw)                                  |


---

## Constantes de Sensibilidade
| Constante        | Sensibilidade (LSB/g) | Faixa (±g) |
| ---------------- | --------------------- | ---------- |
| `ACCEL_SENS_2G`  | 16384                 | 2          |
| `ACCEL_SENS_4G`  | 8192                  | 4          |
| `ACCEL_SENS_8G`  | 4096                  | 8          |
| `ACCEL_SENS_16G` | 2048                  | 16         |


## Tabela de Pinos

Utilizados na bitdoglab pelo conectores I2C 0 `i2c0` ou I2C 1 `i2c1`.  
Os pinos também devem ser configurados conforme a interface I2C utilizada:

| Interface | Pino SDA | Pino SCL |
|-----------|----------|----------|
| i2c0      |    0     |     1    |
| i2c1      |    2     |     3    |

---

## Exemplo de Uso

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "mpu6050/mpu6050.h"

int main() {
    stdio_init_all();

    // Inicialização padrão
    MPU6050 sensor;
    mpu6050_init(&sensor, i2c0, 0, 1);

    // --- Inicialização customizada (opcional) ---
    //mpu6050_init_custom(&sensor, i2c1, 2, 3, 0x68, ACCEL_SENS_4G);

    printf("MPU6050 inicializado!\n");
    printf("Sensibilidade atual: %.0f LSB/g\n\n", (float)sensor.accel_range);

    while (true) {
        if (mpu6050_read(&sensor)) {
            printf("Accel: X=%.2f g Y=%.2f g Z=%.2f g\n",
                   sensor.accel[0], sensor.accel[1], sensor.accel[2]);
            printf("Gyro:  X=%.2f °/s Y=%.2f °/s Z=%.2f °/s\n",
                   sensor.gyro[0], sensor.gyro[1], sensor.gyro[2]);
            printf("Temp:  %.2f °C\n\n", sensor.temperature);
        }

        if (mpu6050_read_raw(&sensor)) {
            printf("Raw Accel: X=%d Y=%d Z=%d\n",
                   sensor.accel_raw[0], sensor.accel_raw[1], sensor.accel_raw[2]);
            printf("Raw Gyro:  X=%d Y=%d Z=%d\n",
                   sensor.gyro_raw[0], sensor.gyro_raw[1], sensor.gyro_raw[2]);
            printf("Raw Temp:  %d\n\n", sensor.temp_raw);
        }

        sleep_ms(1000);
    }
}

```

---

## Observações

- Sempre inicialize o sensor com `mpu6050_init()` antes de realizar leituras.  
- O range do acelerômetro pode ser ajustado conforme a aplicação.  
- Use `read()` para valores em unidades físicas ou `read_raw()` para valores crus.  


# Configurações do CMakeLists
```
# Add the standard library to the build
target_link_libraries(acelerometro_giroscopio_MPU6050
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(acelerometro_giroscopio_MPU6050 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Adicionar os arquivos da biblioteca ao projeto 
target_sources(acelerometro_giroscopio_MPU6050 PRIVATE 
        lib/mpu6050/mpu6050.c
) 

# Add any user requested libraries
target_link_libraries(acelerometro_giroscopio_MPU6050 
        hardware_i2c
)
```