#include "rhd2216_lib.h"

props.device = device;
props.mode = 0;
props.bpw = 8;
props.speed = 1000000;
props.delay = 0;

static void pabort(const char *s) {
	perror(s);
	abort();
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D --device   	device to use (default /dev/spidev0.1)\n"
	     "  -s --speed    	max speed (Hz)\n"
	     "  -d --delay    	delay (usec)\n"
		 "  -n --reg_num  	RHD register number to read or write\n"
		 "  -r --read_reg 	Read register specified by --reg_num\n"
		 "  -w --write_reg  Write register specified by --reg_num\n"
		 "  -c --convert    ADC channel to read. Valid channels 1-16\n"
		 );
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  	1, 0, 'D' },
			{ "speed",   	1, 0, 's' },
			{ "delay",   	1, 0, 'd' },
			{ "mode",		1, 0, 'M'},
			// todo add RHD-specific options, such as register#, R, W, and data
			{ "reg_num", 	1, 0, 'n'},
			{ "read_reg", 	0, 0, 'r'},
			{ "write_reg", 	1, 0, 'w'},
			{ "convert", 	1, 0, 'c'},
			{ NULL, 		0, 0, 0 },
		};

		int c;
		c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NRnrw", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			props.device = optarg;
			break;
		case 's':
			props.speed = atoi(optarg);
			break;
		case 'd':
			props.delay = atoi(optarg);
			break;
		}
	}
}

int main(int argc, char *argv[]) {
	// static variables accessible only used to this script
	char *device = PI_SPI_0_0;
	// initialize with default values

	int ret = 0;
	int fd = open(device, O_RDWR);

	parse_opts(argc, argv);

	if (fd < 0)
		pabort("can't open device");

	ret = pi_spi_config(fd, &props);
	if (ret == -1) 
		pabort("could not configure pi spi properties");

	uint8_t reg_data;
	uint8_t reg_num = 62;
	ret = rhd_reg_read(fd, 62, &reg_data);
	printf("read: regnum: %d, result: 0x%02x", reg_num, reg_data);

	close(fd);
    return 0;
}


