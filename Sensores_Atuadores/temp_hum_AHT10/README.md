# Tutorial de Uso da Biblioteca AHT10 para Raspberry Pi Pico

**Adaptada por Felipe Correia** das seguintes bibliotecas:

* Antonio Sergio Castro de Carvalho Junior
  [https://github.com/ASCCJR/TESTE_aht10_BITDOGLAB](https://github.com/ASCCJR/TESTE_aht10_BITDOGLAB)
* Juliano Oliveira
  [https://github.com/jrfo-hwit/hlab/tree/main/firmware/c_cpp/examples/3_aht10_i2c_uart0](https://github.com/jrfo-hwit/hlab/tree/main/firmware/c_cpp/examples/3_aht10_i2c_uart0)
* Adafruit Industries
  [https://github.com/adafruit/Adafruit_AHTX0/tree/master.c](https://github.com/adafruit/Adafruit_AHTX0/tree/master.c)

---

## Estrutura da Biblioteca

A biblioteca fornece funções simples para:

* Inicializar o sensor AHT10 (`aht10_init` / `aht10_init_default`)
* Ler temperatura e umidade (`aht10_get_temperature`, `aht10_get_humidity`)
* Calcular o ponto de orvalho (`aht10_get_dew_point`)
* Suporte a múltiplos sensores no mesmo barramento I2C

---

### Arquivos da Biblioteca

* **aht10.h** – Definições de constantes, struct e protótipos de funções
* **aht10.c** – Implementação das funções

> Atenção: os comandos do AHT10 (`CMD_INIT`, `CMD_MEASURE`, `CMD_RESET`) e bits de status estão definidos no `.h`.

---

## Inicializando o Sensor

### Com endereço padrão

```c
AHT10 sensor;
aht10_init_default(&sensor, i2c0); // endereço padrão 0x38
```

### Com endereço alternativo

```c
AHT10 sensor2;
aht10_init(&sensor2, i2c0, AHT10_ADDRESS_ALT); // endereço alternativo 0x39
```

> Útil para conectar dois sensores no mesmo barramento I2C.

---

## Leitura de Temperatura e Umidade

```c
float temp = aht10_get_temperature(&sensor1);
float hum  = aht10_get_humidity(&sensor1);

printf("Temp: %.2f °C\n", temp);
printf("Humidity: %.2f %%\n", hum);
```

> As funções fazem a leitura do sensor e retornam valores. Em caso de erro, retornam -1000 (°C) ou -1 (%RH).

---

## Cálculo do Ponto de Orvalho

```c
float dew = aht10_get_dew_point(&sensor1);
printf("Dew Point: %.2f °C\n", dew);
```

> Baseado na fórmula aproximada do ponto de orvalho usando temperatura e umidade relativa.

---

## Exemplo Completo com Dois Sensores

```c
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht10.h"

int main() {
    stdio_init_all();

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    AHT10 sensor1, sensor2;
    aht10_init_default(&sensor1, i2c0);           // 0x38
    aht10_init(&sensor2, i2c0, AHT10_ADDRESS_ALT); // 0x39

    while (1) {
        printf("Sensor1 -> Temp: %.2f C, Hum: %.2f %%\n",
               aht10_get_temperature(&sensor1),
               aht10_get_humidity(&sensor1));

        printf("Sensor2 -> Temp: %.2f C, Hum: %.2f %%\n",
               aht10_get_temperature(&sensor2),
               aht10_get_humidity(&sensor2));

        sleep_ms(1000);
    }
}
```

---

## Build com CMake

No `CMakeLists.txt`:

```cmake
target_link_libraries(temp_hum_AHT10
        pico_stdlib
        hardware_i2c
)

target_include_directories(temp_hum_AHT10 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

target_sources(temp_hum_AHT10 PRIVATE
        lib/aht10/aht10.c
)
```

> Isso inclui a biblioteca e o suporte ao I2C no projeto.

## Observações
- Cuidado ao configurar o CMakeLists: O nome do alvo do projeto deve corresponder corretamente. Por exemplo, o nome do alvo pode ser distancia_VL53L0X ou main, dependendo de como você configurar seu projeto.
- Nesse projeto estive utilizando o sdk 1.5.1