#include <avr/io.h>
#include <avr/interrupt.h>

ISR(INT0_vect) {

    PORTB ^= (1 << PORTB5);   // toggle LED

}

int main(void){

    DDRB |= (1 << DDB5); // PB5 = output (LED) 
    DDRD &= ~(1 << DDD2); // PD2 = input (button)  

    EICRA |= (1 << ISC00) | (1 << ISC01);  // ISC01:ISC00 = 11 -> rising only
    EIMSK |= (1 << INT0);   // enable INT0

    sei();                     // enable global interrupts

    while(1){
        // CPU free - LED updates happen in the ISR
    }

    return 0;
}