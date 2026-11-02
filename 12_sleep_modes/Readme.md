# Sleep Mode

configuring the Sleep Mode Control Register (`SMCR`) combined with an external interrupt allows you to put the chip into its lowest power-saving state (Power-down) and wake it up using a physical signal.

### 1. Direct register access.

#### Step 1: Select the Sleep Mode and Enable Sleep (`SMCR`)

The Sleep Mode Control Register (`SMCR`) is the hardware dashboard for power management. It determines what type of sleep the microcontroller enters when the SLEEP assembly instruction is executed.

* Select the Mode (`SM2:SM0` - **Bits 3:1**):
    These three bits configure which of the six internal sleep levels is initiated.
    * To select Power-down Mode (which turns off the CPU, flash clock, system oscillators, and all hardware timers except for asynchronous events), write 010 to these bits (`SM2 = 0`, `SM1 = 1`, `SM0 = 0`).

* Enable Sleep Commands (`SE` - `Bit 0`):
    The Sleep Enable (`SE`) bit is a safety gate. When `SE` is `0`, the **CPU** will ignore any SLEEP instruction. Set `SE = 1` immediately before you plan to put the processor to sleep, and clear it back to `0` immediately after waking up to prevent accidental sleep execution from rogue software.

* Combined Action: Write `0x05` (binary `0b00000101`) to `SMCR` to choose Power-down and enable sleep.

#### Step 2: Configure the Wake-Up Source (External Interrupt `INT0`)
To wake up from the deep Power-down state, you need an active external hardware interrupt. We will use the `INT0` physical pin (`PD2`) for this example.
1. Define the Trigger Condition (`EICRA` - External Interrupt Control Register A):
    This register determines what electrical signal behavior on the physical pin triggers the interrupt:
    * According to the datasheet, to wake up from deeper sleep modes like Power-down, the hardware requires a low-level detection trigger.
    * To set `INT0` to trigger on a low level, clear the `ISC01` and `ISC00` bits (**Bits 1:0**) in `EICRA` to `00`.

2. Enable the Pin Interrupt (`EIMSK` - External Interrupt Mask Register):
Setting the External Interrupt Request `0` Enable (`INT0` - **Bit 0**) bit to `1` tells the hardware to actively listen to the `PD2` pin and route its trigger to the processor.

#### Step 3: Enable Global Interrupts (`SREG`)

Even if the individual pin interrupt is enabled, the CPU core will ignore it unless the global interrupt switch is on.

* Action: Set the Global Interrupt Enable (I-bit) in the Status Register (`SREG`) to `1`. (This is typically done in C using the `sei()` macro).

#### Step 4: Execute the Sleep Instruction
Once the safety gate `SE` is open, the interrupts are configured, and the global interrupt enable is active, you can safely power down the processor.

* **Action**: Execute the inline assembly instruction sleep.
* **What happens next**: The system clock halts, and power consumption drops to less than $1 \mu\text{A}$.

#### Step 5: Handling the Wake-Up Sequence
When an external device pulls the `INT0` (`PD2`) pin low, the hardware wakes the internal oscillators:

1. The chip holds the CPU for a short start-up delay to let the clock stabilize.
2. The CPU immediately jumps to execute the corresponding `INT0` Interrupt Service Routine (`ISR`).
3. Once the `ISR` finishes, execution resumes at the very next line of code directly following your original sleep instruction.
4. **Action**: The first line of code after waking up should immediately write `SE = 0` inside `SMCR` to close the sleep gate.

```c
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(INT0_vect) {
    // this ISR runs on wake - can be empty, its only job here is to end the sleep instruction
}

void enter_sleep(void) {
    SMCR = (1 << SM1) | (1 << SE);   // SM2:0 = 010 (power-down), SE = 1 (arm sleep)
    sei();
    __asm__ __volatile__("sleep");   // CPU halts here until an interrupt fires

    SMCR &= ~(1 << SE);               // disarm sleep immediately after waking
}

int main(void){
    
    DDRD &= ~(1 << DDD2);   // PD2 = input (button)
    DDRB |= (1 << DDB5);    // PB5 = output (LED, for visual confirmation)

    
    EICRA |= (1 << ISC01) | (1 << ISC00);   // ISC01:ISC00 = 11 -> rising edge (adjust based on your button wiring)
    EIMSK |= (1 << INT0);    // enable INT0

    sei();

    while (1) {
        PORTB |= (1 << PORTB5);    // LED on = awake
        _delay_ms(1000);  // delay so LED-on is visible

        PORTB &= ~(1 << PORTB5);     // LED off = about to sleep
        enter_sleep();               // board powers down here until button press wakes it

        // execution resumes here after waking
        //printf("execution resumes...\n");
    }
}
```

#### 2. using `<avr/sleep.h>` helper functions

```c
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

ISR(INT0_vect) {
    // this ISR runs on wake - can be empty, its only job here is to end the sleep instruction
}

void enter_sleep(void) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // deepest sleep mode
    sleep_enable();                          // arm sleep (sets SE bit)
    sei();
    sleep_cpu();                              // actually sleep (executes the SLEEP instruction)
    sleep_disable();                          // runs immediately after waking - disarm SE
}

int main(void) {
    DDRD &= ~(1 << DDD2);   // PD2 = input (button)

    DDRB |= (1 << DDB5);    // PB5 = output (LED, for visual confirmation)

    EICRA |= (1 << ISC01) | (1 << ISC01);   // ISC01:ISC00 = 11 -> rising edge (adjust based on your button wiring)
    EIMSK |= (1 << INT0);    // enable INT0

    sei();                   // interrupts must be enabled before sleeping

    while (1) {
        PORTB |= (1 << PORTB5);    // LED on = awake
        _delay_ms(1000);  // delay so LED-on is visible 

        PORTB &= ~(1 << PORTB5);   // LED off = about to sleep
        enter_sleep();               // board powers down here until button press wakes it

        // execution resumes here after waking
    }
}
```


**Example: Power-down + Watchdog wake-up (periodic wake, no button needed)**

```c
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

ISR(WDT_vect) {
    // watchdog interrupt (not reset) - just wakes us up, nothing else needed here
}

void wdt_sleep_init(void) {
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);  // ~2s, interrupt-only mode (WDE=0)
}

void enter_sleep(void) {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
}

int main(void) {
    DDRB |= (1 << DDB5);   // LED for visual confirmation

    wdt_sleep_init();
    sei();

    while (1) {
        PORTB ^= (1 << PORTB5);   // toggle LED briefly to show we woke up
        _delay_ms(1000);

        enter_sleep();             // sleep ~2s until watchdog interrupt wakes us
    }
}
```
