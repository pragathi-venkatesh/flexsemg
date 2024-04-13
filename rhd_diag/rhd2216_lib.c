#include "rhd2216_lib.h"

static int dsp_offset_rem_en = 0; // active high

// register defaults
const rhd_reg_t rhd2216_reg_list[] = {
    // reg_num, default val, read_only, short_desc, loooooooooong_desc
    {0, 0b11011110, 0b11011110, 0, "ADC Configuration and Amplifier Fast Settle", ""},
    {1, 0b00100000, 0b00100000, 0, "Supply Sensor and ADC Buffer Bias Current", ""},
    {2, 0b00101000, 0b00101000, 0, "MUX Bias Current", ""},
    {3, 0b00000000, 0b00000000, 0, "MUX Load, Temperature Sensor, and Auxiliary Digital Output", ""},
    {4, 0b11010110, 0b11000100, 0, "ADC Output Format and DSP Offset Removal", ""},
    {5, 0b00000000, 0b00000000, 0, "Impedance Check Control", ""},
    {6, 0b00000000, 0b00000000, 0, "Impedance Check DAC", ""},
    {7, 0b00000000, 0b00000000, 0, "Impedance Check Amplifier Select", ""},
    {8, 0b00011110, 0b00011011, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {9, 0b00000101, 0b00000001, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {10, 0b00101011, 0b01000100, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {11, 0b00000110, 0b00000001, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {12, 0b01000000, 0b00000101, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {13, 0b00000001, 0b00000001, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {14, 0b11111111, 0b11111111, 0, "Registers 14-17: Individual Amplifier Power", ""},
    {15, 0b11111111, 0b11111111, 0, "Registers 14-17: Individual Amplifier Power", ""},
    {16, 0b00000000, 0b00000000, 0, "Registers 14-17: Individual Amplifier Power", ""},
    {17, 0b00000000, 0b00000000, 0, "Registers 14-17: Individual Amplifier Power", ""},
    
    {40, DONT_CARE, DONT_CARE, 1, "Company Designation", ""},
    {41, DONT_CARE, DONT_CARE, 1, "Company Designation", ""},
    {42, DONT_CARE, DONT_CARE, 1, "Company Designation", ""},
    {43, DONT_CARE, DONT_CARE, 1, "Company Designation", ""},
    {44, DONT_CARE, DONT_CARE, 1, "Company Designation", ""},
    {60, DONT_CARE, DONT_CARE, 1, "Die Revision", ""},
    {61, DONT_CARE, DONT_CARE, 1, "Unipolar/Bipolar Amplifiers", ""},
    {62, DONT_CARE, DONT_CARE, 1, "Number of Amplifiers", ""},
    {63, DONT_CARE, DONT_CARE, 1, "Intan Technologies Chip ID", ""},
};

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
	printf("R: reg#%d\n", reg_num);
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
	
	*result = rx_buf[1];
	printf("Got data %x\n\n", *result);
	
	return ret;
}

int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data) {
	printf("W: reg#%d, data %x\n", reg_num, reg_data);
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
	return ret;
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
				char error_str[256];
				sprintf(error_str, "spi xfer failed during convert ch %d", ch);
				pabort(error_str);
			}

			if (counter >= 0) {
				data_buf[counter] = (rx_buf[1] << 8) | rx_buf[0];
			}

			++counter;
		}
	}

	return 0;
}

// initializes RHD registers to default
int rhd_reg_config_default(int fd) {
	// size_t reg_list_len = sizeof(rhd2216_reg_list) / sizeof(rhd2216_reg_list[0]);
	// if (DEBUG) {
	// 	printf("PVDEBUG: Got reg_list_len %zu", reg_list_len);
	// }
	
	// int i;
	// for (i=0; i<reg_list_len; ++i) {
	// 	rhd_reg_t cur_reg = rhd2216_reg_list[i];
	// 	if (cur_reg.read_only) {
	// 		continue;
	// 	}

	// 	uint8_t read_data;
	// 	rhd_reg_write(fd, cur_reg.reg_num, cur_reg.config_write_val);
	// 	rhd_reg_read(fd, cur_reg.reg_num, &read_data);
	// 	if (read_data != cur_reg.config_check_val) {
	// 		printf(
	// 			"WARNING: expected reg %d to read %x after writing %x, but got %x instead\n",
	// 			cur_reg.reg_num,
	// 			cur_reg.config_check_val,
	// 			cur_reg.config_write_val,
	// 			read_data);
	// 	}
	// }

	uint8_t read_data;
	rhd_reg_read(fd, 63, &read_data);
	rhd_reg_read(fd, 63, &read_data);
	rhd_reg_write(fd, 0, 0xde);

	return 0;
}