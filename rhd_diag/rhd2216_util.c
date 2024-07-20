/*
2024-April-07 Pragathi Venkatesh (PV)
Modified from https://github.com/rm-hull/spidev-test/blob/master/spidev_test.c
for use with RHD2216

Usage:
Run from rhd_diag directory
* just read register #4
	./build/rhd2216_util --reg_num 4 
	or
	./build/rhd2216_util --reg_num 4 --reg_read
* write 0xd6 to register #4
	./build/rhd2216_util --reg_num 4 --reg_write 0xd6
* configure registers to default configuration, calibrate, then read chs 1-6 until we read 1000 samples
	./build/rhd2216_util --config --calibrate --convert 1000
* configure registers to default configuration, calibrate, then read from all odd chs until we read 1000 samples
	./build/rhd2216_util --config --calibrate --convert 1000 --active_chs 0x5555
	
Help:
* following prints out complete usage
	./build/rhd2216_util
*/

#include <time.h> // for timestamps
#include "pi_spi_lib.h"
#include "rhd2216_lib.h"

static uint8_t reg_data;
static uint8_t reg_num;
static uint8_t mode;
static uint8_t bpw;
static uint32_t speed;
static char *device = PI_SPI_0_0;
static size_t num_samples = 16;
static uint16_t srate = 1000; 
static uint16_t active_chs_mask = 0xffff;

static int FOUND_REG_NUM = 0;
static int FOUND_REG_READ = 0;
static int FOUND_REG_WRITE = 0;
static int FOUND_ACTIVE_CHS = 0;
static int FOUND_CONVERT = 0;
static int FOUND_CONFIG = 0;
static int FOUND_CALIBRATE = 0;
static int FOUND_CLEAR = 0;
static int FOUND_SRATE = 0;

static void pabort(const char *s) {
	perror(s);
	abort();
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-Dsdnrwc]\n", prog);
	printf("  -D --device			device to use (default /dev/spidev0.1).\n"
	     "  -s --speed			max speed (Hz), Default 12.5 MHz. \n"
	     "  -d --delay			delay (usec).\n"
		 "  -n --reg_num		\tRHD register number to read or write.\n"
		 "  -r --reg_read		\tRead register specified by --reg_num.\n"
		 "  -w --reg_write		Write data (in hex) register specified by --reg_num.\n"
		 "  -a --active_chs		16-bitmask (in hex) defining which channels active. e.g. 0x8001 enables ch 16 and ch 1 only.\n"
		 "  -c --convert		\tContinuously reads chs 1-16 until N samples reached. Default 16 samples. Always starts at ch 1.\n"
		 "  -g --config			Configures RHD registers to default values.\n"
		 "  -C --calibrate		Initiate ADC self-calibration routine.\n"
		 "  -e --clear			Clear Calibration.\n"
		 "  -R --srate			Sampling rate in Hz, used with --convert command.\n"
		 );
	printf(
		"Example:\n./build/rhd2216_util --config --calibrate --convert 1000 --active_chs 0x5555 --srate 1250\n"
		);
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	// TODO single character arguments not working (like -n, -e), 
	// results in segfault
	while (1) {
		static const struct option lopts[] = {
			// name, has_arg, int *flag, int *val
			{ "device",  	1, 0, 'D' },
			{ "speed",   	1, 0, 's' },
			{ "delay",   	1, 0, 'd' },
			{ "mode",		1, 0, 'M'},
			{ "reg_num", 	1, 0, 'n'},
			{ "reg_read", 	0, 0, 'r'},
			{ "reg_write", 	1, 0, 'w'},
			{ "active_chs", 1, 0, 'a'},
			{ "convert", 	1, 0, 'c'},
			{ "config", 	0, 0, 'g'},
			{ "calibrate",  0, 0, 'l'},
			{ "clear",		0, 0, 'e'},
			{ "srate", 		1, 0, 'R'},
			{ NULL, 		0, 0, 0 },
		};

		int c;
		c = getopt_long(argc, argv, "D:s:d:M:n:rw:c:g:l:e", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			// TODO
			break;
		case 's':
			// TODO
			break;
		case 'd':
			// TODO
			break;
		case 'n':
			FOUND_REG_NUM = 1;
			reg_num = strtol(optarg, NULL, 10);
			printf("PVDEBUG: found regnum %d\n", reg_num);
			break;
		case 'r':
			FOUND_REG_READ = 1;
			printf("PVDEBUG: found reg_read\n");
			break;
		case 'w':
			FOUND_REG_WRITE = 1;
			reg_data = strtoul(optarg, NULL, 16);
			printf("PVDEBUG: found write w reg_data: %x\n", reg_data);
			break;
		case 'a':
			FOUND_ACTIVE_CHS = 1;
			active_chs_mask = strtol(optarg, NULL, 16);
			printf("PVDEBUG: found active_chs %04x\n", active_chs_mask);
			break;
		case 'c':
			FOUND_CONVERT = 1;
			num_samples = strtol(optarg, NULL, 10);
			printf("PVDEBUG: found_convert, num_samples %zu\n", num_samples);
			break;
		case 'g':
			FOUND_CONFIG = 1;
		 	printf("PVDEBUG: found config\n");
			break;
		case 'l':
			FOUND_CALIBRATE = 1;
			printf("PVDEBUG: found calibrate\n");
			break;
		case 'e':
			FOUND_CLEAR = 1;
			printf("PVDEBUG: found clear\n");
			break;
		case 'R':
			FOUND_SRATE = 1;
			srate = strtol(optarg, NULL, 10);
			printf("PVDEBUG found srate %d\n", srate);
			break;
		}
	}

	// process arguments
	if ( !FOUND_REG_NUM && (FOUND_REG_READ || FOUND_REG_WRITE) ) {
		pabort("ERROR: If reg_write/reg_read specified, need to provide reg_num");
	}

	if ( !FOUND_ACTIVE_CHS ) {
		printf(
			"WARNING: Default active_chs_mask set to 0xffff. Run with --active_chs arg for custom active_chs_mask.\n"
			);
	}

	if ( FOUND_CONVERT && !FOUND_SRATE ) {
		printf("WARNING: --convert specified but --srate not specified. Using default srate of 1000 Hz.\n");
	}

	// default read reg
	if ( FOUND_REG_NUM && !(FOUND_REG_READ || FOUND_REG_WRITE) ) {
		FOUND_REG_READ = 1;
	}

}

