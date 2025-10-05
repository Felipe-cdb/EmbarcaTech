# Biblioteca VL53L0X – Sensor de Distância a Laser

Biblioteca adaptada por Felipe Correia
Baseada na biblioteca TESTE_vl53l0x_BITDOGLAB por Antonio Sergio Castro de Carvalho Junior
https://github.com/ASCCJR/TESTE_vl53l0x_BITDOGLAB

---

## Introdução

O **VL53L0X** é um sensor de distância a laser que oferece medições de alta precisão e baixo consumo de energia.
Esta biblioteca fornece funções para inicialização, leitura de distância em milímetros e centímetros, e configurações de operação contínua.

---

## Estrutura do Sensor

```c
typedef struct {
    i2c_inst_t* i2c;
    uint8_t sda, scl;
    uint8_t address;
    uint16_t io_timeout;
    uint8_t stop_variable;
    uint32_t measurement_timing_budget_us;
} VL53L0X;
```

---

## Funções Disponíveis

| Função                                  | Descrição                                              |
| --------------------------------------- | ------------------------------------------------------ |
| `bool vl53l0x_init(...)`                | Init sensor endeço padrão (`0x29`)                     |
| `bool vl53l0x_init_custom(...)`         | Inicializa com endereço e sensibilidade personalizados |
| `void vl53l0x_start_continuous(...)`    | Inicia a medição contínua com o intervalo especificado |
| `uint16_t vl53l0x_read_single_mm(...)`  | Lê a distância única em milímetros                     |
| ` --   vl53l0x_read_continuous_mm(...)` | Lê a distância contínua em milímetros                  |
| ` --   vl53l0x_read_single_cm(...)`     | Lê a distância única em centímetros                    |
| ` --   vl53l0x_read_continuous_cm(...)` | Lê a distância contínua em centímetros                 |


---

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
#include "vl53l0x/vl53l0x.h"

#define SDA_PIN 4
#define SCL_PIN 5
#define I2C_PORT i2c0

int main() {
    stdio_init_all();

    VL53L0X sensor;
    if (!vl53l0x_init(&sensor, I2C_PORT, SDA_PIN, SCL_PIN)) {
        printf("Erro ao inicializar o sensor VL53L0X\n");
        return 1;
    }

    while (true) {
        // Leitura de distância em milímetros
        uint16_t distance_mm = vl53l0x_read_single_mm(&sensor);
        if (distance_mm != 65535) {
            printf("Distância: %d mm\n", distance_mm);
        } else {
            printf("Falha na medição\n");
        }

        // Leitura de distância em centímetros
        uint16_t distance_cm = vl53l0x_read_single_cm(&sensor);
        if (distance_cm != 6553) {  // 65535 mm é igual a 6553 cm
            printf("Distância: %d cm\n", distance_cm);
        } else {
            printf("Falha na medição\n");
        }

        sleep_ms(1000);  // Espera 1 segundo antes da próxima medição
    }

    return 0;
}

```

---

## Observações
- Sempre inicialize o sensor com vl53l0x_init() antes de realizar leituras.
- O sensor pode ser configurado para operar em modo contínuo utilizando vl53l0x_start_continuous().
- Verifique o valor de retorno 65535 (em milímetros) ou 6553 (em centímetros) para indicar erro ou falha na medição.
---

# Configurações do CMakeLists
```
# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(distancia_VL53L0X 0)
pico_enable_stdio_usb(distancia_VL53L0X 1)

# Add the standard library to the build
target_link_libraries(distancia_VL53L0X
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(distancia_VL53L0X PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Adicionar os arquivos da biblioteca ao projeto 
target_sources(distancia_VL53L0X PRIVATE 
        lib/vl53l0x/vl53l0x.c
) 

# Add any user requested libraries
target_link_libraries(distancia_VL53L0X 
        hardware_i2c
)
```

## Observações
- Cuidado ao configurar o CMakeLists: O nome do alvo do projeto deve corresponder corretamente. Por exemplo, o nome do alvo pode ser distancia_VL53L0X ou main, dependendo de como você configurar seu projeto.
- Nesse projeto estive utilizando o sdk 1.5.1