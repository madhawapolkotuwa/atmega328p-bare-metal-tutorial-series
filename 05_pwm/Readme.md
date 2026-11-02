# PWM
The `ATmega328P` features 6 PWM channels, which are physically driven by its three hardware timers. Each timer controls exactly two PWM outputs.

Here is the breakdown of the PWM configuration, mapping, and modes:  

1. PWM Channels & Physical Pin Mapping
The 6 PWM channels are mapped to the physical Output Compare (`OCnx`) pins of the chip:

Timer Module | Resolution | PWM Output Pin (AVR Name)| Arduino Physical Pin
-------------|------------|--------------------------|----------------------|
|Timer0|8-bit   |`OC0A` (PD6)|Pin 6   | 
|      |8-bit   |`OC0B` (PD5)|Pin 5   |     
|Timer1|16-bit  |`OC1A` (PB1)|Pin 9   |    
|      |16-bit  |`OC1B` (PB2)|Pin 10  | 
|Timer2|8-bit   |`OC2A` (PB3)|Pin 11  | 
|      |8-bit   |`OC2B` (PD3)|Pin 3   |

2. Supported PWM Modes
The `ATmega328P` timers can generate PWM signals using three primary modes of operation:

    * Fast PWM
        - **How it works**: The timer counts up from BOTTOM (0x00) to TOP (typically 0xFF for 8-bit timers, or custom register limits) and then immediately resets back to BOTTOM.
        - **Signal behavior**: In non-inverting mode, the pin clears on compare match and sets at BOTTOM.
        - **Pros**: Ideal for high-frequency applications (such as power regulation, DAC simulation, and switching power supplies) because it yields a frequency twice as fast as Phase Correct PWM.

    * Phase Correct PWM
        - **How it works**: The timer counts up from BOTTOM to TOP and then counts back down to BOTTOM.
        - **Signal behavior**: In non-inverting mode, the output pin clears on compare match when up-counting, and sets on compare match when down-counting.
        - **Pros**: Highly preferred for motor control. Because the pulses are centered, changing the duty cycle does not cause asymmetrical glitches or phase shifts.

    * Phase and Frequency Correct PWM (Timer1 Only)
        - **How it works**: Similar to Phase Correct PWM, but the Input Capture Register (`ICR1`) or Output Compare Register (`OCR1A`) acts as TOP, and the value is updated only at BOTTOM to avoid phase distortion when changing frequency on-the-fly.
        - **Pros**: Crucial for applications demanding precise, dynamic frequency sweeping (like complex motor control or audio signal synthesis).

3. Basic Register Configuration for PWM
    To output a PWM signal on any of these pins, you must configure two settings:

    1. **Set the Physical Port Pin to Output**: The `DDR` (Data Direction Register) bit for the corresponding port pin must be set to `1` (e.g., `DDRD |= (1 << DDD6`); to output PWM on `OC0A`).
    2. **Configure Compare Match Output Mode (`COMnx` bits)**: You must configure the Timer Control Registers (e.g., `TCCR0A`) to connect the timer output to the physical pin. Setting `COMnx1` = 1 and `COMnx0` = 0 configures the standard, non-inverting PWM output.

<br>
<br>

-----------------------

Now that we've got timer basics, PWM builds directly on top of the timer/counter registers.  
We just change the waveform generation mode(`WGM`) so the pin automatically toggles based on a duty-cycle value, instead of us toggling it manually in an ISR.


**Step-by-Step Register Settings**   

To generate a stable Fast PWM signal, we will use Fast PWM Mode 3 (which counts from 0x00 to 0xFF).  

1. Configure Timer/Counter0 Control Register A (`TCCR0A`)    
     To set up Fast PWM and connect the timer to the output pin:     
     * **Compare Output Mode (`COM0A1:0`)**: Set to `10` (Non-inverting mode). The physical output pin `OC0A` (Port D, Pin 6 / Arduino Pin 6) will clear on a compare match and set at BOTTOM.
     * **Waveform Generation Mode (`WGM01:0`)**: Set to `11` (Fast PWM Mode 3, where TOP is 0xFF).
     * **Action**: Write `0x83` (binary `0b10000011`) to `TCCR0A`.
    ```c
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    ```
2. Configure Timer/Counter0 Control Register B (`TCCR0B`)
    To set the clock source and prescaler:   
    * **Waveform Generation Mode (`WGM02`)**: Set to `0` (required for Mode 3).
    * **Clock Select (`CS02:0`)**: Choose a prescaler. For example, a prescaler of `64` (set `CS01` and `CS00` to `1`) works well for general-purpose PWM.
    * **Action**: Write `0x03` (binary `0b00000011`) to `TCCR0B`.
    ```c
    TCCR0B = (1 << CS01) | (1 << CS00);
    ```

