#include "rhd2216_lib.h"

// static variables accessible only used to this script
static const char *device = "/dev/spidev0.0";
// initialize with default values
static const PiSPIProps props = {
	.device = device;
    .mode = 0; // pol = 0, phase = 0
    .bpw = 8;
    .speed = 500000;
    .delay = 0;
};

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
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		
		}
	}
}

int main() {
	int fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");
    return 0;
}


