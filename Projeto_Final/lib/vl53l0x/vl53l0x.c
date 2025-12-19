// Adaptada por Felipe Correia das seguintes bibliotecas 
// Baseada na biblioteca TESTE_vl53l0x_BITDOGLAB por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_vl53l0x_BITDOGLAB 

#include "vl53l0x.h"

// --- Funções Helper de Baixo Nível I2C ---

static void write_reg(VL53L0X* sensor, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(sensor->i2c, sensor->address, buf, 2, false);
}

static void write_reg16(VL53L0X* sensor, uint8_t reg, uint16_t val) {
    uint8_t buf[3] = {reg, (val >> 8), (val & 0xFF)};
    i2c_write_blocking(sensor->i2c, sensor->address, buf, 3, false);
}

static uint8_t read_reg(VL53L0X* sensor, uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(sensor->i2c, sensor->address, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->address, &val, 1, false);
    return val;
}

static uint16_t read_reg16(VL53L0X* sensor, uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(sensor->i2c, sensor->address, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->address, buf, 2, false);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

// --- Funções Públicas ---
static void vl53l0x_setup_i2c(i2c_inst_t *i2c, uint8_t sda, uint8_t scl){
    // Inicializa o I2C. 100kHz é uma velocidade segura para depuração.
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

bool vl53l0x_init(VL53L0X* sensor, i2c_inst_t* i2c_port, uint8_t sda, uint8_t scl) {
    return vl53l0x_init_custom(sensor, i2c_port, sda, scl, VL53L0X_ADDRESS);
}

bool vl53l0x_init_custom(VL53L0X* sensor, i2c_inst_t* i2c_port, uint8_t sda, uint8_t scl, uint8_t address) {
    sensor->i2c = i2c_port;
    sensor->sda = sda;
    sensor->scl = scl;
    sensor->address = address;
    sensor->io_timeout = 1000; // Timeout de 1 segundo para operações.

    // Configura pinos e inicializa I2C
    vl53l0x_setup_i2c(sensor->i2c, sensor->sda, sensor->scl);

    // A sequência abaixo é uma implementação complexa e específica do VL53L0X,
    // necessária para calibrar e configurar corretamente o sensor.
    // É baseada na API oficial da ST e em drivers de código aberto (ex: Pololu, Adafruit).

    // "Acorda" o sensor e salva a variável de parada.
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(sensor, 0xFF, 0x01);
    write_reg(sensor, SYSRANGE_START, 0x00);
    sensor->stop_variable = read_reg(sensor, 0x91);
    write_reg(sensor, SYSRANGE_START, 0x01);
    write_reg(sensor, 0xFF, 0x00);
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Configurações de tuning e limites.
    write_reg(sensor, MSRC_CONFIG_CONTROL, read_reg(sensor, MSRC_CONFIG_CONTROL) | 0x12);
    write_reg16(sensor, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)(0.25 * (1 << 7)));
    write_reg(sensor, SYSTEM_SEQUENCE_CONFIG, 0xFF);

    // Calibração de SPAD (Single Photon Avalanche Diode).
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01); write_reg(sensor, 0xFF, 0x01);
    write_reg(sensor, SYSRANGE_START, 0x00); write_reg(sensor, 0xFF, 0x06);
    write_reg(sensor, 0x83, read_reg(sensor, 0x83) | 0x04);
    write_reg(sensor, 0xFF, 0x07); write_reg(sensor, 0x81, 0x01);
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01); write_reg(sensor, 0x94, 0x6b);
    write_reg(sensor, 0x83, 0x00);
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (read_reg(sensor, 0x83) == 0x00) {
        if (to_ms_since_boot(get_absolute_time()) - start > sensor->io_timeout) return false;
    }
    write_reg(sensor, 0x83, 0x01);
    read_reg(sensor, 0x92);
    write_reg(sensor, 0x81, 0x00); write_reg(sensor, 0xFF, 0x06);
    write_reg(sensor, 0x83, read_reg(sensor, 0x83) & ~0x04);
    write_reg(sensor, 0xFF, 0x01); write_reg(sensor, SYSRANGE_START, 0x01);
    write_reg(sensor, 0xFF, 0x00); write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // Configuração da interrupção (não usada ativamente, mas parte da sequência).
    write_reg(sensor, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    write_reg(sensor, GPIO_HV_MUX_ACTIVE_HIGH, read_reg(sensor, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    write_reg(sensor, SYSTEM_INTERRUPT_CLEAR, 0x01);

    // Configuração do timing budget (orçamento de tempo por medição).
    sensor->measurement_timing_budget_us = 33000; // 33ms é um bom valor padrão.
    write_reg(sensor, SYSTEM_SEQUENCE_CONFIG, 0xE8);
    write_reg16(sensor, 0x04, 33000 / 1085); // Valor de aproximação para o período.

    write_reg(sensor, SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    return true;
}

uint16_t vl53l0x_read_single_mm(VL53L0X* sensor) {
    // Sequência de trigger para medição única.
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(sensor, 0xFF, 0x01); write_reg(sensor, SYSRANGE_START, 0x00);
    write_reg(sensor, 0x91, sensor->stop_variable); write_reg(sensor, SYSRANGE_START, 0x01);
    write_reg(sensor, 0xFF, 0x00); write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
    write_reg(sensor, SYSRANGE_START, 0x01);

    // Espera o sensor ficar pronto.
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (read_reg(sensor, SYSRANGE_START) & 0x01) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > sensor->io_timeout) return 65535;
    }

    // Espera o dado estar disponível.
    start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(sensor, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > sensor->io_timeout) return 65535;
    }

    uint16_t range = read_reg16(sensor, RESULT_RANGE_MM);
    write_reg(sensor, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}

uint16_t vl53l0x_read_single_cm(VL53L0X* sensor) {
    uint16_t range_mm = vl53l0x_read_single_mm(sensor);
    return range_mm / 10; // Converte para centímetros
}


void vl53l0x_start_continuous(VL53L0X* sensor, uint32_t period_ms) {
    write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
    write_reg(sensor, 0xFF, 0x01); write_reg(sensor, SYSRANGE_START, 0x00);
    write_reg(sensor, 0x91, sensor->stop_variable); write_reg(sensor, SYSRANGE_START, 0x01);
    write_reg(sensor, 0xFF, 0x00); write_reg(sensor, POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    if (period_ms != 0) {
        // Modo contínuo com intervalo programado.
        write_reg16(sensor, SYSTEM_INTERMEASUREMENT_PERIOD, period_ms * 12 / 13);
        write_reg(sensor, SYSRANGE_START, 0x04);
    } else {
        // Modo contínuo mais rápido possível ("back-to-back").
        write_reg(sensor, SYSRANGE_START, 0x02);
    }
}

uint16_t vl53l0x_read_continuous_mm(VL53L0X* sensor) {
    // Espera pelo flag de "dado pronto".
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while ((read_reg(sensor, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if ((to_ms_since_boot(get_absolute_time()) - start) > sensor->io_timeout) return 65535;
    }
    uint16_t range = read_reg16(sensor, RESULT_RANGE_MM);
    write_reg(sensor, SYSTEM_INTERRUPT_CLEAR, 0x01);
    return range;
}

uint16_t vl53l0x_read_continuous_cm(VL53L0X* sensor) {
    uint16_t range_mm = vl53l0x_read_continuous_mm(sensor);
    return range_mm / 10; // Converte para centímetros
}