3. Set the Duty Cycle in `OCR0A`
    For a 50% duty cycle in an 8-bit register (range 0–255):
    * $\text{Target Value} = 255 \times 0.5 \approx 128$ (or 0x80 in hex).
    * Action: Write the value directly to the Output Compare Register A.
    ```c
    OCR0A = 128;
    ```

4. Configure Port Pin as Output
    The physical pin connected to `OC0A` is `PD6`. For the hardware to override the pin and output the PWM wave, the data direction register must configure it as an output.
    ```c
    DDRD |= (1 << DDD6);
    ```

## Example 1 - Timer0 Fast PWM: LED dimming


```c
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    DDRD |= (1 << DDD6);   // D6 (OC0A) = output

    // Fast PWM, non-inverting mod#include <util/delay.h>e on OC0A
    TCCR0A |= (1 << WGM01) | (1 << WGM00);   // Fast PWM, mode 3 (TOP = 0xFF)
    TCCR0A |= (1 << COM0A1);                 // non-inverting: pin HIGH at start, LOW at match
    TCCR0B |= (1 << CS01);                   // prescaler = 8

    OCR0A = 128;   // ~50% duty cycle (0-255 range) -> half brightness

    while (1) {
        // duty cycle is now handled entirely by hardware
        // change OCR0A anytime to change brightness, e.g.:
        // OCR0A = 255; // full brightness
        // OCR0A = 0;   // off

        OCR0A = 0;   // 0%
        _delay_ms(100);

        OCR0A = 64;  // ~25%
        _delay_ms(100);

        OCR0A = 128; // ~50%
        _delay_ms(100);

        OCR0A = 192; // ~75%
        _delay_ms(100);

        OCR0A = 255; // ~100%
        _delay_ms(100);
    }
}
```

## Example 2 - Timer2 Fast PWM: LED dimming on D11

Same idea, different timer - useful when `Timer0` is busy doing something else (like `millis()` if you were mixing with Arduino core functions).

```c
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << DDB3);   // D11 (OC2A) = output

    TCCR2A |= (1 << WGM21) | (1 << WGM20);    // Fast PWM, TOP = 0xFF
    TCCR2A |= (1 << COM2A1);                  // non-inverting
    TCCR2B |= (1 << CS22);                    // prescaler = 64

    OCR2A = 64;   // ~25% duty cycle -> dim

    while (1) {
        // adjust OCR2A to change brightness
    }
}
```

## Example 3 - Timer1 Fast PWM: Servo control on D9
This is where Timer1's 16-bit resolution matters. Servos expect a pulse repeated every ~20ms, where pulse width (typically 1ms–2ms) sets the angle. We use `ICR1` as TOP so we can set an exact 20ms period.

```c
#include <avr/io.h>

int main(void) {
    DDRB |= (1 << DDB1);   // D9 (OC1A) = output

    // Fast PWM, mode 14: TOP = ICR1
    TCCR1A |= (1 << WGM11);
    TCCR1B |= (1 << WGM13) | (1 << WGM12);
    TCCR1A |= (1 << COM1A1);                // non-inverting on OC1A (D9)
    TCCR1B |= (1 << CS11);                  // prescaler = 8

    ICR1 = 39999;   // TOP value -> 20ms period (see math below)

    OCR1A = 3000;   // ~1.5ms pulse -> servo centered (~90 degrees)

    while (1) {
        // change OCR1A to set servo angle, e.g.:
        // OCR1A = 2000;  // ~1ms pulse -> one extreme (~0 degrees)
        // OCR1A = 4000;  // ~2ms pulse -> other extreme (~180 degrees)
    }
}
```

## Example 4 -  RGB LED with PWM

Three PWM channels, one per color, is exactly what an RGB LED needs. 

>> common cathode RGB LED (common pin → GND)

**Pin assignment**   
To avoid conflicts, use one output-compare channel from each of the three timers:
Color | Pin |Timer/Channel |
------|------|-------------|
|Red |D9 |Timer1 OC1A|
|Green|D11|Timer2 OC2A|
|Blue|D6|Timer0 OC0A |

Each color pin needs its own current-limiting resistor (`~220Ω`) between the Uno pin and the LED leg; the common leg goes to GND.

```c
#include <avr/io.h>

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

    set_color(255, 0, 0);   // pure red

    while (1) {
        // static color; try changing set_color() args above
    }
}
```



