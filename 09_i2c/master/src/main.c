#include <avr/io.h>
#include <util/twi.h>
#include <stdio.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (8UL * BAUD)) - 1)

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);

    UCSR0A |= (1 << U2X0);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

static FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);


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

#define BMP180_ADDR 0x77

uint8_t bmp180_read_reg(uint8_t reg) {
    twi_start();
    twi_write((BMP180_ADDR << 1) | TW_WRITE);       // address + write bit
    twi_write(reg);                                 // register we want to read
    twi_start();                                    // repeated start
    twi_write((BMP180_ADDR << 1) | TW_READ);        // address + read bit
    uint8_t value = twi_read_nack();                // read 1 byte, NACK (last byte)
    twi_stop();
    return value;
}

int main(void) {

    uart_init();
    stdout = &uart_output;

    twi_init();

    uint8_t chip_id = bmp180_read_reg(0xD0);
    // chip_id should be 0x55 if communication is working

    printf("chip_id: 0x%x\n", chip_id);
    while (1) {
        // idle - use UART/printf from earlier to print chip_id if you want to see it
        printf("chip_id: 0x%x\n", chip_id);
        _delay_ms(1000);
    }
}