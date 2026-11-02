#include <avr/io.h>
#include <util/delay.h>

void pwm_init(void) {
    // --- Red: Timer1, OC1A (D9) ---
    DDRB |= (1 << DDB1);
    TCCR1A |= (1 << WGM10);              // Fast PWM, 8-bit (mode 5, TOP = 0xFF)
    TCCR1B |= (1 << WGM12);
    TCCR1A |= (1 << COM1A1);             // non-inverting
    TCCR1B |= (1 << CS11);                // prescaler = 8

    // --- Green: Timer2, OC2A (D11) ---
    DDRB |= (1 << DDB3);
    TCCR2A |= (1 << WGM21) | (1 << WGM20); // Fast PWM, TOP = 0xFF
    TCCR2A |= (1 << COM2A1);                // non-inverting
    TCCR2B |= (1 << CS22);                   // prescaler = 64

    // --- Blue: Timer0, OC0A (D6) ---
    DDRD |= (1 << DDD6);
    TCCR0A |= (1 << WGM01) | (1 << WGM00); // Fast PWM, TOP = 0xFF
    TCCR0A |= (1 << COM0A1);                // non-inverting
    TCCR0B |= (1 << CS01);                   // prescaler = 8
}

void set_color(uint8_t r, uint8_t g, uint8_t b) {
    OCR1A = r;   // Red duty cycle   (0-255)
    OCR2A = g;   // Green duty cycle (0-255)
    OCR0A = b;   // Blue duty cycle  (0-255)
}

int main(void) {
    pwm_init();

    set_color(0, 0, 0);   // pure red

    while (1) {
        // static color; try changing set_color() args above
        // set_color(255, 0 , 0);
        // _delay_ms(100);

        // set_color(0, 255 , 0);
        // _delay_ms(100);

        // set_color(0, 0 , 255);
        // _delay_ms(100);

        // set_color(0, 128 , 128);
        // _delay_ms(100);

        // set_color(128, 0 , 128);
        // _delay_ms(100);

        // set_color(255, 255 , 255);
        // _delay_ms(100);
    }
}