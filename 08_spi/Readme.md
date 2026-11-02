# SPI - Serial Peripheral Interface

The serial peripheral interface (SPI) allows high-speed synchronous data transfer between the ATmega328P and peripheral devices or between several AVR devices. 

## Master configuration

### Step 1: Configure the Data Direction Register (`DDRB`)

Before enabling the SPI hardware, the physical pins on Port B must be configured correctly for Master mode:
* **MOSI** (Master Out Slave In - `PB3`): Must be set to an output (`DDB3 = 1`) so the master can transmit data.
* **SS** (Slave Select - PB2): Must be configured as an output (`DDB2 = 1`) to act as the hardware chip select line. Keeping this pin as an output or pulled high is critical,   
if the $\overline{\text{SS}}$ pin is configured as an input and pulled low by external hardware, the SPI automatically drops out of Master mode into Slave mode.

### Step 2: Configure the SPI Control Register (`SPCR`)
The SPI Control Register (`SPCR`) configures the core operational settings of the module.
* Enable the SPI Module (`SPE` - Bit 6):   
    Setting the SPI Enable (`SPE`) bit to `1` powers on the SPI hardware and overrides the normal I/O pin functions for the configured SPI pin
* Select Master Mode (`MSTR` - Bit 4):     
    Setting the Master/Slave Select (`MSTR`) bit to `1` configures the *ATmega328P* as the Master device, driving the SCK clock line.
* Set Data Order (DORD - Bit 5):
    * `0` (default): Transmits the Most Significant Bit (**MSB**) first.
    * `1`: Transmits the Least Significant Bit (**LSB**) first.

* Set Clock Polarity and Phase (`CPOL` & `CPHA` - Bits 3:2):
    These bits define the SPI Mode (Modes 0, 1, 2, or 3) to match the timing diagram required by your slave peripheral:
    * `CPOL = 0`: Clock idles low. `CPOL = 1`: Clock idles high.
    * `CPHA = 0`: Data is sampled on the leading (first) clock edge. `CPHA = 1`: Data is sampled on the trailing (second) clock edge.

### Step 3: Configure the SPI Clock Speed (Prescalers)
The SPI clock frequency ($f_{sck}$) is derived from the main system clock ($f_{osc}$, typically $16\text{ MHz}$) and is throttled using three configuration bits: `SPR1` and `SPR0` (Bits `1:0` in `SPCR`), combined with the SPI2X double-speed bit (Bit `0` in the SPI Status Register `SPSR`).

To set up a specific clock speed, use the following register combinations:
Desired Speed (at 16 MHz)|Division Factor (**fosc​/N**)|`SPI2X` (in `SPSR`)|`SPR1` (in `SPCR`)|`SPR0` (in `SPCR`)    
--------|--------|---------|---------|----------|
8 MHz|fosc​/2|1|0|0
4 MHz (Default)|fosc​/4|0|0|0
2 MHz|fosc​/8|1|0|1
1 MHz|fosc​/16|0|0|1
500 kHz|fosc​/32|1|1|0
250 kHz|fosc​/64|0|1|0
125 kHz|fosc​/128|0|1|1

### Step 4: Manage Transmissions via Status and Data Registers
* SPI Data Register (`SPDR`):
    Writing a byte directly to `SPDR` instigates the hardware transmission clock. Because SPI is a full-duplex shifting architecture, writing a byte shifts out data on the MOSI line while simultaneously shifting in a byte from the MISO line into this exact same register.

* SPI Status Register (`SPSR`) - Polling `SPIF` (Bit 7):
    When a full 8-bit block completes its serial shift, the hardware sets the SPI Interrupt Flag (`SPIF`) bit to 1. By setting up a loop that polls SPSR and waits until SPIF becomes active, the CPU knows the data transfer has finished and that the new incoming byte is safely sitting in `SPDR` ready to be read.

Code:

```c
#include <avr/io.h>
#include <util/delay.h>

#define SS_PIN PB2   // D10

void spi_master_init(void) {
    DDRB |= (1 << DDB3) | (1 << DDB5) | (1 << DDB2);  // MOSI (D11), SCK (D13), SS (D10) = outputs
    // MISO (D12) stays input automatically (not set in DDRB)

    PORTB |= (1 << SS_PIN);              // idle CS high (deselected)

    SPCR = (1 << SPE) | (1 << MSTR);   // enable SPI, master mode
    // clock rate left at default (fosc/4); 
}

uint8_t spi_transfer(uint8_t data) {
    SPDR = data;                        // load byte into data register, starts transmission
    while (!(SPSR & (1 << SPIF)));      // wait until transmission complete
    return SPDR;                        // returns byte received back (full-duplex)
}

uint8_t spi_transfer_with_cs(uint8_t data) {
    PORTB &= ~(1 << SS_PIN);   // CS low - select slave
    uint8_t result = spi_transfer(data);
    PORTB |= (1 << SS_PIN);    // CS high - deselect slave
    return result;
}

int main(void) {
    spi_master_init();

    while (1) {
        uint8_t ret = spi_transfer_with_cs(0x12);;   // send a test byte
        _delay_ms(1000);
    }
}
```

## Slave configuration

### Step 1: Invert the Data Direction Register (`DDRB`) Settings
In Slave mode, the external Master controls the clock (SCK) and the chip select ($\overline{\text{SS}}$) lines, while the data directions for MOSI and MISO reverse.

* **MISO** (Master In Slave Out - PB4): Must now be explicitly set to an output (`DDB4 = 1`) so the slave can shift its data back out to the Master.

* **MOSI** (Master Out Slave In - PB3): Must be configured as an input (`DDB3 = 0`) to receive data from the Master.

* **SCK** (Serial Clock - PB5): Must be configured as an input (`DDB5 = 0`) since the clock pulse is generated externally by the Master.

* **SS** (Slave Select - PB2): Must be configured as an input (`DDB2 = 0`). The hardware uses a low signal on this pin to sleep/wake the slave SPI logic.

### Step 2: Modify the SPI Control Register (`SPCR`)

* **Clear Master Mode** (`MSTR` - Bit 4):
    This bit must be set to `0`. Clearing `MSTR` configures the SPI peripheral into Slave mode, letting the internal shift register sit passively until external clock cycles arrive on the SCK pin.

* **SPI Enable** (`SPE` - Bit 6):
    This remains set to `1` to keep the SPI subsystem powered on.

* **Clock Rate Bits** (`SPR1`, `SPR0`, and `SPI2X`):
These prescaler bits become entirely ignored by the hardware in Slave mode. The slave automatically synchronizes itself to whatever clock frequency the Master provides (up to a maximum threshold of $f_{osc}/2$).

### Step 3: Streamline Data Transfers (`SPDR` & `SPSR`)
The process for full-duplex transmission remains fundamentally the same, with one primary architectural difference in how it is triggered:

* Data Register (`SPDR`):
    Writing a byte to `SPDR` does not initiate a transmission in Slave mode. Instead, writing a byte simply loads the slave's internal buffer. The actual data shift only begins when the external Master starts pulsing the `SCK` line.

* Status Register Flag (`SPIF` - Bit 7):
    Just like in Master mode, you still poll the `SPIF` bit in SPSR to wait for a transmission to finish. When `SPIF` reads `1`, it indicates that a full 8-bit block has successfully completed shifting in/out, meaning the new data byte from the master is waiting to be read inside `SPDR`.

```c
#include <avr/io.h>

void spi_slave_init(void) {
    DDRB &= ~((1 << DDB3) | (1 << DDB5) | (1 << DDB2));  // MOSI, SCK, SS = inputs
    DDRB |= (1 << DDB4);                                 // MISO = output

    SPCR = (1 << SPE);   // enable SPI, slave mode (no MSTR)

    SPDR = 0x34;         // preload first response byte (matches your ESP32 tx_buffer example)
}

int main(void) {
    spi_slave_init();

    while (1) {
        while (!(SPSR & (1 << SPIF)));    // wait for master to complete a transfer
        uint8_t received = SPDR;          // byte the master sent

        // ... do something with 'received' ...

        SPDR = 0x34;          // preload next response byte for the following transfer
    }
}
```