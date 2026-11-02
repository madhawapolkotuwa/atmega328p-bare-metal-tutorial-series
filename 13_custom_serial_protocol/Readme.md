A custom framing protocol solves a real problem with raw UART: without structure, you can't tell where one message ends and the next begins, or whether the data got corrupted in transit. Let's build a simple but solid frame format.

## Frame format

```
[START] [LENGTH] [PAYLOAD...] [CHECKSUM]
```

| Field | Size | Purpose |
|---|---|---|
| START | 1 byte | Fixed marker (e.g. `0xAA`) so the receiver can find the beginning of a frame |
| LENGTH | 1 byte | Number of payload bytes that follow |
| PAYLOAD | 0–N bytes | Actual data |
| CHECKSUM | 1 byte | XOR of LENGTH + all PAYLOAD bytes — lets receiver detect corruption |

Using XOR checksum since it's cheap on an 8-bit AVR (no multiply/divide needed), and good enough to catch single-byte corruption and most transmission noise, though not as strong as a CRC.

## Step 1 — Sending a frame

```c
#include <avr/io.h>

// uart_init(), uart_putchar() as before

void send_frame(const uint8_t *payload, uint8_t length) {
    uint8_t checksum = length;

    uart_putchar(0xAA);         // START byte
    uart_putchar(length);        // LENGTH byte

    for (uint8_t i = 0; i < length; i++) {
        uart_putchar(payload[i]);
        checksum ^= payload[i];   // fold each payload byte into the checksum
    }

    uart_putchar(checksum);      // CHECKSUM byte
}
```

**Why start the checksum at `length`, not 0:** including the length byte in the checksum catches corruption of the length field itself (e.g. a bit flip that made the receiver expect the wrong number of bytes) — not just corruption inside the payload.

## Step 2 — Receiving a frame (state machine parser)

Since bytes arrive one at a time over UART, the receiver needs to track *where* it is within a frame — this is a small state machine.

```c
#include <avr/io.h>

// uart_getchar() as before

typedef enum {
    WAIT_START,
    WAIT_LENGTH,
    WAIT_PAYLOAD,
    WAIT_CHECKSUM
} frame_state_t;

uint8_t receive_frame(uint8_t *payload_out, uint8_t *length_out) {
    frame_state_t state = WAIT_START;
    uint8_t length = 0;
    uint8_t index = 0;
    uint8_t checksum = 0;

    while (1) {
        uint8_t byte = uart_getchar();   // blocks until a byte arrives

        switch (state) {
            case WAIT_START:
                if (byte == 0xAA) {
                    state = WAIT_LENGTH;
                }
                // if not 0xAA, just keep waiting - resyncs automatically on noise
                break;

            case WAIT_LENGTH:
                length = byte;
                checksum = byte;   // start folding checksum, same as sender
                index = 0;
                state = (length > 0) ? WAIT_PAYLOAD : WAIT_CHECKSUM;
                break;

            case WAIT_PAYLOAD:
                payload_out[index++] = byte;
                checksum ^= byte;
                if (index >= length) {
                    state = WAIT_CHECKSUM;
                }
                break;

            case WAIT_CHECKSUM:
                *length_out = length;
                if (byte == checksum) {
                    return 1;   // valid frame received
                } else {
                    return 0;   // checksum mismatch - corrupted frame
                }
        }
    }
}
```

**Why `WAIT_START` just keeps waiting on a mismatch (rather than erroring):** this makes the parser self-resynchronizing. If noise or a partial frame leaves garbage on the line, the receiver just keeps scanning for the next `0xAA` rather than getting permanently stuck — a key robustness property for any framing protocol.

## Step 3 — Full example: send a 3-byte sensor reading, receive + validate it

