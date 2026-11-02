#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
#include <stdio.h>

#include "avr_twi.h"
#include "bmp180.h"
#include "avr_uart.h"

#define BMP180_ADDR 0x77
#define F_CPU 16000000UL

int main(void) {
    twi_init();
    uart_init();
    stdout = &uart_output;

    bmp180_read_calibration();
    printf("BMP180 calibration loaded.\n");

    while (1) {
        uint16_t ut = bmp180_read_raw_temp();
        uint32_t up = bmp180_read_raw_pressure(0);   // OSS = 0

        int32_t b5;
        int32_t temp_decidegC = bmp180_true_temp_and_b5(ut, &b5);
        int32_t pressure_pa = bmp180_true_pressure(up, b5, 0);

        printf("Temp: %ld.%ld C   Pressure: %ld Pa\n",
               temp_decidegC / 10, temp_decidegC % 10, pressure_pa);

        _delay_ms(1000);
    }
}

