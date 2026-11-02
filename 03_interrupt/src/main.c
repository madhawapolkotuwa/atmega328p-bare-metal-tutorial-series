#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t last_state = 0;

ISR(PCINT0_vect) {
    uint8_t current_state = PINB & (1 << PINB4);

    if (current_state && !last_state) {
        // was LOW, now HIGH -> rising edge (button just pressed)
        PORTB ^= (1 << PORTB5);   // toggle LED
    }

    last_state = current_state;
}

int main(void) {
    DDRB &= ~(1 << DDB4);   // PB4 = input (button)
    DDRB |= (1 << DDB5);    // PB5 = output (LED)

    PCICR  |= (1 << PCIE0);    // enable pin-change interrupts for PORTB
    PCMSK0 |= (1 << PCINT4);   // enable mask bit for PB4

    sei();                     // enable global interrupts

    while (1) {
        // CPU free — LED updates happen in the ISR
    }
}