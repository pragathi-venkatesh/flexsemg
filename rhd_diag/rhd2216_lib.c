#include "rhd2216_lib.h"

static int dsp_offset_rem_en = 0; // active high

// register defaults
const rhd_reg_t rhd2216_reg_list[] = {
    // reg_num, default val, read_only, short_desc, loooooooooong_desc (TODO)
    {0, 0b11011110, 0b11011110, 0, "ADC Configuration and Amplifier Fast Settle", ""},
    {1, 0b00100000, 0b00100000, 0, "Supply Sensor and ADC Buffer Bias Current", ""},
    {2, 0b00101000, 0b00101000, 0, "MUX Bias Current", ""},
    {3, 0b00000000, 0b00000000, 0, "MUX Load, Temperature Sensor, and Auxiliary Digital Output", ""},
    {4, 0b11010110, 0b11010110, 0, "ADC Output Format and DSP Offset Removal", ""},
    {5, 0b00000000, 0b00000000, 0, "Impedance Check Control", ""},
    {6, 0b00000000, 0b00000000, 0, "Impedance Check DAC", ""},
    {7, 0b00000000, 0b00000000, 0, "Impedance Check Amplifier Select", ""},
    {8, 0b00011110, 0b00011110, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {9, 0b00000101, 0b00000101, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {10, 0b00101011, 0b00101011, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {11, 0b00000110, 0b00000110, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
    {12, 0b00101000, 0b00101000, 0, "Registers 8-13: On-Chip Amplifier Bandwidth Select", ""},
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

// TODO is this supposed to do anything? like write to a register?
int set_dsp_offset_rem_en(int en) {
	dsp_offset_rem_en = en;
	return 0;
}

int rhd_reg_read(int fd, uint8_t reg_num, uint8_t *result) {
	if (DEBUG) {
		printf("R: reg#%d\n", reg_num);
	}

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
	if (DEBUG) {
		printf("Got data %x\n\n", *result);
	}
	
	return ret;
}

int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data) {
	if (DEBUG) {
		printf("W: reg#%d, data %x\n", reg_num, reg_data);
	}
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
int rhd_convert(int fd, uint16_t active_chs_msk, uint16_t srate, uint16_t *data_buf, size_t buf_len) {
	if (active_chs_msk == 0) {
		pabort("rhd_convert: argument active_chs_mask must be non-zero.");
	}

	if (srate > RHD_MAX_SRATE) {
		printf(
			"WARNING: desired srate %d Hz greater than max srate %d Hz. Defaulting to max srate.",
			srate,
			RHD_MAX_SRATE
		);
		srate = RHD_MAX_SRATE;
	}

	printf("PVDEBUG: dsp rem en = %d\n", dsp_offset_rem_en);
	size_t counter = 0;
	int ret;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};
	uint8_t command_word = 0;
	uint16_t delay_amt_us = 1000000 / srate;

	// due to pipelining, first two rx results are garbage values.
	// function will only start writing to data buf after first two spi transfers
	while (counter < (buf_len+2)) {
		// assume convert occurs instantly. 
		// enforce sample rate by delaying by 1/srate seconds
		delay_us(delay_amt_us);
		for (int ch=0; ch<16; ++ch) {
			if (counter >= (buf_len+2)) {
				break;
			}
			// skip if channel not part of active_ch_msk
			if ( !((0b1 << ch) & active_chs_msk) )
				continue; // do not increment counter

			command_word = 0;
			command_word |= ch & 0x3f;
			tx_buf[0] = command_word;
			tx_buf[1] = dsp_offset_rem_en & 0b1;
			ret = rhd_spi_xfer(fd, tx_buf, N, rx_buf);
			if (ret == -1) {
				printf("spi xfer failed during convert ch %d", ch);
			}

			if (counter >= 2) {
				data_buf[counter - 2] = (rx_buf[0] << 8) | rx_buf[1];
			}

			// printf("PVDEBUG: counter: %zu\n", counter);
			++counter;
			
			// TODO set DSP offset flag depending on sample rate and integral of past values
		}
	}

	return 0;
}

// // read until we get expected value or hit max_num_reads
// static void check_read(int fd, uint8_t reg_num, uint8_t check_val, uint8_t *read_val) {
// 	size_t count = 0;
// 	size_t max_num_reads = 100;

// 	rhd_reg_read(fd, reg_num, read_val);
// 	while(*read_val != check_val) {
// 		if (count > max_num_reads) {
// 			break;
// 		}
// 		rhd_reg_read(fd, reg_num, read_val);
// 		count++;
// 	}
// }

// initializes RHD registers to default
int rhd_reg_config_default(int fd, uint16_t active_chs_mask) {
	size_t reg_list_len = sizeof(rhd2216_reg_list) / sizeof(rhd2216_reg_list[0]);
	if (DEBUG) {
		printf("PVDEBUG: Got reg_list_len %zu\n", reg_list_len);
	}

	// idk why but michael does 20 extra reads in his 2022 nrf code.
	// seems overkill but why not. -PV 2024-May-18
	uint8_t tx_buf[] = {0b11111111, 0};
	uint8_t rx_buf[] = {0,0};
	for (int i=0; i<20; ++i) {
		rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	}
	printf("PVDEBUG: begin register config w 20 reads, got %d\n", rx_buf[1]);

	int i;
	for (i=0; i<reg_list_len; ++i) {
		rhd_reg_t cur_reg = rhd2216_reg_list[i];
		if (cur_reg.read_only) {
			continue;
		}

		uint8_t read_data;
		rhd_reg_write(fd, cur_reg.reg_num, cur_reg.config_write_val);
		rhd_reg_read(fd, cur_reg.reg_num, &read_data);
		if (read_data != cur_reg.config_check_val) {
			printf(
				"WARNING: expected reg %d to read %x after writing %x, but got %x instead\n",
				cur_reg.reg_num,
				cur_reg.config_check_val,
				cur_reg.config_write_val,
				read_data
				);
		}
	}

	// power on active channels based on active_chs_mask
	rhd_reg_write(fd, 14, (uint8_t) (active_chs_mask & 0xff));
	rhd_reg_write(fd, 15, (uint8_t) (active_chs_mask >> 8) && 0xff);

	return 0;
}

int rhd_calibrate(int fd) {
	int ret;
	uint8_t tx_buf[] = {0,0};
	uint8_t rx_buf[] = {0xde, 0xad};
	tx_buf[0] = 0b01010101;
	ret = rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	if (ret == -1) {
		pabort("ERROR:could not send calibration command\n");
	}

	// 9 dummy reads needed following calibrate
	// we are excessive so we do more.
	tx_buf[0] = 255;
    tx_buf[1] = 0;
	for( int i = 0; i < 50; i++){
		ret = rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	}

	// do DSP offset removal on all channels
	uint16_t databuf[16];
	set_dsp_offset_rem_en(1);
	// read from all 16 channels, doesn't matter if they are active or not.
	rhd_convert(fd, 0xffff, 1000, databuf, 16);
	set_dsp_offset_rem_en(0);
	return ret;
}

int rhd_clear_calibration(int fd) {
	int ret;
	uint8_t tx_buf[] = {0,0};
	uint8_t rx_buf[] = {0xde, 0xad};
	tx_buf[0] = 0b1101010;
	ret = rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	if (ret == -1) {
		pabort("ERROR: could not send clear calibration command\n");
	}

	return ret;
}