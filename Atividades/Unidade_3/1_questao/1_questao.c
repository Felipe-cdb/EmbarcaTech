#include <stdio.h>
#include "pico/stdlib.h"
#include "bh1750/bh1750.h"
#include "fs90r/fs90r.h"    // funções da biblioteca FS90R

#include "hardware/pwm.h"
#include "hardware/i2c.h"

int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
