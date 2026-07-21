#include <avr/io.h>

#ifdef WITHOUT_UTIL_DELAY
#else
#include <util/delay.h>
#endif

#ifdef WITHOUT_UTIL_DELAY
#define F_CPU 16000000UL // 16MHz
#define CYCLES_PER_LOOP 9

void delay_cycles(uint16_t cycles){
    while(cycles--){
        __asm__ __volatile__("nop");
    }
}

void delay_ms(uint16_t ms){
    while(ms--){
        delay_cycles(F_CPU / 1000UL / CYCLES_PER_LOOP);
    }
}
#endif

int main(void){
    DDRB = DDRB | ( 1 << DDB5 ); // DDRB |= (1 << DDB5); 

    while(1){

        // LED ON
        PORTB = PORTB | (1 << PORTB5); // PORTB |= (1 << PORTB5); // 0b00100000

#ifdef WITHOUT_UTIL_DELAY
        delay_ms(1000);  // 1s
#else
        _delay_ms(1000); // 1s
#endif
        // LED OFF
        PORTB = PORTB & ~(1 << PORTB5); // PORTB &= ~(1 << PORTB5); // 0b00000000

#ifdef WITHOUT_UTIL_DELAY
        delay_ms(1000); // 1s
#else
        _delay_ms(1000); // 1s
#endif

    }
}