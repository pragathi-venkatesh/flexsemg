#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#ifndef DEBUG
#define DEBUG 1 // if nonzero, prints debug statements
#endif

int spi_config(int fd, uint8_t mode, uint8_t bpw, uint32_t speed);
int rhd_spi_xfer(int fd, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf); 