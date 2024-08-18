#include "arduino_spi_lib.h"

int spi_config(uint8_t mode, uint8_t bpw, uint32_t speed) {
    int ret;
	#TODO implement me!
	ret = -1;
	return ret;
}

// rx buf must have same size as tx buf
// result stored in rx_buf
int rhd_spi_xfer(uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf) {
    int ret;
	# TODO implement me!
	ret = -1;
    return ret;
}

// making a wrapper function so that code is controller-independent.
// code can import any controller spi library and use its delay_us 
// instead of calling any hw-specific timers.
void delay_us(unsigned long us) {
    delayMicroseconds(us);
}