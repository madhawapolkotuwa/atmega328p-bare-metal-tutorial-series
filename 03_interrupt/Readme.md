# External Interrupt

Here we walks step by step through converting the polling button/LED example into interrupt-driven versions: first `PCINT` (button stays on PB4), then `INT0` (button moved to PD2), including a rising-edge-only variant.

## Starting Point : Polling (for reference) [last example](../02_button_input/)
```c
#include <avr/io.h>

int main(void) {
    DDRB &= ~(1 << DDB4);   // PB4 = input (button)
    DDRB |= (1 << DDB5);    // PB5 = output (LED)

    while (1) {
        if (PINB & (1 << PINB4))
            PORTB |= (1 << PORTB5);   // LED on
        else
            PORTB &= ~(1 << PORTB5); // LED off
    }
}
```
>> The CPU continuously checks the pin in a loop. Works fine, but wastes CPU cycles busy-waiting.

------------------

## Step 1: PCINT Version (button stays on PB4)

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

## Step 2: PCINT Rising-Edge-Only Version (software-checked)

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

## Step 3: INT0 Version (button moved to PD2)

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

## Step 4: INT0 Rising-Edge-Only Version (true hardware edge trigger)
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


