![](title.png)

# Arduino Bare-Metal Programming with C - `No IDE`

A beginner-to-master series on programming the Arduino Uno (`ATmega328P`) directly at the register level.    
    **no Arduino IDE**, **no Arduino core library**,     
    `just C`, `avr-gcc`, and a `datasheet`.

------------------

## Why bare-metal?

The Arduino IDE and `Arduino.h` hide almost everything interesting about the chip you're programming. This series strips that away. Every tutorial talks straight to the ATmega328P's registers - `DDRx`, `PORTx`, `TCCRx`, `UCSRx`, `TWCR`, and friends - using nothing but:


* C, compiled with avr-gcc
* avr-libc for register/interrupt-vector definitions
* Make to build
* avrdude to flash, over the Uno's stock bootloader

No sketches. No `.ino` files. No hidden `setup()/loop()` magic ,     
 just `main()` and the `datasheet`.

-------------------------

## Series roadmap

| #  | Topic | Key concepts |
|---|-------|--------------|
| 00 | [Setup & Installation](./00_setup/) | Toolchain (`avr-gcc`, `avr-libc`, `avrdude`, `make`), **Linux/macOS/Windows** install, **VS Code config**. |
01 | [LED Blink](./01_led_blink/) | `DDRx`, `PORTx`.
02/03 | [Button Input](./02_button_input/) <br> [Interrupt](./02_button_input/) | Polling input, pull-ups, then `PCINT/INT0` external interrupts
04/05 | [Timers](./04_timers/) <br> [PWM](./05_pwm/) | `Timer0/1/2`, prescalers, fast PWM for LED dimming and servo control
06 | [UART](./06_uart/) | Polling TX/RX, then interrupt-driven serial I/O 
07 | [ADC Read](./07_adc/) | Single-channel ADC, then multi-channel switching
08 | [SPI (Master & Slave)](./08_spi/) | Full-duplex SPI
09 | [TWI/I2C (Master & Slave)](./09_i2c/) | I2C protocol from scratch, real sensor read (**BMP180**)
10 | [EEPROM Read/Write](./10_eeprom/) | Persistent settings storage, wear considerations
11 | [Watchdog Timer](./11_watchdog/) | Fault recovery, WDT-triggered reset vs interrupt mode
11 | [Sleep Modes](./12_sleep_modes/) | Power-down mode, wake-on-interrupt, power measurement
12 |[ Custom Serial Protocol](./13_custom_serial_protocol/) | Framing, checksums, building a protocol over UART


