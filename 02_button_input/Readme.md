# AVR LED Blink + Button Control 

## Video :-

[![Youtube Video](https://img.youtube.com/vi/lw3IV1jJNLU/0.jpg)](https://www.youtube.com/watch?v=lw3IV1jJNLU)


## Polling
This project demonstrates basic GPIO usage on an AVR microcontroller (ATmega-series, PORTB): first blinking an LED, then extending it to control the LED with a pushbutton.

**Hardware:**

Signal | Pin | Direction | Notes | 
-------|-----|-----------|-------|
LED | PORTB5 (PB5) |Output | Drives an LED (with current-limiting resistor) 
Button | PORTB4 (PB4) | Input | Reads button state

**Button wiring (this setup):**

* Button pressed → PB4 reads HIGH
* Button released → PB4 reads LOW (held low via an external pull-down resistor, e.g. 10kΩ to GND)

**Part 1:  LED Blink**

The starting point for this project was a simple blink sketch: PB5 configured as an output, toggled on and off in a loop (typically with a delay) to blink the LED continuously.

**Part 2: Button-Controlled LED**    

This part replaces the timed blink with button-driven control:

* PB4 is configured as an input (no internal pull-up, since the circuit uses an external pull-down).
* PB5 remains an output driving the LED.
* The LED simply follows the button state: on while pressed, off while released.

```c
#include <avr/io.h>

int main(void) {
    DDRB &= ~(1 << DDB4);   // PB4 = input (button)
    // no pull-up enabled - external pull-down resistor holds it LOW when idle
    // if not enable internal pull-up on PB4  -> PORTB |= (1 << PORTB4); 

    DDRB |= (1 << DDB5);    // PB5 = output (LED)

    while (1) {
        if (PINB & (1 << PINB4)) {
            // button pressed -> PB4 reads HIGH
            PORTB |= (1 << PORTB5);   // LED on
        } else {
            PORTB &= ~(1 << PORTB5);  // LED off
        }
    }
}
```

How it works


1. `DDRB` (Data Direction Register B) sets pin direction: clearing bit 4 makes PB4 an input; setting bit 5 makes PB5 an output.
2. `PINB` is read to check the current logic level on PB4.
3. `PORTB` is written to drive PB5 high (LED on) or low (LED off), based on the button reading.
The while(1) loop polls the button continuously, so the LED updates in real time as the button is pressed and released.

Important notes


* Make sure an external pull-down resistor (~10kΩ) is wired from PB4 to GND. Without it, the pin will float when the button isn't pressed and the LED behavior will be unpredictable.
* This version does not include debouncing. A mechanical button press can cause brief, rapid HIGH/LOW glitches ("bounce"). For this simple on/off behavior it's usually not noticeable, but if you later want to count presses or trigger single events, debouncing (in software or hardware) will be needed.

>> above polling method: The CPU continuously checks the pin in a loop. Works fine, but wastes CPU cycles busy-waiting.

## External Interrupts

### Step 1: PCINT Version (button stays on PB4)

PB4 doesn't support `INT0`/`INT1` (those are fixed to `PD2`/`PD3`), but it does support pin-change interrupts via `PCINT4`, part of the `PCINT0` vector group covering all of `PORTB` (PB0–PB7).

1. Set pin direction
    ```c
    DDRB &= ~(1<<DDB4);     // PB4 = input (button)
    DDRB |= (1 << DDB5);    // PB5 = output (LED)
    ```
2. Enable the pin-change interrupt group for `PORTB`
    ```c
    PCICR |= (1 << PCIE0);   // enable pin-change interrupts for PCINT[7:0] (PORTB)
    ```
3. Enable the mask bit for `PB4` specifically
    ```c
    PCMSK0 |= (1 << PCINT4);
    ```
    `PCICR` turns the interrupt group on; `PCMSK0` selects which individual pin(s) inside that group actually trigger it.

4. Write the ISR
    `PCINT` fires on any change (press or release) - it can't tell you the direction, so the ISR re-reads the pin and sets the LED to match:
    ```c
    ISR(PCINT0_vect) {
        if (PINB & (1 << PINB4))
            PORTB |= (1 << PORTB5);    // button pressed -> LED on
        else
            PORTB &= ~(1 << PORTB5);   // button released -> LED off
    }
    ```
Full PCINT example:
```c
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(PCINT0_vect) {
    if (PINB & (1 << PINB4))
        PORTB |= (1 << PORTB5);    // button pressed -> LED on
    else
        PORTB &= ~(1 << PORTB5);   // button released -> LED off
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
```

--------------------

### Step 2: PCINT Rising-Edge-Only Version (software-checked)

`PCINT` has no hardware edge-direction selection,So it trigger every Rising and Falling edge.    

what if we want only: "rising edge ISR" (i.e., react only when the button is pressed, not released) has to be done in software by tracking the previous pin state.

1. Add a state-tracking variable
    ```c
    volatile uint8_t last_state = 0;
    ```
Must be `volatile` since it's shared between the ISR and (potentially) `main`.

2. Update the ISR to detect the rising edge
    ```c
    ISR(PCINT0_vect) {
        uint8_t current_state = PINB & (1 << PINB4);

        if (current_state && !last_state) {
            // was LOW, now HIGH -> rising edge (button just pressed)
            PORTB ^= (1 << PORTB5);   // toggle LED
        }

        last_state = current_state;
    }
    ```

### Step 3: INT0 Version (button moved to PD2)

`INT0` is a dedicated external interrupt pin with true hardware edge/level selection via `EICRA` - no software state tracking needed. This requires moving the button from PB4 to **PD2**.

1. Set pin directions
    ```c
    DDRD &= ~(1 << DDD2);   // PD2 = input (button, moved from PB4)
    DDRB |= (1 << DDB5);    // PB5 = output (LED)
    ```

2. Choose the trigger condition in `EICRA`

    The `ISC01:ISC00` bit pair controls what triggers `INT0`:

    ISC01 | ISC00 | Trigger |
    ------|-------|---------|
    0 | 0 | Low level 
    0 | 1 | Any logical change
    1 | 0 | Falling edge
    1 | 1 | Rising edge

For "any change" (mirrors PCINT behavior, files on press and release)
    ```c
    EICRA |= (1 << ISC00);   // ISC01:ISC00 = 01 -> any logical change
    ```
3. Enable `INT0`
    ```c
    EIMSK |= (1 << INT0);
    ```

4. Enable global interrupts
    ```c
    sei();
    ```

5. Write the ISR (level-following)
    ```c
    #include <avr/io.h>
    #include <avr/interrupt.h>

    ISR(INT0_vect) {
        if (PIND & (1 << PIND2))
            PORTB |= (1 << PORTB5);    // button pressed -> LED on
        else
            PORTB &= ~(1 << PORTB5);   // button released -> LED off
    }

    int main(void) {
        DDRD &= ~(1 << DDD2);   // PD2 = input (button)
        DDRB |= (1 << DDB5);    // PB5 = output (LED)

        EICRA |= (1 << ISC00);  // ISC01:ISC00 = 01 -> any logical change
        EIMSK |= (1 << INT0);   // enable INT0

        sei();                  // enable global interrupts

        while (1) {
            // CPU free
        }
    }
    ```

### Step 4: INT0 Rising-Edge-Only Version (true hardware edge trigger)
This is the cleanest version: set `ISC01:ISC00 = 11` and the hardware only interrupts on the rising edge (button press) - no software state tracking required.

1. Set the rising-edge trigger
    ```c
    EICRA |= (1 << ISC01) | (1 << ISC00);  // ISC01:ISC00 = 11 -> rising edge only
    ```

2. Write the ISR

    Since this only fires once per press (no corresponding release interrupt), toggling the LED makes more sense than setting it:

    ```c
    #include <avr/io.h>
    #include <avr/interrupt.h>

    ISR(INT0_vect) {
        PORTB ^= (1 << PORTB5);   // toggle LED — this ISR only fires on button press (rising edge)
    }

    int main(void) {
        DDRD &= ~(1 << DDD2);   // PD2 = input (button)
        DDRB |= (1 << DDB5);    // PB5 = output (LED)

        EICRA |= (1 << ISC01) | (1 << ISC00);  // rising edge only
        EIMSK |= (1 << INT0);                   // enable INT0

        sei();                                  // enable global interrupts

        while (1) {
            // CPU free
        }
    }
    ```



