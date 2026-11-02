#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdio.h>

#include "avr_uart.h"

#define BOOT_COUNT_ADDR 0

int main(void) {
    uart_init();
    stdout = &uart_output;

    uint32_t boot_count = eeprom_read_dword((uint32_t*)BOOT_COUNT_ADDR);

    // Handle first-ever boot: fresh/erased EEPROM reads as 0xFFFFFFFF
    if (boot_count == 0xFFFFFFFF) {
        boot_count = 0;
    }

    boot_count++;
    eeprom_write_dword((uint32_t*)BOOT_COUNT_ADDR, boot_count);

    printf("Boot count: %lu\n", boot_count);

    while (1) {
        // idle
    }
}