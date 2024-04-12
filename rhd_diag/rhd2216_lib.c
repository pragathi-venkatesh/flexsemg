#include "rhd2216_lib.h"

static int dsp_offset_rem_en = 0; // active high

static void pabort(const char *s) {
	perror(s);
	abort();
}

int get_dsp_offset_rem_en(void) {
	return dsp_offset_rem_en;
}

int set_dsp_offset_rem_en(int en) {
	dsp_offset_rem_en = en;
	return 0;
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

// if all channels active, active_ch_msk = 0xffff. If only ch1 active, active_ch_msk = 0x01 etc...
// TODO better description
int rhd_convert(int fd, uint16_t active_chs_msk, uint16_t *data_buf, size_t buf_len) {
	// due to pipelining, first two rx results are garbage values.
	// function will only start writing to data buf after first two spi transfers
	int counter = -2;

	int ret;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};
	uint8_t command_word = 0;
	
	while (counter < buf_len) {
		for (int ch=0; ch<16; ++ch) {
			// skip if channel not part of active_ch_msk
			if ( !((0b1 << ch) & active_chs_msk) )
				continue; // do not increment counter

			command_word = 0;
			command_word |= ch & 0x3f;
			tx_buf[1] = command_word;
			tx_buf[0] = dsp_offset_rem_en & 0b1;
			ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);
			if (ret == -1) {
				pabort("spi transfer failed");
			}

			if (counter >= 0) {
				data_buf[counter] = (rx_buf[1] << 8) | rx_buf[0];
			}

			++counter;
		}
	}

	return 0;
}