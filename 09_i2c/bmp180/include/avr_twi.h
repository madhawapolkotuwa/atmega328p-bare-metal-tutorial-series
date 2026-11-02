#ifndef  AVR_TWI_H
#define  AVR_TWI_H

#include <avr/io.h>

void twi_init(void) {
    TWSR = 0x00;                              // prescaler = 1
    TWBR = ((F_CPU / 100000UL) - 16) / 2;     // 100kHz SCL
    TWCR = (1 << TWEN);                        // enable TWI
}

void twi_start(void) {
    TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));   // wait for start condition to complete
}

void twi_stop(void) {
    TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

void twi_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));   // wait for byte to be shifted out + ACK/NACK received
}

uint8_t twi_read_ack(void) {
    TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);   // read + send ACK (more bytes wanted)
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t twi_read_nack(void) {
    TWCR = (1 << TWEN) | (1 << TWINT);   // read + send NACK (last byte)
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

#endif /* AVR_TWI_H */