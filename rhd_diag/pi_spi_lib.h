/* note:
 * make sure to init and config SPI file control device before doing any
 * SPI tranfers. 
 * e.g. 
 * spi_init("/dev/spidev0.0");
 * spi_config(...);
 * ...
 * spi_close();
 */
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

// pi SPI bus file control - each bus corresponds to different GPIO pins.
#define PI_SPI_0_0 "/dev/spidev0.0"
#define PI_SPI_0_1 "/dev/spidev0.1"
#define PI_SPI_1_0 "/dev/spidev1.0"
#define PI_SPI_1_1 "/dev/spidev1.1"

// pi-specific print function
// TODO #define RHD_PRINTF(f_, ...) printf((f_), __VA_ARGS__)

// spi + timing functions
int spi_init(char* device);
int spi_config(uint8_t mode, uint8_t bpw, uint32_t speed);
int rhd_spi_xfer(uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf); 
void delay_us(unsigned long us);
int spi_close();