static void get_fname(char *fname, size_t max_len) {
	time_t t;
    struct tm *tmp;
    char time_str[14];
	
	time(&t);
	tmp = localtime(&t);
	strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", tmp);

	sprintf(fname, "./dlogs/rhd2216_util_convert_%s_N%ld.txt", time_str, num_samples);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		print_usage(argv[0]);
	}
	
	
	// initialize with default values

	int ret = 0;
	int fd = open(device, O_RDWR);

	parse_opts(argc, argv);

	if (fd < 0)
		pabort("can't open device");

	mode = 0;
	bpw = 8;
	speed = 8000000; //12500000; // 12.5 mHz
	ret = spi_config(fd, mode, bpw, speed);
	if (ret == -1) 
		pabort("Could not configure pi spi properties");
	
	if (FOUND_REG_READ) {
		uint8_t read_data;
		ret = rhd_reg_read(fd, reg_num, &read_data);
		printf("read: reg_num: %d, result: 0x%02x\n", reg_num, read_data);
	}

	if (FOUND_REG_WRITE) {
		// TODO
		printf("write: reg_num: %d, write_data: 0x%02x\n", reg_num, reg_data);
		ret = rhd_reg_write(fd, reg_num, reg_data);
	}

	if (FOUND_CONFIG) {
		ret = rhd_reg_config_default(fd, active_chs_mask);
		printf("Default register configuration done, ret code: %d\n", ret);
	}

	if (FOUND_CALIBRATE) {
		ret = rhd_calibrate(fd);
		// calibrate command always returns 2? not sure what this means
		// but I analyzed the signals in a logic analyzer and they looked
		// fine. -PV 2024-May-12
		printf("Calibration done, ret code: %d\n", ret);
	}

	if (FOUND_CONVERT) {
		FILE *output;
		char fname[255];
		int i;

		get_fname(fname, sizeof(fname));
		output = fopen(fname, "w");
		// uint16_t data_buf[num_samples];
		uint16_t *data_buf = (uint16_t*) malloc(num_samples * sizeof(uint16_t));
		ret = rhd_convert(fd, active_chs_mask, srate, data_buf, num_samples);

		// write to file
		printf("Writing to file...\n");
		fprintf(output, "active_chs_mask: %x\n", active_chs_mask);
		fprintf(output, "num_samples: %zu\n", num_samples);
		fprintf(output, "sample rate: %d Hz\n", srate);
		// TODO this can be optimized.
		for (i=0; i<num_samples; ++i) {
			fprintf(output, "%2x\n", data_buf[i]);
		} 
		printf("Done.\n");

		printf("Printing first 16 values:\n");
		for (i=0; i<num_samples && i<16; ++i) {
			printf("0x%x\n", data_buf[i]);
		}

		printf("Full data stored in %s\n", fname);
		free(data_buf);
		fclose(output);
	}

	if (FOUND_CLEAR) {
		ret = rhd_clear_calibration(fd);
		if (ret == 0) {
			printf("Clear calibration done, ret code: %d\n", ret);
		} else {
			printf("Clear calibration failed, err code: %d\n", ret);
		}
	}

	close(fd);
    return 0;
}


