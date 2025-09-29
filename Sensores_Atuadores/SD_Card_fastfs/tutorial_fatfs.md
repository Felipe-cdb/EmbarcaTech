# Tutorial de Uso da Biblioteca FatFs no Raspberry Pi Pico  

**Adaptada e referenciada dos seguintes autores:**  
- Labiras → [BitDogLab-SDCard](https://github.com/LabirasIFPI/BitDogLab-SDCard/tree/main)  
- Nícolas Rafael → [EmbarcaTech/KitSensors/SdCard](https://github.com/NicolasRaf/EmbarcaTech/tree/main/KitSensors/SdCard)  

---

## 📂 Introdução
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
