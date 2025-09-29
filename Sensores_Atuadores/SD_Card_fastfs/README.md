# Introdução sobre as funções originais da biblioteca FatFs
A biblioteca [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) é um módulo genérico para sistemas embarcados que permite trabalhar com cartões SD em formato FAT/exFAT.  
Com ela é possível montar o cartão, criar arquivos, escrever, ler e manipular diretórios de forma simples.  

---

## ⚙️ Estrutura típica
Ao usar a biblioteca, você terá arquivos principais como:
- **ff.c / ff.h** → núcleo da biblioteca  
- **diskio.c / diskio.h** → camada de acesso ao hardware (driver SD)  
- **ffconf.h** → configurações de compilação  

No seu `main.c`, basta incluir os headers e usar as funções do FatFs.  

---

## 🚀 Passos básicos de uso

### 1. Inclusão de headers
```c
#include "ff.h"       // API principal
#include "diskio.h"   // Interface do driver
#include <stdio.h>
```

### 2. Variáveis globais
```c
FATFS fs;     // Sistema de arquivos
FIL fil;      // Arquivo
FRESULT fr;   // Resultado das operações
```

### 3. Montar o cartão SD
```c
fr = f_mount(&fs, "", 1);  // Monta no drive lógico 0
if (fr != FR_OK) {
    printf("Erro ao montar SD: %d\n", fr);
}
```

### 4. Criar ou abrir arquivo
```c
fr = f_open(&fil, "teste.txt", FA_WRITE | FA_CREATE_ALWAYS);
if (fr != FR_OK) {
    printf("Erro ao abrir/criar arquivo\n");
}
```

### 5. Escrever no arquivo
```c
UINT bw;
fr = f_write(&fil, "Hello SD!\r\n", 10, &bw);
if (fr == FR_OK && bw == 10) {
    printf("Escrita concluída!\n");
}
```

### 6. Fechar arquivo
```c
f_close(&fil);
```

### 7. Ler arquivo
```c
char buffer[64];
UINT br;

fr = f_open(&fil, "teste.txt", FA_READ);
if (fr == FR_OK) {
    f_read(&fil, buffer, sizeof(buffer)-1, &br);
    buffer[br] = '\0'; // garante fim de string
    printf("Conteúdo: %s\n", buffer);
    f_close(&fil);
}
```

---

## 🔑 Principais Funções do FatFs

| Função          | Descrição |
|-----------------|-----------|
| `f_mount`       | Monta o sistema de arquivos |
| `f_open`        | Abre ou cria um arquivo |
| `f_write`       | Escreve dados em um arquivo |
| `f_read`        | Lê dados de um arquivo |
| `f_close`       | Fecha o arquivo aberto |
| `f_mkdir`       | Cria um diretório |
| `f_unlink`      | Remove arquivo ou diretório |
| `f_stat`        | Obtém informações sobre arquivo/diretório |
| `f_lseek`       | Move o ponteiro de leitura/escrita do arquivo |

---

## ✅ Exemplo Completo

```c
#include "ff.h"
#include "diskio.h"
#include <stdio.h>

FATFS fs;
FIL fil;
FRESULT fr;

int main() {
    // Montar o SD
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Falha ao montar SD\n");
        return 1;
    }

    // Criar arquivo e escrever
    UINT bw;
    fr = f_open(&fil, "log.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr == FR_OK) {
        f_write(&fil, "Teste no SD!\r\n", 14, &bw);
        f_close(&fil);
    }

    // Ler arquivo
    char buffer[32];
    UINT br;
    fr = f_open(&fil, "log.txt", FA_READ);
    if (fr == FR_OK) {
        f_read(&fil, buffer, sizeof(buffer)-1, &br);
        buffer[br] = '\0';
        printf("Conteúdo: %s\n", buffer);
        f_close(&fil);
    }

    return 0;
}
```

---

## 📌 Observações
- Antes de usar, formate o cartão em **FAT32**.  
- Sempre feche arquivos com `f_close` para garantir integridade.  
- A camada `diskio.c` já cuida da comunicação SPI/SDIO com o cartão.  

