#include "rhd2216_lib.h"

static void pabort(const char *s) {
	perror(s);
	abort();
}

int rhd_reg_read(int fd, uint8_t reg_num, uint8_t *result) {
	int ret;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};

	uint8_t command_word = 0;
	command_word |= 0b11000000;
	command_word |= reg_num & 0x3f;
	tx_buf[0] = command_word;
	tx_buf[1] = 0;

	// clear out any previous data from pipelining
	rhd_spi_xfer(fd, tx_buf, N, rx_buf);
	rhd_spi_xfer(fd, tx_buf, N, rx_buf);

	// do the actual read now
	ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);
	if (ret == -1)
		pabort("spi xfer failed");
	
	*result = rx_buf[1];
	return ret;
}

int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data) {
	int ret;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};

	// do write
	uint8_t command_word = 0;
	command_word |= 0b10000000;
	command_word |= reg_num & 0x3f;
	tx_buf[0] = command_word;
	tx_buf[1] = reg_data;

	// do the write
	ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);
	if (ret == -1) {
		pabort("spi xfer failed");
	}

	// read the register 2 more times
	// to clear previous data from pipelining
	tx_buf[0] = 0b11000000 & (reg_num & 0x3f); // read command word
	ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);
	ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);

	uint8_t read_data = rx_buf[1];
	if (read_data != reg_data) {
		printf(
			"WARNING: tried to write 0x%02x but got %0x02x instead.\n", 
			reg_data, 
			read_data);
	}

	return 0;
}