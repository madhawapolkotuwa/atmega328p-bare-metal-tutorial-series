# USART (Universal Synchronous and Asynchronous serial Receiver and Transmitter)

UART is straightforward on the Uno since the USB port is the UART,   
pins `D0` (RX) and `D1` (TX) connect through the onboard USB-to-serial chip,     
so whatever send out UART shows up directly in the Serial Monitor at the baud correct baud rate.


## Example: 9600 Baud, 8-bit Data, and 1 Stop Bit (8N1)

### Step-by-Step USART Register Settings

#### 1. Calculate and Set the Baud Rate (`UBRR0`)
To achieve 9600 baud in asynchronous normal mode (U2X0 = 0), the register value formula is:  

$$\text{UBRR0} = \frac{f_{osc}}{16 \times \text{Baud}} - 1$$

* For a 16 MHz ($16,000,000\text{ Hz}$) clock and 9600 Baud:

    $$\text{UBRR0} = \frac{16,000,000}{16 \times 9,600} - 1 = 104.167 - 1 \approx \mathbf{103} \text{ (or } \mathtt{0x67}\text{)}$$

* Since `UBRR0` is a 16-bit value split across two 8-bit registers, we write the high and low bytes:
```c
UBRR0H = 0x00; // Upper 8 bits (103 fits entirely in the lower byte)
UBRR0L = 0x67; // Lower 8 bits (103 in decimal)
```

#### 2. Configure the Frame Format (UCSR0C)
To set up 8-bit data size, No Parity, and 1 Stop Bit (8N1):  
* Asynchronous Mode: Set `UMSEL01` and `UMSEL00` to `0` (default).

* Parity Mode (Disabled): Set `UPM01` and `UPM00` to `0` (default).

* Stop Bit (1 Stop Bit): Set `USBS0` to `0` for `1` stop bit (`1` would configure `2` stop bits).

* Character Size (8-bit): Set `UCSZ01` and `UCSZ00` to `1` (along with `UCSZ02` in `UCSR0B` kept as `0`).
* Action: Write `0x06` (binary `0b00000110`) to `UCSR0C`.

#### 3. Enable Transmitter and Receiver (`UCSR0B`)
To turn on the serial hardware for both sending and receiving data:  
* **Receiver Enable (`RXEN0`)**: Set to `1`.
* **Transmitter Enable (`TXEN0`)**: Set to `1`.
* **Action**: Write `0x18` (binary `0b00011000`) to `UCSR0B`.
    ```c
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    ```

Full test code:

```c
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void) {
    // 1. Set Baud Rate 
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);

    // 2. Set Frame Format: 8-bit Data, 1 Stop Bit, No Parity (8N1)
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    
    // 3. Enable Receiver and Transmitter
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
}

void uart_putchar(char data) {
    // Wait for empty transmit buffer (UDRE0 flag is set to 1 when empty)
    while (!(UCSR0A & (1 << UDRE0)));  

    // Put data into buffer, sends the data
    UDR0 = data;
}

char usart_receive(void) {
    // Wait for data to be received (RXC0 flag is set to 1 when there's data)
    while (!(UCSR0A & (1 << RXC0)));
    
    // Get and return received data from buffer
    return UDR0;
}

void uart_print(const char *str) {
    while (*str) {
        uart_putchar(*str++);
    }
}

int main(void) {
    uart_init();

    uart_print("Hello from ATmega328P!\r\n");

    while (1) {
        char c = usart_receive();
        uart_putchar(c); // echo-back (read a byte off RX, write the same byte back out on TX.)
    }
}
```

#### Wiring up `printf`

AVR-libc's `printf` doesn't know how to output anything by default.

we attach it to a stream by telling it which function to call per character. This is where `fdev_setup_stream` and `stdout` come in.

```c
#include <stdio.h>

int uart_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r', stream);  // translate \n -> \r\n for terminals
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

static FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

// now we can use printf
```

* `FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE)` builds a `FILE` stream backed by our `uart_putchar` function - `NULL` means no read function (write-only stream).   

* Assigning `stdout = &uart_output;` makes every subsequent `printf()` route through UART.   

* Translating `\n` → `\r\n` matters because most terminals (including Arduino's Serial Monitor) expect a carriage return + line feed to move to a fresh line properly.

------------------------

## Example: 115200 Baud, 8-bit Data, and 1 Stop Bit (8N1)

If we just change the BAUD 9600 -> 115200:
```c
#define F_CPU 16000000UL
#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)
```
$$\text{UBRR0} = \frac{f_{osc}}{16 \times \text{Baud}} - 1$$
$$\text{UBRR0} = \frac{16,000,000}{16 \times \text{115,200}} - 1 = 8.68 -1 = 7.68 -> truncates to 8$$

That truncation introduces a baud rate error of about 3.7%, which is on the edge of what most UART receivers tolerate reliably (usually want to stay under ~2-3%). It'll often still work, but can occasionally cause garbled characters, especially over longer strings.

* Enable Double Speed Mode (`U2X0`)
To achieve 115200 baud in asynchronous double speed mode (`U2X0 = 1`), the register value formula is:  

$$\text{UBRR0} = \frac{f_{osc}}{8 \times \text{Baud}} - 1$$

* For a 16 MHz ($16,000,000\text{ Hz}$) clock and 9600 Baud:

    $$\text{UBRR0} = \frac{16,000,000}{8 \times 115,200} - 1 = 17.36 - 1 \approx \mathbf{16}$$

truncates to 16. This gives an error of about 2.1% - noticeably better than the non-doubled version, and generally reliable for 115200.

```c
#include <avr/io.h>

#define F_CPU 16000000UL
#define BAUD 115200
#define UBRR_VALUE ((F_CPU / (8UL * BAUD)) - 1)   // note: 8, not 16, because of U2X0

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);

    UCSR0A |= (1 << U2X0);                          // double speed mode
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
```






