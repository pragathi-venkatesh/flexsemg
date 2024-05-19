#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h> // not for SPI, but for udelay function

#ifndef DEBUG
#define DEBUG 0 // if nonzero, prints debug statements
#endif

#define PI_SPI_0_0 "/dev/spidev0.0"
#define PI_SPI_0_1 "/dev/spidev0.1"
#define PI_SPI_1_0 "/dev/spidev1.0"
#define PI_SPI_1_1 "/dev/spidev1.1"

int spi_config(int fd, uint8_t mode, uint8_t bpw, uint32_t speed);
int rhd_spi_xfer(int fd, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf); 
void delay_us(unsigned long us);