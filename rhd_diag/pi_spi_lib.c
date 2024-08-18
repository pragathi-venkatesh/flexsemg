#include "pi_spi_lib.h"

static uint8_t _mode;
static uint8_t _bpw;
static uint32_t _speed;
static int fd = -1;

static void pabort(const char *s) {
	perror(s);
	abort();
}

int spi_init(char* device) {
	fd = open(device, O_RDWR);
	if (fd < 0) {
		printf("ERROR: cannot open device %s\n", device);
		return -1;
	}
	return 0;
}

int spi_config(uint8_t mode, uint8_t bpw, uint32_t speed) {
    int ret;

	_mode = mode;
	_bpw = bpw;
	_speed = speed;

    /*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &_mode);
	if (ret == -1)
		pabort("cannot set spi mode\n");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &_mode);
	if (ret == -1)
		pabort("cannot get spi mode\n");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &_bpw);
	if (ret == -1)
		pabort("cannot set bits per word\n");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &_bpw);
	if (ret == -1)
		pabort("cannot get bits per word\n");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &_speed);
	if (ret == -1)
		pabort("cannot set max speed hz\n");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &_speed);
	if (ret == -1)
		pabort("cannot get max speed hz\n");
    
    printf("resulting pi SPI config:\n");
	printf("spi mode: %d\n", _mode);
	printf("bits per word: %d\n", _bpw);
	printf("max speed: %d Hz (%d MHz)\n", 
			_speed, 
			_speed/1000000);
	printf("\n");

	return ret;
}

// rx buf must have same size as tx buf
// result stored in rx_buf
int rhd_spi_xfer(uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf) {
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = tx_len,
        // .delay_usecs = current_props->delay,
        // .speed_hz = current_props->speed,
        // .bits_per_word = current_props->bpw,
    };
	if (DEBUG) {
		printf("tx_buf: ");
		for (size_t i=0; i<tx_len; ++i) {
			printf("%02x, ", tx_buf[i]);
		}
		printf("\n");
	}
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (DEBUG) {
		printf("rx_buf: ");
		for (size_t i=0; i<tx_len; ++i) {
			printf("%02x, ", rx_buf[i]);
		}
		printf("\n\n");
	}
    return ret;
}

// close
int spi_close() {
	int ret;
	ret = close(fd);
	if (ret < 0) {
		printf("ERROR: cannot close spi file control.\n");
	}
	return ret;
}

// making a wrapper function because I don't want any dependent
// libraries to use wiringPi functions (because it's only for pi).
// -PV 2024-May-18
void delay_us(unsigned long us) {
	usleep(us);
}