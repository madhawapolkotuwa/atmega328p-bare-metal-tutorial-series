#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (8UL * BAUD)) - 1)

#define SS_PIN PB2   // D10

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

void spi_slave_init(void) {
    DDRB &= ~((1 << DDB3) | (1 << DDB5) | (1 << DDB2));  // MOSI, SCK, SS = inputs
    DDRB |= (1 << DDB4);                                 // MISO = output

    SPCR = (1 << SPE);   // enable SPI, slave mode (no MSTR)

    SPDR = 0x34;         // preload first response byte 
}

int main(void) {
    spi_slave_init();

    uart_init();
    stdout = &uart_output;

    while (1) {
        while (!(SPSR & (1 << SPIF)));   // wait for master to complete a transfer
        uint8_t received = SPDR;          // byte the master sent

        // ... do something with 'received' ...
        printf("received: 0x%x\n", received);

        SPDR = 0x34;                       // preload next response byte for the following transfer
    }
}