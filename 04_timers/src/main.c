#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER1_COMPA_vect) {
    PORTB ^= (1 << PORTB5);   // toggle LED (D13)
}

int main(void) {
    DDRB |= (1 << DDB5);      // D13 = output

    TCCR1B |= (1 << WGM12);               // CTC mode (mode 4: top = OCR1A)
    TCCR1B |= (1 << CS12) | (1 << CS10);  // prescaler = 1024
    
    // (16-bit)
    OCR1A = 15624;                        // compare value for 1 Hz
    TIMSK1 |= (1 << OCIE1A);              // enable Timer1 compare-match A interrupt

    sei();

    while (1) {
        // CPU free
    }
}