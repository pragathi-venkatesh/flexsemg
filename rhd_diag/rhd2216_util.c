/*
2024-April-07 Pragathi Venkatesh (PV)
Modified from https://github.com/rm-hull/spidev-test/blob/master/spidev_test.c
for use with RHD2216

Usage:
* just read register #4
	./rhd2216_util --reg_num 4 
	or
	./rhd2216_util --reg_num 4 --reg_read
* write 0xd6 to register #4
	./rhd2216_util --reg_num 4 --reg_write 0xd6
* configure registers to default configuration, calibrate, then 
* following prints out complete usage
	./rhd2216_util
*/

#include "pi_spi_lib.h"
#include "rhd2216_lib.h"

static uint8_t reg_data;
static uint8_t reg_num;
static uint8_t mode;
static uint8_t bpw;
static uint32_t speed;
// static uint8_t ch_num;
static int FOUND_REG_NUM = 0;
static int FOUND_REG_READ = 0;
static int FOUND_REG_WRITE = 0;
static int FOUND_CONVERT = 0;
static int FOUND_CONFIG = 0;
static int FOUND_CALIBRATE = 0;
static int FOUND_CLEAR = 0;

static void pabort(const char *s) {
	perror(s);
	abort();
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-Dsdnrwc]\n", prog);
	puts("  -D --device		device to use (default /dev/spidev0.1)\n"
	     "  -s --speed		max speed (Hz)\n"
	     "  -d --delay		delay (usec)\n"
		 "  -n --reg_num	\tRHD register number to read or write\n"
		 "  -r --reg_read	\tRead register specified by --reg_num\n"
		 "  -w --reg_write	Write data (in hex) register specified by --reg_num\n"
		 "  -c --convert	\tReads all 16 channels at once\n"
		 "  -g --config		Configures RHD registers to default values\n"
		 "  -C --calibrate	Initiate ADC self-calibration routine\n"
		 "  -e --clear		Clear Calibration.\n"
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
			{ "convert", 	0, 0, 'c'},
			{ "config", 	0, 0, 'g'},
			{ "calibrate",  0, 0, 'C'},
			{ "clear",		0, 0, 'e'},
			{ NULL, 		0, 0, 0 },
		};

		int c;
		c = getopt_long(argc, argv, "D:s:d:DsdMnrwcgCe", lopts, NULL);

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
			reg_num = atoi(optarg);
			break;
		case 'r':
			FOUND_REG_READ = 1;
			break;
		case 'w':
			FOUND_REG_WRITE = 1;
			reg_data = strtoul(optarg, NULL, 16);
			printf("PVDEBUG: found reg_data: %x\n", reg_data);
			break;
		case 'c':
			FOUND_CONVERT = 1;
			break;
		case 'g':
			FOUND_CONFIG = 1;
			break;
		case 'C':
			FOUND_CALIBRATE = 1;
			break;
		case 'e':
			FOUND_CLEAR = 1;
			break;
		}
	}

	// process arguments
	if ( !FOUND_REG_NUM && (FOUND_REG_READ || FOUND_REG_WRITE) ) {
		printf("If reg_write/reg_read specified, need to provide reg_num");
	}

	// default read reg
	if ( FOUND_REG_NUM && !(FOUND_REG_READ || FOUND_REG_WRITE) ) {
		FOUND_REG_READ = 1;
	}

	printf("PVDEBUG: found_reg_read: %d, found_reg_write: %d\n", 
		FOUND_REG_READ, FOUND_REG_WRITE);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		print_usage(argv[0]);
	}
	
	// static variables accessible only used to this script
	char *device = PI_SPI_0_0;
	// initialize with default values

	int ret = 0;
	int fd = open(device, O_RDWR);

	parse_opts(argc, argv);

	if (fd < 0)
		pabort("can't open device");

	mode = 0;
	bpw = 8;
	speed = 1000000;
	ret = spi_config(fd, mode, bpw, speed);
	if (ret == -1) 
		pabort("could not configure pi spi properties");
	
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
		ret = rhd_reg_config_default(fd);
		printf("Default register configuration done.\n");
	}

	if (FOUND_CALIBRATE) {
		ret = rhd_calibrate(fd);
		if (ret == -1) {
			printf("Calibration done.\n");
		}
	}

	if (FOUND_CONVERT) {
		uint16_t data_buf[16];
		ret = rhd_convert(fd, 0xffff, data_buf, 16);

		printf("Reading all 16 channels:\n");
		int i;
		for (i=0; i<16; ++i) {
			printf("ch%d, 0x%x\n", i, data_buf[i]);
		}
	}

	if (FOUND_CLEAR) {
		ret = rhd_clear_calibration(fd);
		if (ret == 0) {
			printf("Clear calibration done.\n");
		}
	}

	close(fd);
    return 0;
}


