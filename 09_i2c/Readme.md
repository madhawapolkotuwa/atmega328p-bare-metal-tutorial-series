# I2C (TWI) (2-wire Serial Interface)

unlike SPI, I2C only needs two wires (SDA/SCL) plus pull-ups.

The TWI (Two-Wire Serial Interface) or $\text{I}^2\text{C}$ hardware uses a state-machine driven register architecture.

Unlike other peripherals, the TWI hardware relies on a central status flag (`TWINT`) to execute each step sequentially. 

## TWI Master Mode Configuration

To act as a Master, the ATmega328P must generate the Clock ($\text{SCL}$) and actively command the bus.

#### Step 1: Calculate and Set the $\text{SCL}$ Bus Clock Speed (TWBR & TWSR)
The $\text{SCL}$ clock frequency is determined by the TWI Bit Rate Register (`TWBR`) and the TWI Prescaler bits (`TWPS1:0` in the TWI Status Register `TWSR`).

The $\text{SCL}$ frequency formula is:

$$\text{SCL Frequency} = \frac{f_{osc}}{16 + 2 \times \text{TWBR} \times \text{PrescalerValue}}$$

To achieve standard $\text{I}^2\text{C}$ speeds at a 16 MHz system clock:    
* For 100 kHz (Standard Mode):
    * Set the Prescaler to 1 by clearing the prescaler bits (`TWPS1:0 = 00`) in `TWSR`.
    * Set `TWBR` = 72 (since $\frac{16,000,000}{16 + 2(72)(1)} = 100,000\text{ Hz}$).

* For 400 kHz (Fast Mode):   
    * Set the Prescaler to 1 (`TWPS1:0 = 00`).
    * Set `TWBR` = 12 (since $\frac{16,000,000}{16 + 2(12)(1)} = 400,000\text{ Hz}$).

#### Step 2: Enable the TWI Module (`TWCR`)
Master communication requires driving the bus state by writing specific bit patterns to `TWCR`:

1. Generate a START Condition:
    * Write `(1 << TWINT) | (1 << TWSTA) | (1 << TWEN)` to `TWCR`.
    * Note on `TWINT`: Writing a 1 to the TWI Interrupt Flag (`TWINT` - Bit 7) is how you clear it. You must clear `TWINT` to instruct the hardware to start processing the command.

2. Wait for Hardware Action:
    * Poll the `TWINT` flag. It is pulled to 0 during active hardware execution and sets back to 1 by hardware once the START condition has successfully completed.

3. Transmit the Address / Data (`TWDR`):
    * Once `TWINT` is set, load your target 7-bit Slave Address + the Read/Write bit (LSB) into the TWI Data Register (`TWDR`).
    * Clear the `TWINT` flag again to send the byte by writing (`1 << TWINT`) | (`1 << TWEN`) to `TWCR`. Poll `TWINT` until it sets to `1` again, confirming the byte was shifted out and an ACK/NACK was received.

4. Generate a STOP Condition:
    * Write `(1 << TWINT) | (1 << TWSTO) | (1 << TWEN)` to `TWCR`. The `TWSTO` bit is automatically cleared by the hardware once the STOP condition is successfully written to the bus.

```c
#include <avr/io.h>

void twi_init(void) {
    TWSR = 0x00;                              // prescaler = 1
    TWBR = ((F_CPU / 100000UL) - 16) / 2;     // 100kHz SCL
    TWCR = (1 << TWEN);                        // enable TWI
}

void twi_start(void) {
    TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));   // wait for start condition to complete
}

void twi_stop(void) {
    TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);
}

void twi_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWEN) | (1 << TWINT);
    while (!(TWCR & (1 << TWINT)));   // wait for byte to be shifted out + ACK/NACK received
}

uint8_t twi_read_ack(void) {
    TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);   // read + send ACK (more bytes wanted)
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t twi_read_nack(void) {
    TWCR = (1 << TWEN) | (1 << TWINT);   // read + send NACK (last byte)
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}
```

------------------

## TWI Slave Mode Configuration
In Slave Mode, the microcontroller sits passively, responding only when its assigned address is called by an external Master.


#### Step 1: Assign the Slave Address (`TWAR`)
The TWI (Slave) Address Register (`TWAR`) holds the unique identifying address for this device.

* Set 7-Bit Address (`TWA6:TWA0` - Bits 7:1):
    Load your desired 7-bit device address into the upper 7 bits of `TWAR`.

* General Call Enable (`TWGCE` - Bit 0):
    Set to `1` if you want this device to respond to the global broadcast address (`0x00`), or `0` to ignore general calls.

#### Step 2: Configure and Enable the Slave Node (`TWCR`)
Instead of driving the bus, the slave must be configured to acknowledge matches.

* Enable Acknowledge (`TWEA` - Bit 6):
    Set to `1`. This instructs the hardware to automatically pull the $\text{SDA}$ line low (generating an ACK pulse) when:
    1. Its address matches the byte on the bus.
    2. A general call is received (if `TWGCE` is enabled).
    3. A data byte is received during communication.

* Enable TWI (`TWEN` - Bit 2):
    Set to `1` to turn on the interface.

* Action Command:
    Writing `(1 << TWINT) | (1 << TWEA) | (1 << TWEN)` tells the slave to wait passively on the bus.

#### Step 3: Detect Address Match and Read/Write Data

* Address Recognition:
    The CPU will idle until a Master addresses it. When a match occurs, the hardware pulls TWINT to `1`.
* Identify the Transfer Direction (`TWSR` - Bits 7:3):
    The application must read the status bits in the TWI Status Register (`TWSR`) to know if it was addressed as a receiver or a transmitter:
    * `0x60` status code: The device has been addressed as a Slave Receiver (Master wants to write to you).
    * `0xA8` status code: The device has been addressed as a Slave Transmitter (Master wants to read from you).
* Process the Data:
    * If receiving: Read the received byte from `TWDR`, then clear `TWINT` with `TWEA` set to wait for the next byte.
    * If transmitting: Write the byte you want to send into `TWDR`, then clear `TWINT` to let the hardware shift the byte out to the Master.

Code:

```c
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#define MY_SLAVE_ADDR 0x42

volatile uint8_t received_byte = 0;
volatile uint8_t data_ready = 0;
volatile uint8_t response_byte = 0x99;   // update this anytime to change what we report

ISR(TWI_vect) {
    uint8_t status = TWSR & 0xF8;

    switch (status) {
        case TW_SR_SLA_ACK:
            // master is about to write to us — just ACK and wait for data
            TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT) | (1 << TWIE);
            break;

        case TW_SR_DATA_ACK:
            // a data byte has arrived
            received_byte = TWDR;
            data_ready = 1;
            TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT) | (1 << TWIE);
            break;

        case TW_ST_SLA_ACK:
        case TW_ST_DATA_ACK:
            // master wants to read a byte from us
            TWDR = response_byte;
            TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT) | (1 << TWIE);
            break;

        case TW_ST_LAST_DATA:
        case TW_SR_STOP:
            // transaction finished — reset to listen for next one
            TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT) | (1 << TWIE);
            break;

        default:
            // unexpected status — reset TWI state machine
            TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWINT) | (1 << TWIE);
            break;
    }
}

void twi_slave_init(void) {
    TWAR = (MY_SLAVE_ADDR << 1);
    TWCR = (1 << TWEN) | (1 << TWEA) | (1 << TWIE);   // enable TWI + ACK + interrupt
    sei();
}

```