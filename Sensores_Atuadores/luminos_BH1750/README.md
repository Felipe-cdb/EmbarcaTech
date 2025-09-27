# BH1750 – Sensor de luminosidade

Adaptada por Felipe Correia das seguintes bibliotecas
Biblioteca bh1750 por dernasherbrezon
[https://github.com/dernasherbrezon/bh1750](https://github.com/dernasherbrezon/bh1750)
Biblioteca TESTE_bh1750_BITDOGLAB por por Antonio Sergio Castro de Carvalho Junior
[https://github.com/ASCCJR/TESTE\_bh1750\_BITDOGLAB/tree/main](https://github.com/ASCCJR/TESTE_bh1750_BITDOGLAB/tree/main)

**Observação:** Basta colocar a pasta `bh1750` dentro do seu projeto e configurar o cmake de acordo com sua estrutura.

---

# Introdução

O BH1750 é um sensor digital de luminosidade (lux) que se comunica via I2C.
Ele possui modos de operação contínuos e “one-time”, além de permitir ajuste do tempo de medição (sensibilidade).
A biblioteca foi adaptada para permitir múltiplos sensores no mesmo barramento I2C.

---

# Estrutura BH1750

```c
typedef struct {
    i2c_inst_t *i2c;   // instância do I2C
    uint8_t addr;      // endereço I2C do sensor
    uint8_t mode;      // modo de operação atual
} BH1750;
```

Você cria um objeto `BH1750` para cada sensor que desejar usar.

---

# Funções disponíveis na biblioteca

## Inicialização do sensor

### Função principal

```c
void bh1750_init(BH1750 *sensor, i2c_inst_t *i2c, uint8_t addr, uint8_t mode);
```

* `sensor` → ponteiro para a estrutura do sensor BH1750
* `i2c` → instância do barramento I2C (ex.: i2c0, i2c1)
* `addr` → endereço I2C do sensor (0x23 ou 0x5C)
* `mode` → modo de operação inicial (ex.: BH1750\_CONTINUOUSLY\_H\_RES\_MODE)

Exemplo:

```c
BH1750 sensor1;
bh1750_init(&sensor1, i2c0, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);
```

---

## Leitura de luminosidade

### Função de leitura

```c
float bh1750_read_lux(BH1750 *sensor);
```

* Retorna valor de luminosidade em lux
* Retorna `-1.0f` em caso de erro

Exemplo:

```c
float lux = bh1750_read_lux(&sensor1);
printf("Luminosidade: %.2f lux\n", lux);
```

### Funcionamento automático

Se o `mode` escolhido na inicialização for contínuo (`BH1750_CONTINUOUSLY_*`), a função apenas lê os dados.
Se o `mode` for “one-time”, a biblioteca envia o comando de medição, espera o tempo adequado e depois lê o valor.

---

## Ajuste do tempo de medição (sensibilidade)

```c
void bh1750_set_measurement_time(BH1750 *sensor, uint8_t mt);
```

* `mt` → tempo de medição de 31 a 254 (default = 69)
  Exemplo:

```c
bh1750_set_measurement_time(&sensor1, 120);
```

---

## Modos disponíveis

```c
#define BH1750_CONTINUOUSLY_H_RES_MODE   0x10 // Alta resolução (~1 lx), contínuo
#define BH1750_CONTINUOUSLY_H_RES_MODE2  0x11 // Alta resolução (~0.5 lx), contínuo
#define BH1750_CONTINUOUSLY_L_RES_MODE   0x13 // Baixa resolução (~4 lx), contínuo

#define BH1750_ONE_TIME_H_RES_MODE       0x20 // Alta resolução, leitura única
#define BH1750_ONE_TIME_H_RES_MODE2      0x21 // Alta resolução 0.5 lx, leitura única
#define BH1750_ONE_TIME_L_RES_MODE       0x23 // Baixa resolução, leitura única
```

---

# Exemplo de uso – múltiplos sensores

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bh1750.h"

int main() {
    stdio_init_all();

    // Inicializa I2C
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Cria dois sensores BH1750
    BH1750 sensor1, sensor2;

    bh1750_init(&sensor1, i2c0, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);
    bh1750_init(&sensor2, i2c0, BH1750_ADDRESS_ALT, BH1750_ONE_TIME_L_RES_MODE);

    while (1) {
        float lux1 = bh1750_read_lux(&sensor1);
        float lux2 = bh1750_read_lux(&sensor2);

        if (lux1 >= 0) printf("Sensor 1: %.2f lux\n", lux1);
        if (lux2 >= 0) printf("Sensor 2: %.2f lux\n", lux2);

        sleep_ms(1000);
    }
}
```

---

# CMakeLists.txt

```cmake
# Add the standard library to the build
target_link_libraries(luminos_BH1750
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(luminos_BH1750 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Adicionar os arquivos da biblioteca ao projeto 
target_sources(luminos_BH1750 PRIVATE 
        lib/bh1750/bh1750.c
) 

# Add any user requested libraries
target_link_libraries(luminos_BH1750 
        hardware_i2c
        )

```
