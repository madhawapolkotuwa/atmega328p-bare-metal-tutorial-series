#ifndef BMP180_H
#define BMP180_H

#include "avr_twi.h"
#include <util/twi.h>
#include <util/delay.h>

#define BMP180_ADDR 0x77

typedef struct {
    int16_t  ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t  b1, b2;
    int16_t  mb, mc, md;
} bmp180_calib_t;

bmp180_calib_t calib;

uint16_t bmp180_read_u16(uint8_t reg) {
    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_WRITE);
    twi_write(reg);
    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_READ);
    uint8_t msb = twi_read_ack();
    uint8_t lsb = twi_read_nack();
    twi_stop();
    return ((uint16_t)msb << 8) | lsb;
}

void bmp180_read_calibration(void) {
    calib.ac1 = (int16_t)bmp180_read_u16(0xAA);
    calib.ac2 = (int16_t)bmp180_read_u16(0xAC);
    calib.ac3 = (int16_t)bmp180_read_u16(0xAE);
    calib.ac4 = bmp180_read_u16(0xB0);
    calib.ac5 = bmp180_read_u16(0xB2);
    calib.ac6 = bmp180_read_u16(0xB4);
    calib.b1  = (int16_t)bmp180_read_u16(0xB6);
    calib.b2  = (int16_t)bmp180_read_u16(0xB8);
    calib.mb  = (int16_t)bmp180_read_u16(0xBA);
    calib.mc  = (int16_t)bmp180_read_u16(0xBC);
    calib.md  = (int16_t)bmp180_read_u16(0xBE);
}

void bmp180_write_reg(uint8_t reg, uint8_t value) {
    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_WRITE);
    twi_write(reg);
    twi_write(value);
    twi_stop();
}

uint16_t bmp180_read_raw_temp(void) {
    bmp180_write_reg(0xF4, 0x2E);   // start temperature measurement
    _delay_ms(5);                    // datasheet: 4.5ms max conversion time
    return bmp180_read_u16(0xF6);
}

uint32_t bmp180_read_raw_pressure(uint8_t oss) {
    bmp180_write_reg(0xF4, 0x34 + (oss << 6));   // start pressure measurement at given OSS
    _delay_ms(5);                                   // OSS=0 needs ~4.5ms; higher OSS needs longer (see table below)

    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_WRITE);
    twi_write(0xF6);
    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_READ);
    uint8_t msb  = twi_read_ack();
    uint8_t lsb  = twi_read_ack();
    uint8_t xlsb = twi_read_nack();
    twi_stop();

    uint32_t raw = ((uint32_t)msb << 16) | ((uint32_t)lsb << 8) | xlsb;
    return raw >> (8 - oss);
}

int32_t bmp180_true_temp_and_b5(uint16_t ut, int32_t *b5_out) {
    int32_t x1 = ((int32_t)(ut - calib.ac6) * calib.ac5) >> 15;
    int32_t x2 = ((int32_t)calib.mc << 11) / (x1 + calib.md);
    int32_t b5 = x1 + x2;
    *b5_out = b5;

    int32_t temp = (b5 + 8) >> 4;   // in units of 0.1 degC
    return temp;
}

int32_t bmp180_true_pressure(uint32_t up, int32_t b5, uint8_t oss) {
    int32_t b6 = b5 - 4000;

    int32_t x1 = (calib.b2 * (b6 * b6 >> 12)) >> 11;
    int32_t x2 = (calib.ac2 * b6) >> 11;
    int32_t x3 = x1 + x2;
    int32_t b3 = (((((int32_t)calib.ac1 * 4) + x3) << oss) + 2) >> 2;

    x1 = (calib.ac3 * b6) >> 13;
    x2 = (calib.b1 * (b6 * b6 >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    uint32_t b4 = (calib.ac4 * (uint32_t)(x3 + 32768)) >> 15;

    uint32_t b7 = ((uint32_t)(up - b3)) * (50000 >> oss);
    int32_t p;
    if (b7 < 0x80000000) {
        p = (b7 * 2) / b4;
    } else {
        p = (b7 / b4) * 2;
    }

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);   // final pressure in Pa

    return p;
}

#endif /* BMP180_H */
