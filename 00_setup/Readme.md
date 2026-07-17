# Setup & Toolchain

Series: Arduino Bare-Metal Programming with C (No IDE)   
Board: Arduino Uno [(ATmega328P, 8-bit AVR)](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)   
Primary OS: `Linux` : with `macOS` and `Windows` instructions included   

## 1. What **`bare-metal`** means in this series

**No Arduino IDE**. No `Arduino.h`. No `digitalWrite()`, no `Serial.println()`.

Every tutorial in this series talks directly to the `ATmega328P`'s registers, using:
* **C** (compiled with `avr-gcc`, no Arduino core library)
* **avr-libc** (the standard C library for AVR  gives us `<avr/io.h>`, register names, interrupt vectors)
* **Make** (to build)
* **avrdude** (to flash, using the Uno's existing bootloader over `serial/USB`)

The Arduino Uno's bootloader stays on the chip, we are not replacing it in this tutorial. We're just writing our own firmware instead of Arduino sketches, and flashing it the same way the IDE would (avrdude, over the USB-serial port), just from the command line. 


--------------------------

## 2. Hardware you need

item | Notes
-----|-------|
Arduino Uno (or Uno-compatible board) |ATmega328P, 16 MHz, USB-serial onboard |
USB-B cable | For power + programming via bootloader |
Breadboard + jumper wires | For Tutorial 1 onward (LED, button, resistor, etc.) |

No external programmer (USBasp/ISP) is required to start, the Uno's factory bootloader is enough for every tutorial until we reach the custom-bootloader episode. If you do have a USBasp lying around, keep it,     
 it becomes useful later for fuse bits and self-programming experiments.

-----------

## 3. Toolchain overview
Tool | Purpose |
-----|-------|
`avr-gcc` | C compiler targeting AVR instruction set | 
`avr-libc` | Register definitions, interrupt vector macros, core headers
`avr-binutils` (`avr-objcopy, avr-objdump, avr-size`) | Convert ELF → HEX, inspect binaries, check flash/RAM usage |
`avrdude` | Flash the compiled HEX to the chip over the bootloader | 
`make` | Build automation (each tutorial ships a `Makefile`) | 
`avr-gdb + simavr` (optional) | Simulate/debug without hardware | 
Editor (your choice) | VS Code recommended, but any text editor + terminal works

## 4. Installation - Linux - *primary platform*

```bash
sudo apt update
sudo apt install gcc-avr avr-libc binutils-avr avrdude make
```

Log out and back in (or reboot) for the group change to take effect.
```bash
avr-gcc --version
avrdude --version
```

Find your board's port:
```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Plug the Uno in and re-run the command - the new device that appears (usually `/dev/ttyACM0` on Uno, `/dev/ttyUSB0` on some clones) is your port.
>> Note it down, every tutorial's flash command needs it.

## 5. Installation - macOS

Using Homebrew:
```bash
brew tap osx-cross/avr
brew install avr-gcc avrdude make
```
>> `avr-gcc` on **macOS** comes from the `osx-cross/avr` tap because Homebrew's core repo dropped it. If `brew install avr-gcc` alone fails, the tap above is required.

Verify:
```bash
avr-gcc --version
avrdude -v
```

Find your board's port:
```bash
ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial* 2>/dev/null
```
>> Use the `/dev/cu.*` device (not `/dev/tty.*`) for programming.

## 6. Installation - Windows

Two supported paths. `WSL2`(window subsystem for linux) is recommended since the rest of the series (Makefiles, shell commands) is written for Linux and will work unmodified.

### Option A : `WSL2` (recommended)
1. Install `WSL2 + Ubuntu LTS`: open PowerShell as Administrator and run:
    ```powershell
    wsl --install -d Ubuntu
    ```
2. Inside the Ubuntu shell, follow the Linux instructions in Section 4 exactly.
3. USB passthrough: `WSL2` doesn't see **USB devices** by default. Install `usbipd-win` on the Windows side:
    ```powershell
    winget install usbipd
    ```

    Then, each time you plug in the Uno:  

    ```powershell
    usbipd list
    usbipd bind --busid <BUSID>
    usbipd attach --wsl --busid <BUSID>
    ```
    >> Inside WSL, confirm it shows up as `/dev/ttyACM0` or `/dev/ttyUSB0`.


### Option B: Native Windows (MSYS2)
1. Install `MSYS2`.
2. Open the **MSYS2 MinGW64** terminal and run:
    ```bash
    pacman -S mingw-w64-x86_64-avr-gcc avr-libc avrdude make
    ```
3. Find your port in Device Manager under Ports (**COM & LPT**) - it'll be something like COM5. You'll pass `-P COM5` to avrdude instead of a `/dev/...` path.

---------------

## 7. Verify the whole toolchain with a smoke test


`test.c`:    
```c
int main(void) {
    while (1) {}
}
```
Compile:
```bash
avr-gcc -Os -mmcu=atmega328p -DF_CPU=16000000UL -o test.elf test.c
avr-objcopy -O ihex -R .eeprom test.elf test.hex

# or 
avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o test.o test.c
avr-gcc -mmcu=atmega328p -o test.bin test.o
avr-objcopy -O ihex -R .eeprom  test.bin  test.hex
```

Flash (replace the port with yours):
```bash
sudo avrdude -c arduino -p atmega328p -P /dev/ttyACM0 -b 115200 -U flash:w:test.hex:i
```

If avrdude reports something like:
```
avrdude: verifying ...
avrdude done, thank you.
```
* your toolchain is fully working end to end. If it fails, check:
* Correct port (`Section 4/5/6 `above)
* You're in the `dialout` group (Linux) and re-logged in
* No other program (Arduino IDE Serial Monitor, etc.) has the port open
* Baud rate: most Unos use `115200`; older bootloaders sometimes use `57600`

-----------

## 8. Repository / project structure for this series
Each tutorial folder is self-contained:
```
tutorial-XX-topic/
├── README.md        <- steps, register explanations, wiring diagram
├── Makefile          <- build + flash targets
├── src/
│   └── main.c
└── include/          <- (only once we split into multiple files, later tutorials)
```

Standard `Makefile` targets every tutorial will use (introduced properly in Tutorial 1):
```bash
make        # build .hex
make flash  # flash to board
make clean  # remove build artifacts
```

## 9. What's next

Tutorial 1 - LED Blink (no delay library, raw register toggling on PORTB)
First real register-level program: `DDRx`, `PORTx`, and a manual busy-wait loop.

----------------------

## 10. Editor setup (optional but recommended)

VS Code with the extensions:
* C/C++ (Microsoft) - for syntax highlighting/IntelliSense
* Optionally configure `c_cpp_properties.json` to point at your `avr-gcc` include path so IntelliSense resolves `<avr/io.h>` correctly. 

>> Any editor works fine - **Vim**, **Neovim**, **Sublime**, etc. The series has no IDE dependency by design.



--------------
--------------
<br>
<br>

## Fuse bit setup

* [Learn About ATmega328P Fuse Bits and How to Use Them with an External Crystal Oscillator](https://www.allaboutcircuits.com/projects/atmega328p-fuse-bits-and-an-external-crystal-oscillator/)
* [How to Change Fuse Bits of AVR Atmega328p - 8bit Microcontroller Using Arduino](https://www.instructables.com/How-to-change-fuse-bits-of-AVR-Atmega328p-8bit-mic/)


