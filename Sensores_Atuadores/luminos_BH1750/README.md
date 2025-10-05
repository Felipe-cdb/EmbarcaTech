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
    i2c_inst_t *i2c;        // Instância do barramento I2C
    uint8_t sda, scl;       // Pinos SDA e SCL do I2C
    uint8_t addr;           // Endereço I2C do sensor
    uint8_t mode;           // Modo de operação (contínuo ou one-time)
} BH1750;
```

Você cria um objeto `BH1750` para cada sensor que desejar usar.

---

# Funções disponíveis na biblioteca

## Inicialização do sensor

### Função principal de inicialização

```c
bool bh1750_init(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl);
```

* `sensor` → ponteiro para a estrutura do sensor BH1750
* `i2c` → instância do barramento I2C (ex.: i2c0, i2c1)
* `sda` → pino sda
* `scl` → pino scl

Os dados padrões alocados automaticamente
* `addr` → endereço I2C do sensor padrão(0x23)
* `mode` → modo de operação inicial com BH1750_CONTINUOUSLY_H_RES_MODE

Exemplo:

```c
BH1750 sensor1;
bh1750_init(&sensor1, i2c0, sda, scl);
```

### Inicialização personalizada
---

```c
bool bh1750_init_custom(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t mode)
```

* `sensor` → ponteiro para a estrutura do sensor BH1750
* `i2c` → instância do barramento I2C (ex.: i2c0, i2c1)
* `sda` → pino sda
* `scl` → pino scl
* `addr` → endereço I2C do sensor (0x23 ou 0x5C)
* `mode` → modo de operação inicial (ex.: BH1750\_CONTINUOUSLY\_H\_RES\_MODE)

Exemplo:

```c
BH1750 sensor1;
bh1750_init(&sensor1, i2c1, 2, 3, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);
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

## Função para alterar o modo do sensor
```c
void bh1750_set_mode(BH1750* sensor, uint8_t mode);
```
* `mode` → modo de leitura baixa a alta resolução e continua ou unica

### Modos disponíveis

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
#include "bh1750/bh1750.h"

// --- Configuração da Porta I2C da BitDogLab ---
// Alterne os comentários para escolher a porta desejada.
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0   // Corrigido o uso do "=" para a atribuição correta
#define I2C_SCL_PIN 1   // Corrigido o uso do "=" para a atribuição correta

int main() {
    stdio_init_all();

    // Inicializa o sensor. É crucial verificar o retorno desta função.
    BH1750 sensor;
    if (bh1750_init(&sensor, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)) {
        printf("Sensor BH1750 inicializado com sucesso.\n");
    } else {
        printf("ERRO: Falha ao inicializar o sensor BH1750.\n");
        return -1;  // Finaliza o programa em caso de falha na inicialização
    }

    // Ajusta o tempo de medição (sensibilidade)
    bh1750_set_measurement_time(&sensor, 100); // Aumenta sensibilidade (tempo de medição maior)

    while (1) {
        // Lê a luminosidade no modo contínuo
        float lux = bh1750_read_lux(&sensor);
        if (lux < 0) {
            printf("Erro ao ler sensor BH1750!\n");
        } else {
            printf("Luminosidade: %.2f lx\n", lux);
        }

        // Demonstração: mudar temporariamente para modo one-time
        bh1750_set_mode(&sensor, BH1750_ONE_TIME_H_RES_MODE);
        lux = bh1750_read_lux(&sensor);
        printf("Leitura One-Time H-RES: %.2f lx\n", lux);

        // Volta para modo contínuo H-RES
        bh1750_set_mode(&sensor, BH1750_CONTINUOUSLY_H_RES_MODE);

        sleep_ms(2000); // Espera 2 segundos antes da próxima leitura
    }

    return 0;
}

```

---

# CMakeLists.txt

```cmake
# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(distancia_VL53L0X 0)
pico_enable_stdio_usb(distancia_VL53L0X 1)

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
## Observações
- Cuidado ao configurar o CMakeLists: O nome do alvo do projeto deve corresponder corretamente. Por exemplo, o nome do alvo pode ser distancia_VL53L0X ou main, dependendo de como você configurar seu projeto.
- Nesse projeto estive utilizando o sdk 1.5.1