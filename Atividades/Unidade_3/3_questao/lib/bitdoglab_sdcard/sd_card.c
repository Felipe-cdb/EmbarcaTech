/*
* Biblioteca obitida das seguintes fontes:
* - Labiras → [BitDogLab-SDCard]
* (https://github.com/LabirasIFPI/BitDogLab-SDCard/tree/main)  
* - Nícolas Rafael → [EmbarcaTech/KitSensors/SdCard]
* (https://github.com/NicolasRaf/EmbarcaTech/tree/main/KitSensors/SdCard)  
*/

#include "sd_card.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"

// Variável estática apenas para o sistema de arquivos.
static FATFS fs;

// Função de tempo exigida pelo FatFs
DWORD get_fattime(void) {
    return ((DWORD)(2025 - 1980) << 25) | ((DWORD)7 << 21) | ((DWORD)9 << 16);
}

// Função de inicialização 
bool sd_card_init(void) {
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("❌ Falha ao montar o SD Card (erro FatFs: %d)\n", fr);
        return false;
    }
    printf("✅ SD Card montado e pronto.\n");
    return true;
}

// Função para escrever texto simples
sd_status_t sd_card_write_text(const char* filename, const char* text) {
    FIL fil; 
    
    FRESULT fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_ALWAYS | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("❌ Erro ao abrir '%s' (erro: %d)\n", filename, fr);
        return SD_OPEN_FAILED;
    }

    f_write(&fil, text, strlen(text), NULL); // O número de bytes escritos pode ser NULL se não for verificado
    
    // Força a escrita do buffer para o cartão
    f_sync(&fil);
    
    f_close(&fil);
    
    printf("✅ Texto anexado em '%s'\n", filename);
    return SD_OK;
}

// Função para anexar JSON
sd_status_t sd_card_append_json(const char* filename, const char* json_string) {
    FIL fil; 
    FRESULT fr;
    UINT bw;

    fr = f_open(&fil, filename, FA_READ | FA_WRITE | FA_OPEN_ALWAYS);
    if (fr != FR_OK) {
        printf("❌ Erro ao abrir/criar o arquivo JSON '%s' (erro: %d)\n", filename, fr);
        return SD_OPEN_FAILED;
    }

    if (f_size(&fil) == 0) { // Arquivo novo, cria a estrutura do array
        f_write(&fil, "[\n", 2, &bw);         // Abre o array
        f_write(&fil, "\t", 1, &bw);         // Adiciona a primeira indentação
        f_write(&fil, json_string, strlen(json_string), &bw); // Escreve o objeto
        f_write(&fil, "\n]", 2, &bw);         // Fecha o array
    } else { // Arquivo existente, anexa um novo objeto
        f_lseek(&fil, f_size(&fil) - 1);  // Move o cursor para antes do ']' final
        f_write(&fil, ",\n", 2, &bw);        // Adiciona uma vírgula e uma nova linha
        f_write(&fil, "\t", 1, &bw);         // Adiciona a indentação para o novo objeto
        f_write(&fil, json_string, strlen(json_string), &bw); // Escreve o novo objeto
        f_write(&fil, "\n]", 2, &bw);        // Fecha o array na nova linha
    }
    
    f_sync(&fil);
    f_close(&fil);
    
    printf("✅ Objeto JSON adicionado em '%s'\n", filename);
    return SD_OK;
}

// Função de leitura generica
sd_status_t sd_card_read(const char* filename, char* buffer, int buffer_size) {
    FIL fil; 
    
    FRESULT fr;
    UINT br;

    memset(buffer, 0, buffer_size);
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("❌ Falha ao ler '%s' (erro: %d)\n", filename, fr);
        return SD_OPEN_FAILED;
    }

    f_read(&fil, buffer, buffer_size - 1, &br);
    f_close(&fil);
    
    printf("📄 Conteúdo de '%s':\n------------------\n%s\n------------------\n", filename, buffer);
    return SD_OK;
}