```c
#include <avr/io.h>
#include <stdio.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putchar(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

char uart_getchar(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

int uart_printf_putchar(char c, FILE *stream) {
    if (c == '\n') uart_putchar('\r');
    uart_putchar(c);
    return 0;
}
static FILE uart_output = FDEV_SETUP_STREAM(uart_printf_putchar, NULL, _FDEV_SETUP_WRITE);

void send_frame(const uint8_t *payload, uint8_t length) {
    uint8_t checksum = length;
    uart_putchar(0xAA);
    uart_putchar(length);
    for (uint8_t i = 0; i < length; i++) {
        uart_putchar(payload[i]);
        checksum ^= payload[i];
    }
    uart_putchar(checksum);
}

typedef enum { WAIT_START, WAIT_LENGTH, WAIT_PAYLOAD, WAIT_CHECKSUM } frame_state_t;

uint8_t receive_frame(uint8_t *payload_out, uint8_t *length_out) {
    frame_state_t state = WAIT_START;
    uint8_t length = 0, index = 0, checksum = 0;

    while (1) {
        uint8_t byte = uart_getchar();
        switch (state) {
            case WAIT_START:
                if (byte == 0xAA) state = WAIT_LENGTH;
                break;
            case WAIT_LENGTH:
                length = byte;
                checksum = byte;
                index = 0;
                state = (length > 0) ? WAIT_PAYLOAD : WAIT_CHECKSUM;
                break;
            case WAIT_PAYLOAD:
                payload_out[index++] = byte;
                checksum ^= byte;
                if (index >= length) state = WAIT_CHECKSUM;
                break;
            case WAIT_CHECKSUM:
                *length_out = length;
                return (byte == checksum) ? 1 : 0;
        }
    }
}

int main(void) {
    uart_init();
    stdout = &uart_output;

    printf("Frame protocol test starting...\n");

    // Simulate sending a 3-byte "sensor reading" frame to ourselves
    uint8_t sensor_data[3] = {0x12, 0x34, 0x56};
    send_frame(sensor_data, 3);

    // Since TX/RX are looped externally (or just testing parser logic),
    // read back whatever comes in - in real use, this runs on a separate device
    uint8_t received_payload[32];
    uint8_t received_length;
    uint8_t valid = receive_frame(received_payload, &received_length);

    if (valid) {
        printf("Frame OK, length=%u, data:", received_length);
        for (uint8_t i = 0; i < received_length; i++) {
            printf(" 0x%02X", received_payload[i]);
        }
        printf("\n");
    } else {
        printf("Checksum FAILED - frame corrupted\n");
    }

    while (1) {
        // idle
    }
}
```

**Note on testing this on a single board:** `send_frame()` and `receive_frame()` both go through the same physical TX (D1) pin out and RX (D0) pin in — they won't loop back to each other automatically unless you **physically jumper D1 to D0** on the same Uno (a simple wire bridging TX→RX), or you run sender and receiver on two separate boards wired UART-to-UART (TX→RX, RX→TX, common GND) — the classic setup for testing this kind of protocol between two AVRs, or between the AVR and, say, the ESP32 you've been using for SPI/I2C testing.

## Step 4 — Testing corruption detection

To confirm the checksum actually catches errors, try deliberately flipping a bit before sending:

```c
sensor_data[1] ^= 0x01;   // introduce a fake bit-flip corruption
send_frame(sensor_data, 3);
```
This should make `receive_frame()` return `0` (checksum mismatch) rather than silently accepting bad data — worth trying once to confirm the whole pipeline actually rejects corrupted frames as intended.

### Quick reference

| Frame field | Value/rule |
|---|---|
| START | Fixed `0xAA` |
| LENGTH | Number of payload bytes (0–255) |
| PAYLOAD | Your actual data |
| CHECKSUM | XOR of LENGTH and all PAYLOAD bytes |

Want to extend this with a **timeout** (so `receive_frame()` doesn't block forever if a frame never completes), add a second command/type byte for a richer protocol (e.g. distinguishing "sensor reading" vs "command" vs "ack" frames), or move on to pulling together the full step-by-step README covering everything we've built so far?