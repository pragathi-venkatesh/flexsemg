#include <stdlib.h>
#include <SPI.h>

int spi_config(uint8_t mode, uint8_t bpw, uint32_t speed);
int rhd_spi_xfer(uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf); 
void delay_us(unsigned long us);
