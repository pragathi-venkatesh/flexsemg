#include "rhd2216_lib.h"

static int verbose = 0; // if 1, logs ALL spi read/writes/
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

static int get_max_srate_and_delay(int fd, uint16_t srate, uint16_t active_chs_msk, uint16_t* max_srate, uint16_t* delay_us) {
	// measures duration of convert for all chs in active_chs_msk
	// then uses that duration to calculate appropriate delay needed 
	// to reach desired srate
	// returns -1 if desired srate is not possible (too fast)

	int ch_num = 0;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};
	uint8_t command_word = 0;
	double desired_period_sec = 1.0 / srate;
	double time_spent_sec;
	clock_t begin;
	clock_t end;

	begin = clock();

	command_word = 0;
	command_word |= ch_num & 0x3f;
	tx_buf[1] = dsp_offset_rem_en & 0b1;
	// read from same channel 16 times just to get duration
	rhd_spi_xfer(fd, tx_buf, N, rx_buf);

	printf(
		"PVDEBUG: Desired srate: %.3e Hz, desired sampling period is %.3e s.\n",
		(double) srate,
		desired_period_sec
		);
	end = clock();

	time_spent_sec = (double)(end - begin) / CLOCKS_PER_SEC;
	*max_srate = (uint16_t) (1.0 / time_spent_sec);

	if (time_spent_sec > desired_period_sec) {
		*delay_us = 0;
		printf(
			"PVDEBUG: Desired srate %.3e Hz too fast with current SPI properties. Each read from active_chs_mask %04x takes %.2e s, so max srate is %.3e Hz\n",
			(double) srate,
			active_chs_msk,
			time_spent_sec,
			(double) *max_srate
			);
		return -1;
	}

	*delay_us = (uint16_t) ((desired_period_sec - time_spent_sec) * 1000000);
	printf(
		"PVDEBUG: reading from active_chs_msk %04x took %.2e s\n. Desired period is %.2e us. Resulting delay_us applied by controller is %d us\n", 
		active_chs_msk,
		time_spent_sec * 1000000,
		desired_period_sec * 1000000,
		*delay_us
	);
	return 0;	
}

int get_dsp_offset_rem_en(void) {
	return dsp_offset_rem_en;
}

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
	if (verbose) {
		printf("Got data %x\n\n", *result);
	}
	
	return ret;
}

int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data) {
	if (verbose) {
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

	size_t counter = 0;
	int ret;
	size_t N = 2;
	uint8_t tx_buf[] = {0, 0};
	uint8_t rx_buf[] = {0xde, 0xad};
	uint8_t command_word = 0;
	uint16_t max_srate;
	uint16_t delay_amt_us;

	printf("PVDEBUG: start rhd_convert.");
	ret = get_max_srate_and_delay(fd, srate, active_chs_msk, &max_srate, &delay_amt_us);

	if (ret == -1) {
		printf(
			"WARNING: desired srate %.2e Hz greater than max possible srate %.2e Hz. Using max srate.\n",
			(double) srate,
			(double) max_srate
		);
		srate = max_srate;
	}

	// due to pipelining, first two rx results are garbage values.
	// function will only start writing to data buf after first two spi transfers
	printf("PVDEBUG: dsp rem en = %d\n", dsp_offset_rem_en);
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

	printf("PVDEBUG: end rhd_convert.");
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
	// idk why but michael does 20 extra reads in his 2022 nrf code.
	// seems overkill but why not. -PV 2024-May-18
	size_t reg_list_len = sizeof(rhd2216_reg_list) / sizeof(rhd2216_reg_list[0]);
	int ret;
	uint8_t tx_buf[] = {0b11111111, 0};
	uint8_t rx_buf[] = {0,0};

	printf("PVDEBUG: Got reg_list_len %zu\n", reg_list_len);
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
		ret = rhd_reg_write(fd, cur_reg.reg_num, cur_reg.config_write_val);
		ret = rhd_reg_read(fd, cur_reg.reg_num, &read_data);
		if (read_data != cur_reg.config_check_val) {
			printf(
				"WARNING: reg_config_default: expected reg %d to read %x after writing %x, but got %x instead\n",
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

	printf("PVDEBUG: end register config sequence, ret %d.", ret);
	return 0;
}

int rhd_calibrate(int fd) {
	int ret;
	int N = 16;
	uint8_t tx_buf[] = {0,0};
	uint8_t rx_buf[] = {0xde, 0xad};
	uint16_t databuf[N];

	tx_buf[0] = 0b01010101;

	printf("PVDEBUG: start calibration");
	ret = rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	if (ret == -1) {
		pabort("ERROR: could not send calibration command\n");
	}

	// 9 dummy reads needed following calibrate
	// we are excessive so we do more.
	tx_buf[0] = 255;
    tx_buf[1] = 0;
	for( int i = 0; i < 50; i++){
		ret = rhd_spi_xfer(fd, tx_buf, 2, rx_buf);
	}

	// do DSP offset removal on all channels
	set_dsp_offset_rem_en(1);
	// read from all 16 channels, doesn't matter if they are active or not.
	rhd_convert(fd, 0xffff, 1000, databuf, N);
	set_dsp_offset_rem_en(0);
	printf("PVDEBUG: end calibration, ret %d", ret);
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

	printf("PVDEBUG: clear calibration command sent, ret %d", ret);
	return ret;
}