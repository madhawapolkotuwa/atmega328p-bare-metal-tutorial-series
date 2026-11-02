#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void) {
    // 1. Set Baud Rate 
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);

    // 2. Set Frame Format: 8-bit Data, 1 Stop Bit, No Parity (8N1)
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    
    // 3. Enable Receiver and Transmitter
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
}

void uart_putchar(char data) {
    // Wait for empty transmit buffer (UDRE0 flag is set to 1 when empty)
    while (!(UCSR0A & (1 << UDRE0)));  

    // Put data into buffer, sends the data
    UDR0 = data;
}

char usart_receive(void) {
    // Wait for data to be received (RXC0 flag is set to 1 when there's data)
    while (!(UCSR0A & (1 << RXC0)));
    
    // Get and return received data from buffer
    return UDR0;
}

void uart_print(const char *str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

int main(void) {
    uart_init();

    uart_print("Hello from ATmega328P!\r\n");

    while (1) {
        char c = usart_receive();
        uart_putchar(c);
    }
}