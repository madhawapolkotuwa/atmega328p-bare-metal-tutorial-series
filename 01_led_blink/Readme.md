# AVR LED Blink - ATmega328P (bare-metal, no Arduino core)

## Video :-

[![Youtube Video](https://img.youtube.com/vi/KKl6COGqNzk/0.jpg)](https://www.youtube.com/watch?v=KKl6COGqNzk)

This project blinks the onboard LED on an Arduino Uno R3 by writing directly     
to the ATmega328P's I/O registers instead of using the Arduino `digitalWrite()`  
API. It's built with `avr-gcc`/`avrdude` and a plain **Makefile**.   

### Hardware background

The Arduino Uno R3 is built around the ATmega328P, an 8-bit AVR  
microcontroller. On the Uno board, digital pin D13 is wired to the   
microcontroller pin PB5 (Port B, bit 5), and that same pin also drives the   
onboard LED named as "L" - which is exactly why D13/PB5 is the   
classic "hello world" pin for AVR blink examples.    

The board also exposes SPI's SCK on the same physical pin, since PB5 doubles
as SCK on this chip - it's shared silicon, not a coincidence.

### How AVR GPIO registers work

The ATmega328P groups its I/O pins into ports named `B`, `C`, `D`, etc. Each
port is controlled by three 8-bit registers, one bit per physical pin:

Register | Purpose | Effect
---------|---------|--------
`DDRx` (Data Direction Register) | Sets each pin as input (`0`) or output (`1`) | Must be configured before the pin is use. |
`PORTx`(Data Register) | When pin is an **output**: drivers it high (`1`) or low (`0`). <br/> When pin is an **input**: enables (`1`) or disables (`0`) the internal pull-up registor. | Controls the actual voltage on the pin.
`PINx`(Input Register) | Reads the current logic level of the pin register | Also has a special quirk: writing a `1` to a bit in `PINx` toggels that pin's `PORTx` bit, regardless of its current state.

For Port B, bit 5 corresponds to physical pin PB5 (Uno D13), so the code uses
the constants `DDB5` / `PORTB5` / `PINB5` - these are just the number 5,
named for readability (they come from `<avr/io.h>)`.

**Bit manipulation used in `main.c`**    
```c
DDRB = DDRB | (1 << DDB5); // set bit 5 to 1, leave all other bits untouched
```
`1 << DDB5` produces `0b00100000`. OR-ing it into DDRB sets only that bit
to 1 (configuring PB5 as an output) without disturbing the direction of the
other 7 pins on the port.

```c
PORTB = PORTB | (1 << PORTB5);   // PB5 -> HIGH  (LED on)
// or PORTB |= (1 << PORTB5);

PORTB = PORTB & ~(1 << PORTB5);  // PB5 -> LOW   (LED off)
// or PORTB &= ~(1 << PORTB5);
```

* `|= (1 << n)` : sets bit `n` to `1` (turn on), leaving other bits alone.
* `&= ~(1 << n)` : clears bit `n` to `0` (turn off): `~(1 << n)` is all `1`s
except a `0` in position n, so AND-ing only that bit can be cleared.

----------------

**using `PINx`**     
```c
PINB = (1 << PINB5); // toggles PB5 instead of setting/clearing it explicitly
```
takes advantage of the AVR-specific "write-1-to-toggle" behavior of the
`PINx` register - a single line replaces the set/clear pair, at the cost of
being less obvious to a reader unfamiliar with the trick.

**The delay**

`_delay_ms(1000)` comes from `<util/delay.h>` and busy-waits (blocks the CPU
in a calibrated loop) for the given number of milliseconds. It relies on
F_CPU being correctly #defined at compile time:  
hence `-DF_CPU=16000000UL` in the `Makefile`, matching the Uno's **16 MHz crystal**.

**Program flow**

1. `DDRB |= (1 << DDB5)` : one-time setup: PB5 becomes an output.
2. Inside `while(1)` (runs forever, since a microcontroller has no OS to
return to):
    * Drive PB5 high → LED on → wait 1 s.
    * Drive PB5 low → LED off → wait 1 s.
    * Repeat.

### Makefile walkthrough

```makefile
BUILD_DIR := build
OUTPUT_FILE := blink

source := src/main
out	   := $(BUILD_DIR)/$(OUTPUT_FILE)
```
Variables: the OUTPUT_FILE, where build artifacts go, the source file
(without extension), and the base name of every output file
(`build/out.o`, `build/out.bin`, `build/out.hex`).

>> Note: source := `src/main` assumes `main.c` lives in a `src/` folder.   
>> If your `main.c` is in the project root, this should be `source := main`,     
>> or move the file into `src/`.


```makefile
all:
	mkdir -p $(BUILD_DIR)
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o $(out).o $(source).c
	avr-gcc -mmcu=atmega328p -o $(out).bin $(out).o
	avr-objcopy -O ihex -R .eeprom  $(out).bin  $(out).hex
```

The default target, run three toolchain steps:   
1. Compile (`-c`) `main.c` into an **object** file `.o`.
    * `-Os` : optimize for size (important on a chip with only 32 KB flash).
    * `-DF_CPU=16000000UL` : tells `<util/delay.h>` the clock speed so its timing loops are calibrated correctly.
    * `-mmcu=atmega328p` : target CPU variant, selects the correct register definitions and instruction set.

2. Link the object file into an ELF binary (`.bin`) - resolves all symbols and produces a single executable image, still in ELF format at this point despite the `.bin` extension.
3. Convert the ELF binary into an Intel HEX file (`.hex`) with `avr-objcopy`, which is the format `avrdude` (and most AVR programmers) expect for flashing. `-R .eeprom` strips out any **EEPROM** section so only flash-memory content is included.

```makefile
clean:
	rm -rf $(BUILD_DIR)
```
Removes the entire build directory: a fresh make all afterward rebuilds everything from scratch.

```makefile
flash:
	sudo avrdude -F -V -c arduino -p atmega328p -P /dev/ttyACM0 -b 115200 -U flash:w:$(out).hex
```

Uploads the compiled `.hex` file to the chip using avrdude:

* `-c arduino` : programmer type: the Uno's built-in bootloader protocol
(talking over serial through the 16U2 USB-to-serial chip, not a real ISP
programmer).
* `-p atmega328p` : target chip.
* `-P /dev/ttyACM0` — serial port the Uno enumerates as on Linux (this can
differ, check with ls /dev/tty* before/after plugging in the board; it
may be /dev/ttyUSB0 on some systems).
* `-b 115200` : baud rate expected by the Uno's stock bootloader.
* `-U flash:w:$(out).hex` : write (w) the hex file's contents to the
flash memory section.
* `sudo` is needed only if your user lacks permission on the serial device
(fixable long-term by adding your user to the `dialout` group instead).

**Building and flashing**
```bash
make             # compiles main.c -> build/blink.hex
make flash       # uploads build/blink.hex to the board
make clean       # removes build/ entirel
```
--------------------
--------------------
--------------------

## Manual delay 

### The core idea: CPU cycles = time

At `16MHz`, the CPU executes `16,000,000 cycles per second`,     
so 1 cycle takes `1/16,000,000` seconds ≈ `62.5 nanoseconds`.    
If you know how many cycles a loop takes to execute,    
you can calculate how many iterations you need for any given delay.

* Using inline assembly `nop` for exact, guaranteed cycle counts
    ```c
    void delay_cycles(uint16_t cycles) {
        while (cycles--) {
            __asm__ __volatile__("nop");   // exactly 1 cycle, guaranteed
        }
    }
    ```
* Here's the `delay_ms()` built on top of `delay_cycles()`
    ```c
    #define F_CPU 16000000UL

    // approx cycles per loop iteration: 1 (nop) + ~3 (decrement + compare + branch)
    #define CYCLES_PER_LOOP 4

    void delay_cycles(uint16_t cycles) {
        while (cycles--) {
            __asm__ __volatile__("nop");
        }
    }

    void delay_ms(uint16_t ms) {
        while (ms--) {
            delay_cycles(F_CPU / 1000UL / CYCLES_PER_LOOP);   // cycles needed for 1ms
        }
    }
    ```
------

The actual number of cycles the `while (cycles--) { nop; }` loop takes per iteration depends on exactly what assembly the compiler generates.    
which varies with optimization level (`-O0` vs `-Os` vs `-O2`),      
and isn't something you can be fully certain of without checking.    
There's also overhead from the `delay_cycles()` function call itself (pushing arguments, calling, returning) that isn't accounted for in the "4 cycles" estimate at all - that overhead happens once per `delay_cycles()` call, not once per nop, so it adds a small fixed error each time `delay_ms()`'s loop calls it.

### How to actually find the real number: disassemble the linked ELF

```bash
avr-objdump -d build/out.elf
```

```asm
.L2:
   cp   r18, r1     ; 1 cycle
   cpc  r19, r1     ; 1 cycle
   brne .L3         ; 1 cycle (not taken) / 2 cycles (taken)
   ret

.L3:
   nop              ; 1 cycle
   subi r18, 0x01   ; 1 cycle
   sbc  r19, r1     ; 1 cycle
   rjmp .L2         ; 2 cycles (always taken, it's an unconditional jump)
```

* Adding these up for one full iteration (compare + branch-taken + nop + decrement + jump-back):
    ```
    1 (cp) + 1 (cpc) + 2 (brne taken) + 1 (nop) + 1 (subi) + 1 (sbc) + 2 (rjmp) = 9 cycles
    ```

*   That's roughly double the `4 cycles` we originally assumed - which lines up almost exactly with our measured `~2x` delay.    
    ```c
    #define CYCLES_PER_LOOP 9   // confirmed via disassembly: cp+cpc+brne(taken)+nop+subi+sbc+rjmp
    ```
    Now:
    ```
    16,000,000 / 1000 / 9 ≈ 1777
    ```

* Full `delay_cycles(1777)` call cost:     
    * Entry: movw r18, r24 → 1 cycl  
    * 1777 loop iterations × 9 cycles → 15,993 cycles    
    * Final exit iteration (count reaches 0, branch not taken): cp+cpc+brne(not taken)+ret → 1+1+1+4 = 7 cycles  
    * Total inside delay_cycles: 1 + 15,993 + 7 = **16,001 cycles**  

* Overhead from `delay_ms`'s per-millisecond loop
    ```asm
    .L6:
        ldi r24, 0xF1   ; 1
        ldi r25, 0x06   ; 1
        call delay_cycles ; 4 (call itself, execution counted separately above)
        sbiw r28, 0x01  ; 2
        rjmp .L5        ; 2
    .L5:
        sbiw r28, 0x00  ; 2
        brne .L6        ; 2 (taken)
    ```
    Overhead per millisecond iteration: 1+1+4+2+2+2+2 = **14 cycles**

* Total cycles per millisecond
    ```
    16,001 (inside delay_cycles) + 14 (delay_ms loop overhead) = 16,015 cycles
    ```
    At `16MHz`:
    ```
    16,015 cycles / 16,000,000 cycles/sec = 0.0010009 sec ≈ 1.0009 ms
    ```

    That's within `0.09%` of exactly `1ms`, essentially spot-on.     
    For `delay_ms(1000)`, this compounds to roughly 1.0009 seconds total.

------------

>> Note: keep in mind - this cycles-per-loop value will change if we change the optimization level(`-O0`, `-Os`, `-O1`). It's not something you can just assume and be fully confident about, without actually checking the assembly yourself.

>> So, instead of doing this manually every time, we can simply use the `<util/delay.h>` header instead. and built in helper functions like: `_delay_us()`, `_delay_ms`.

### Reference
* [ATmega328P, 8-bit AVR datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)
* [Arduino UNO R3 datasheet (board topology, pinout, ATmega328P processor
confirmation)](https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf)
* [Atmel AVR 8-bit Instruction Set](https://robotics.ee.uwa.edu.au/nano/doc/Atmel-AVR-InstructionSet.pdf)


