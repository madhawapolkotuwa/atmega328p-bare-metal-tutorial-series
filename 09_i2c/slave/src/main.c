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

int main(void) {
    twi_slave_init();
    int cnt = 0;
    while (1) {
        if (data_ready) {
            data_ready = 0;
            // use received_byte here, e.g. update response_byte based on a command
            response_byte = received_byte + cnt;   // simple example: echo back +1
            cnt++;
            if(cnt > 16) cnt = 0;
        }
    }
}