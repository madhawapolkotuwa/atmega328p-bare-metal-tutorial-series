#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "avr_uart.h"

volatile uint8_t wdt_fired = 0;

ISR(WDT_vect) {
    wdt_fired = 1;
    // Note: in interrupt-only mode (WDE=0, WDIE=1), the watchdog does NOT reset the chip -
    // it just fires this ISR once, then WDIE is auto-cleared by hardware (falls back to reset mode
    // if WDE is still set, or disables itself if not) - see step 6 for "interrupt + reset" mode

}

void wdt_interrupt_init(void) {
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP1) | (1 << WDP0) | (1 << WDP2);   // 2s, interrupt mode (WDE=0 : No Reset)
    // WDTCSR = (1 << WDIE) | (1 << WDE) | (1 << WDP1) | (1 << WDP0) | (1 << WDP2); // "Interrupt + reset" mode
    sei();
}

int main(void) {
    uart_init();
    stdout = &uart_output;

    printf("Watchdog interrupt test starting...\n");
    wdt_interrupt_init();

    while (1) {
        if (wdt_fired) {
            wdt_fired = 0;
            // handle periodic wake/warning here
            printf("wdt_fired...\n");
        }
    }
}