# README original da Biblioteca BitDogLab-SDCard
- Labiras → [BitDogLab-SDCard](https://github.com/LabirasIFPI/BitDogLab-SDCard/tree/main)  
- Nícolas Rafael → [EmbarcaTech/KitSensors/SdCard](https://github.com/NicolasRaf/EmbarcaTech/tree/main/KitSensors/SdCard)  


# Módulo de Cartão SD para Raspberry Pi Pico

**Referência da Biblioteca de Sistema de Ficheiros:**

[FatFs - Generic FAT Filesystem Module](https://www.elm-chan.org/fsw/ff)

## 1. Arquitetura e API do Módulo SD Card

### 1.1. Arquitetura de Software em Camadas

O projeto foi desenhado com uma arquitetura de três camadas para separar as responsabilidades e facilitar a manutenção e reutilização:

1. **Módulo de Abstração (`sd_card.c` e `sd_card.h`):**
    - Serve como uma "fachada" ou API (Interface de Programação de Aplicações) simplificada
    - Fornece funções fáceis de usar, como `sd_card_init()` e `sd_card_append_json()`, escondendo a complexidade das camadas inferiores.
2. **Camada de Sistema de Ficheiros (FatFs):**
    - Utiliza a biblioteca **FatFs** (`ff.c`, `ffunicode.c`), uma solução robusta que implementa o sistema de ficheiros FAT/FAT32.
    - É responsável por gerir a estrutura de ficheiros, diretórios e a alocação de espaço no cartão.
3. **Camada de Interface de Disco (`diskio.c`):**
    - É a "ponte" entre a FatFs e o hardware.
    - Implementa as funções de baixo nível para ler e escrever setores físicos de dados no cartão SD, utilizando o barramento SPI do Raspberry Pi Pico.

### 1.2. Descrição da API (Funções Públicas)

A interface do módulo é definida em `inc/sd_card.h` e oferece as seguintes funções:

- **`bool sd_card_init(void);`**
  - **Descrição:** Inicializa a comunicação SPI com o cartão SD e monta o sistema de ficheiros. Esta função deve ser chamada uma vez no início do programa.
  - **Retorna:** `true` se a inicialização for bem-sucedida, `false` caso contrário.
- **`sd_status_t sd_card_write_text(const char* filename, const char* text);`**
  - **Descrição:** Escreve (anexa) uma string de texto simples ao final de um ficheiro. Se o ficheiro não existir, ele é criado. Ideal para logs.
  - **Parâmetros:**
    - `filename`: O nome do ficheiro a ser escrito (ex: "log.txt").
    - `text`: O conteúdo a ser salvo.
  - **Retorna:** `SD_OK` em caso de sucesso, ou um código de erro.
- **`sd_status_t sd_card_append_json(const char* filename, const char* json_string);`**
  - **Descrição:** Anexa um objeto JSON a um array num ficheiro, mantendo a formatação JSON válida. Gere automaticamente a adição de `[` `]` e vírgulas.
  - **Parâmetros:**
    - `filename`: O nome do ficheiro JSON (ex: "dados.json").
    - `json_string`: A string do objeto JSON a ser salvo (ex: `"{\"valor\":123}"`).
  - **Retorna:** `SD_OK` em caso de sucesso, ou um código de erro.
- **`sd_status_t sd_card_read_text(const char* filename, char* buffer, int buffer_size);`**
  - **Descrição:** Lê o conteúdo completo de um ficheiro (seja texto ou JSON) para um buffer de memória.
  - **Parâmetros:**
    - `filename`: O nome do ficheiro a ser lido.
    - `buffer`: Buffer para armazenar o conteúdo lido.
    - `buffer_size`: O tamanho máximo do buffer.
  - **Retorna:** `SD_OK` em caso de sucesso, ou um código de erro.

## 2. Guia de Utilização e Integração

Esta secção é um guia prático para integrar o módulo de cartão SD num novo projeto do Raspberry Pi Pico.

### 2.1. Conexão de Hardware

O módulo utiliza a interface SPI. A conexão entre o Raspberry Pi Pico e um módulo de leitor de cartão SD padrão deve seguir a pinagem definida no projeto:

| **Componente (Módulo SD)** | **Pino no Pico** | **GPIO** | **Observação** |
| --- | --- | --- | --- |
| **VCC** | 3V3 (OUT) | `Pino 36` | Tensão de alimentação de 3.3V para o módulo. |
| **GND** | GND | `Pino 38` | O terra (GND) deve ser comum entre o Pico e o módulo. |
| **MISO** (DO) | SPI0 RX | `GPIO 16` | Master In, Slave Out. |
| **MOSI** (DI) | SPI0 TX | `GPIO 19` | Master Out, Slave In. |
| **SCK** (CLK) | SPI0 SCK | `GPIO 18` | Pino de clock do barramento SPI. |
| **CS** | - | `GPIO 17` | Chip Select. Essencial para selecionar o dispositivo. |

> **Ponto Importante:** Se optar por utilizar um conjunto de pinos diferente, é imprescindível que atualize as definições de pinagem no ficheiro `diskio.h` para garantir o funcionamento correto.mCertifique-se de que os GPIOs escolhidos para MISO, MOSI e SCK correspondem a um periférico de hardware SPI válido no Pico (por exemplo, `SPI0` ou `SPI1`).

### 2.2. Integração de Software

Para utilizar este módulo no seu próprio projeto, siga os passos abaixo.

### Configurar o Ficheiro `CMakeLists.txt`

Adicione os ficheiros fonte do módulo e o diretório de cabeçalhos (`inc`) ao seu `CMakeLists.txt` para que o compilador os encontre.

``` Cmake
# Adicione os ficheiros .c à sua lista de executáveis
add_executable(seu_projeto
    main.c
    sd_card.c
    diskio.c
    ff.c
    ffunicode.c
)

# Certifique-se de que a biblioteca hardware_spi está ligada
target_link_libraries(seu_projeto pico_stdlib hardware_spi)
```

### Utilizar o Módulo no `main.c`

Agora, no seu `main.c`, inclua o cabeçalho do módulo e utilize as suas funções, como demonstrado no [projeto de exemplo](https://github.com/LabirasIFPI/SDCard_PicoW-EmbarcaTech/blob/831d0cc8abd954cd3e133a29194af6e9af786c14/main.c).

## Documentações Relacionados

- <https://elm-chan.org/docs/mmc/mmc_e.html>
- <https://github.com/igordev23/Sd-card-bitdog>
- <http://www.technoblogy.com/show?3XEP>

> Documentação por [@Nicolas Rafael](https://github.com/NicolasRaf)
