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