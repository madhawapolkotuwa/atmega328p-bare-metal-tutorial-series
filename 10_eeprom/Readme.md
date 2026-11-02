# EEPROM

The ATmega328P microcontroller contains 1,024 bytes(1kB) of built-in EEPROM (Electrically Erasable Programmable Read-Only Memory).    
This is a type of non-volatile memory, meaning it securely saves data (like system settings or calibration numbers) even when the power goes off.    
It guarantees `100,000` write/erase cycles per address.


Two ways to access it
1. Direct register access (`EEARL`/`EEARH`, `EEDR`, `EECR`) - the low-level way, good to understand.
2. `<avr/eeprom.h>` - AVR-libc's built-in helper functions that wrap the registers for you. Almost always what you'd actually use in practice.

## 1. Direct register access

#### The EEPROM Hardware Registers
* `EEAR` (`EEARL`/`EEARH`) (**EEPROM Address Register**): A combined 16-bit register used to specify the exact memory location (0 to 1023) you want to read from or write to.
* `EEDR` (**EEPROM Data Register**): The data buffer. For a write operation, you place the byte to be saved here. For a read operation, the hardware places the retrieved byte here.
* `EECR` (**EEPROM Control Register**): The command register used to initiate the read and write sequences.

#### Configuration for an EEPROM WRITE
Writing to EEPROM requires a strict hardware handshake sequence to prevent accidental data corruption.
1. Check for Pending Write Operations
    Before attempting a new write, you must wait until any previous write cycle finishes. The hardware sets the EEPROM Write Enable (`EEPE` - **Bit 1**) bit to `1` while a write is in progress and clears it to `0` when finished.
    * **Action**: Poll the `EEPE` bit in the `EECR` register and wait until it becomes `0`.
    * **Self-Programming Check** (Optional): You should also check the Store Program Memory Control and Status Register (`SPMCSR`) to ensure the flash memory isn't currently self-programming (`SELFPRGEN` bit is `0`).

2. Load the Address and Data
    * **Action**: Write the target memory address into `EEARH` and `EEARL`.
    * **Action**: Write the **8-bit** data byte you wish to save into `EEDR`.

3. Execute the Two-Step Write Command
    The hardware requires a safety lock bit to be set right before triggering the write mechanism. If more than **4 clock cycles** pass between these two actions, the hardware automatically cancels the write.

    * **Step A (Master Write Enable)**: Set the EEPROM Master Write Enable (`EEMPE` - **Bit 2**) bit in `EECR` to `1`, while keeping the write enable bit (`EEPE`) at `0`.

    * **Step B (Write Enable)**: Within the next **4 clock cycles**, set the EEPROM Write Enable (`EEPE` - **Bit 1**) bit in `EECR` to `1`. The CPU will automatically halt for two clock cycles while the internal hardware handles the physical EEPROM write cycle.

```c
void eeprom_write(uint16_t address, uint8_t data) {
    while (EECR & (1 << EEPE));   // wait for any previous write to finish

    EEAR = address;
    EEDR = data;

    EECR |= (1 << EEMPE);          // master write enable (must be set first...)
    EECR |= (1 << EEPE);           // ...then this actually starts the write, within 4 clock cycles
}
```

-----------------------------

#### Configuration for an EEPROM READ

Reading from EEPROM is much simpler, takes only one clock cycle, and does not require a safety lock protocol.

1. Check for Active Write Cycles
    You cannot read from the EEPROM if it is currently busy writing a byte.
    * **Action**: Poll the `EEPE` bit in the `EECR` register and wait until it reads `0`.

2. Set the Address
    * **Action**: Write the target memory address you want to look up into `EEARH` and `EEARL`.

3. Trigger the Read Command
    * **Action**: Set the EEPROM Read Enable (`EERE` - Bit 0) bit in `EECR` to `1`.

4. Fetch the Data
    * **Action**: The hardware immediately reads the data from the memory matrix and places it into the EEDR register during the next instruction cycle. You can now copy the data byte out of EEDR into your working software variable.

```c
uint8_t eeprom_read(uint16_t address) {
    while (EECR & (1 << EEPE));   // wait for any previous write to finish

    EEAR = address;                // set address (EEARL + EEARH handled together as EEAR)
    EECR |= (1 << EERE);           // start read
    return EEDR;                    // data is ready immediately after EERE for reads
}
```


## using `<avr/eeprom.h>` AVR-libc's built-in helper functions

```c
#include <avr/io.h>
#include <avr/eeprom.h>

// storing a 16-bit value
void save_brightness(uint16_t brightness) {
    eeprom_write_word((uint16_t*)0, brightness);
}

int main(void) {
    // Write a single byte to address 0
    eeprom_write_byte((uint8_t*)0, 42);

    // Read it back
    uint8_t value = eeprom_read_byte((uint8_t*)0);

    while (1) {
        // value now holds 42 (persists across power cycles)
    }
}
```


### example: boot counter

```c
#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdio.h>

#define BOOT_COUNT_ADDR 0

// uart_init(), uart_putchar(), FDEV_SETUP_STREAM as before

int main(void) {
    uart_init();
    stdout = &uart_output;

    uint32_t boot_count = eeprom_read_dword((uint32_t*)BOOT_COUNT_ADDR);

    // Handle first-ever boot: fresh/erased EEPROM reads as 0xFFFFFFFF
    if (boot_count == 0xFFFFFFFF) {
        boot_count = 0;
    }

    boot_count++;
    eeprom_write_dword((uint32_t*)BOOT_COUNT_ADDR, boot_count);

    printf("Boot count: %lu\n", boot_count);

    while (1) {
        // idle
    }
}
```



