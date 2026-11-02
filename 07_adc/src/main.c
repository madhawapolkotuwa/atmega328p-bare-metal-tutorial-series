#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);
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

void adc_init(void) {
    ADMUX  = (1 << REFS0); // reference = AVCC (5V), channel = ADC0 (A0)
    ADCSRA = (1 << ADEN)   // enable ADC
        | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // // prescaler = 128 -> 125kHz ADC clock
}

uint16_t adc_read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // select channel, keep reference bits
    ADCSRA |= (1 << ADSC);                     // start conversion
    while (ADCSRA & (1 << ADSC));              // wait for conversion to finish
    return ADC;                                // 10-bit result (0-1023)
}

int main(void) {
    adc_init();
    uart_init();
    stdout = &uart_output;

    printf("ADC monitor started.\n");

    while (1) {
        uint16_t value = adc_read(0); // read A0
        printf("A0 = %u\n", value);

        _delay_ms(100);
    }
}