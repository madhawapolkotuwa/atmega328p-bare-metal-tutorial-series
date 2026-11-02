# Timer/Counter

**ATmega328P** has time/counter peripheral options **two 8-bit timers**, **one 16-bit timer**, six PWM channels, and a 10-bit ADC. Let's start with the basics of each before bringing PWM into it.

* Two 8-bit Timer/Counters (Timer/Counter0 and Timer/Counter2)
* One 16-bit Timer/Counter (Timer/Counter1)

Timer | Size | Notable pins (OC = output compare) | Notes
------|------|------------------------------------|-------
Timer/Counter0 | 8-bit | OC0A = PD6, OC0B = PD5 | Also used by `millis()`/`delay()` in Arduino core
Timer/Counter1 | 16-bit | OC1A = PB1, OC1B = PB2 | Best for servos (needs finer resolution)
Timer/Counter2 | 8-bit | OC2A = PB3, OC2B = PD3 | Independent clock source option (async mode)

### How a timer works
A timer is a register (`TCNTn`) that automatically increments on every **clock tick** (or every N ticks, via a prescaler). we don't touch it manually.

1. Pick a mode (Normal, CTC, PWM, etc.) via the `TCCRnA`/`TCCRnB` control registers.
2. Pick a prescaler (how much to divide the 16 MHz clock before it reaches the timer).
3. Optionally set a compare value (`OCRnA`/`OCRnB`) - when `TCNTn` matches it, something happens (interrupt fires, pin toggles, etc.)

----------------------------------------------------

## Example: Timer/Couner0(8-bit) in CTC mode (`10ms timer`)


* To configure the `8-bit Timer/Counter0` for a `10 ms interval`,      
The register description steps need to change because an `8-bit` timer can only count up to `255`.

* At a standard **16 MHz** clock speed, the maximum delay Timer0 can achieve with its largest prescaler (`1024`) is about $16.38\text{ ms}$, so a 10 ms delay is entirely possible.

* Here are the register configuration steps based on Section [14.9 of the datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) (conceptually identical for Timer0 in its respective register section, typically Chapter 14):

### Step-by-Step Register Settings (at 16 MHz)

1. Calculate the Target Compare Value (`OCR0A`)

    To achieve a 10 ms ($0.01\text{ s}$) delay, we must use a clock prescaler(max) of `1024`.

* The timer clock frequency becomes: $\frac{16,000,000\text{ Hz}}{1024} = 15,625\text{ Hz}$ (ticks per second).
* Ticks needed for 10 ms: $15,625\text{ Hz} \times 0.01\text{ s} = 156.25\text{ ticks}$.
* Rounding to the nearest whole tick gives 156.
* Because the timer counts from 0, the value to write into the Output Compare Register is: $156 - 1 = \mathbf{155}$ (or `0x9B` in hex).
>> (Note: Because 156.25 isn't a perfect integer, the actual interval will be $9.984\text{ ms}$, giving a minor calculation error of about 0.16%).

2. Configure Timer/Counter0 Control Register A (`TCCR0A`)    
* Set Mode 2 (**CTC** Mode with TOP in `OCR0A`).     
For this mode, the waveform generation bits must be configured so $WGM01 = 1$ and $WGM00 = 0$.
* Action: Write `0x02` to `TCCR0A`.
    ```c
    TCCR0A = (1 << WGM01);
    ```

3. Configure Timer/Counter0 Control Register B (`TCCR0B`)
* Set the remaining CTC mode bit: $WGM02$ must be 0.
* Set the Prescaler clock source to `1024`. This requires setting bits $CS02$ and $CS00$ to 1 ($CS01$ is 0).
* Action: Write `0x05` (binary `0b00000101`) to `TCCR0B`.
    ```c
    TCCR0B = (1 << CS02) | (1 << CS00);
    ```

4. Load the Compare Value into `OCR0A`
* Action: Write the calculated value directly to the 8-bit register.
    ```c
    OCR0A = 155;
    ```

5. Enable the Interrupt (Optional)
* To trigger an interrupt function every time the timer hits 10 ms, enable the Timer/Counter0 Output Compare Match A Interrupt bit ($OCIE0A$) inside the Timer Interrupt Mask Register (`TIMSK0`).

    ```c
    TIMSK0 |= (1 << OCIE0A);
    ```

### Full Code:
```c
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER0_COMPA_vect) {
    PORTB ^= (1 << PORTB5);   // toggle LED (D13) every time the timer matches
}

int main(void) {
    DDRB |= (1 << DDB5);      // D13 = output

    // Timer0 in CTC mode
    TCCR0A |= (1 << WGM01);                 // CTC mode (clear timer on compare match)
    TCCR0B |= (1 << CS02) | (1 << CS00);    // prescaler = 1024

    // (8-bit)
    OCR0A = 155;                            // compare value -> ~10 Hz toggle rate
    TIMSK0 |= (1 << OCIE0A);                // enable Timer0 compare-match A interrupt

    sei();

    while (1) {
        // CPU free
    }
}
```

#### Timer/Couner0(8-bit) in CTC mode (`1s timer`)
What if we want to generate large dealy timer: ex:- (`1s`)   

Since an 8-bit timer cannot count high enough to generate a 1-second delay directly,     
we can utilizing a **software counter** (`isr_count`) inside the Interrupt Service Routine (ISR).    

Based on the register settings configured in the previous step:  
* Each Compare Match A interrupt occurs every 10 ms (technically $9.984\text{ ms}$ due to the minor calculation rounding error at 16 MHz).
* Multiplying that by 100 interrupts gives: $9.984\text{ ms} \times 100 \approx \mathbf{0.9984\text{ s}}$ (very close to 1 second).

```c
volatile uint16_t isr_count = 0;

ISR(TIMER0_COMPA_vect) {
    isr_count++;
    if (isr_count >= 100) {       // 100 interrupts x 10ms = 1000ms = 1s
        PORTB ^= (1 << PORTB5);   // toggle LED (D13)
        isr_count = 0;
    }
}
```

-------------------------------

## Timer/Counter1(16-bit) CTC mode (`1s timer`)

Now `16-bit` timer:  
Math: 16 MHz / 1024 = 15,625 Hz tick rate. `OCR1A = 15624`   v
    (i.e., 15625 counts, since it counts 0→OCR1A inclusive) → exactly 1 second per compare match → LED toggles once per second, same visible result this time no software counter needed.

To achieve a 1s delay, clock prescaler(max) of `1024`.

* The timer clock frequency becomes: $\frac{16,000,000\text{ Hz}}{1024} = 15,625\text{ Hz}$ (ticks per second).
* Ticks needed for 1s: $15,625\text{ Hz} \times 1\text{ s} = 15,625\text{ ticks}$.
* Because the timer counts from 0, the value to write into the Output Compare Register is: $15,625 - 1 = \mathbf{15,624}$.

Full code:
```c
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
```







