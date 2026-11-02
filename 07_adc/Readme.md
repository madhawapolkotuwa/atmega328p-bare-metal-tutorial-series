# ADC

## ADC basics on ATmega328P

* 10-bit resolution → results range 0 - 1023.
* 6 analog input channels on the Uno: A0 - A5 (ADC0 - ADC5).
* Reference voltage options: AVCC (5V, most common), internal 1.1V, or external AREF.
* Conversion takes multiple ADC clock cycles, and the ADC clock itself needs to be prescaled down from 16 MHz into its recommended 50 kHz-200 kHz range for full accuracy.


### Example: Basic polling ADC read (A0)
Wire a potentiometer's wiper (middle pin) to A0, outer pins to 5V and GND.

#### Step 1: Configure the Reference Voltage and Input Channel (`ADMUX`)
The ADC Multiplexer Selection Register (`ADMUX`) configures which voltage reference the ADC uses to measure signals, how it formats the output data, and which analog pin is selected.
* Select the Voltage Reference (`REFS1:REFS0` - Bits 7:6):
    To measure analog signals, the **ADC** needs a reference scale. Setting `REFS1` to `0` and `REFS0` to `1` selects $AV_{CC}$ with an external capacitor at the `AREF` pin. This sets the maximum measurable voltage limit directly to the microcontroller's supply voltage ($5\text{V}$ on standard boards).
* Set the Data Alignment (`ADLAR` - Bit 5):
    The **ADC** converts analog voltages into a **10-bit** digital value (**0–1023**). Because this 10-bit result must be stored across two 8-bit registers (ADCH and ADCL), we must choose how to align the bits:
    * Keeping `ADLAR` at `0` configuration forces right-adjustment. The **lowest 8 bits** are stored in `ADCL`, and the **remaining 2 highest bits** are placed in `ADCH`. This is the standard setting when we want to read the full `10-bit resolution`.

* Select the Analog Pin Channel (`MUX3:MUX0` - Bits 3:0):
    These bits select which analog input pin is connected to the ADC. For example, setting these bits to `0000` routes physical channel ADC0 (A0) to the converter. You can dynamically modify these four bits during program execution to switch between different physical pins (such as A0 to A5) before initiating a conversion.

#### Step 2: Enable the ADC and Set the Clock Speed (`ADCSRA`)
The **ADC** Control and Status Register A (`ADCSRA`) is used to power up the ADC hardware, initiate conversions, and throttle the conversion clock speed.

* Turn on the Hardware (`ADEN` - Bit 7):
    Setting the **ADC** Enable (`ADEN`) bit to `1` powers on the internal analog-to-digital converter circuitry. The ADC remains dormant and consumes no power until this bit is set.

* Establish the ADC Clock Speed (`ADPS2:ADPS0` - Bits 2:0):
    The **ADC** hardware requires an input clock frequency between $50\text{ kHz}$ and $200\text{ kHz}$ to operate at its maximum 10-bit resolution.     
    Since microcontrollers often run at a much faster system clock (like $16\text{ MHz}$), you must scale down the frequency using the prescaler bits:
    * Setting `ADPS2`, `ADPS1`, and `ADPS0` all to `1` selects a division factor of `128`.
    * This divides a $16\text{ MHz}$ system clock down to $125\text{ kHz}$ ($\frac{16,000,000}{128}$), landing perfectly within the target speed range.

#### Step 3: Trigger a Conversion and Wait for Completion
Once configured, the conversion process is controlled dynamically using the status flags in `ADCSRA`.

* Start the Measurement (ADSC - Bit 6):
    Writing a 1 to the ADC Start Conversion (`ADSC`) bit tells the hardware to sample the selected analog channel and begin processing.
* Poll the Status (`ADSC` - Bit 6):
    The hardware keeps the `ADSC` bit at `1` for the entire duration of the sampling and successive approximation sequence (which takes 13 ADC clock cycles). The absolute moment the digital calculation is complete, the hardware automatically pulls `ADSC` back to `0`.  
    By setting up a software loop that waits while this bit is `1`, you ensure the CPU does not attempt to read the result before the conversion is fully finished.

#### Step 4: Retrieve the Digital Value (ADC)
Once `ADSC` returns to `0`, the calculated digital value is locked into the ADC Data Registers (`ADCH` and `ADCL`).

* Because the data is right-adjusted, reading the combined 16-bit virtual register address **ADC** (which internally reads the low byte `ADCL` first, then locks and reads the high byte `ADCH`) returns a stable integer value between 0 (representing $0\text{V}$) and 1023 (representing your reference voltage, $5\text{V}$).

Full Code:
```c
#include <avr/io.h>

void adc_init(void) {
    ADMUX  = (1 << REFS0);                 // reference = AVCC (5V), channel = ADC0 (A0)
    ADCSRA = (1 << ADEN)                    // enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler = 128 -> 125kHz ADC clock
}

uint16_t adc_read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);  // select channel, keep reference bits
    ADCSRA |= (1 << ADSC);                      // start conversion
    while (ADCSRA & (1 << ADSC));               // wait for conversion to finish
    return ADC;                                 // 10-bit result (0-1023)
}

int main(void) {
    adc_init();

    while (1) {
        uint16_t value = adc_read(0);   // read A0
        // value is now available to use, e.g. 0-1023
    }
}
```





