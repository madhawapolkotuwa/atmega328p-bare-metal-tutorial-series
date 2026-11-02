#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "avr_uart.h"

ISR(INT0_vect) {
    // this ISR runs on wake - can be empty, its only job here is to end the sleep instruction
    printf("ISR..\n");
}

void enter_sleep_hfn(void) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

void enter_sleep(void) {
    SMCR = (1 << SM1) | (1 << SE);   // SM2:0 = 010 (power-down), SE = 1 (arm sleep)
    
    __asm__ __volatile__("sleep");   // CPU halts here until an interrupt fires

    SMCR &= ~(1 << SE);               // disarm sleep immediately after waking
}

int main(void) {

    uart_init();
    stdout = &uart_output;

    DDRD &= ~(1 << DDD2);   // PD2 = input (button)
    DDRB |= (1 << DDB5);    // PB5 = output (LED, for visual confirmation)

    
    EICRA |= (1 << ISC01) | (1 << ISC00);   // ISC01:ISC00 = 11 -> rising edge (adjust based on your button wiring)
    EIMSK |= (1 << INT0);    // enable INT0

    sei();

    printf("re-init...\n");

    while (1) {
        PORTB |= (1 << PORTB5);    // LED on = awake
        _delay_ms(1000);  // delay so LED-on is visible

        PORTB &= ~(1 << PORTB5);   // LED off = about to sleep
        enter_sleep();               // board powers down here until button press wakes it

        // execution resumes here after waking
        //printf("execution resumes...\n");
    }
}