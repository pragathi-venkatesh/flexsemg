/*
RHD2000 series chips datasheet: 
https://intantech.com/files/Intan_RHD2000_series_datasheet.pdf

Linux SPI API (spidev) only supports half duplex. This means two way comm is
possible, but not at the same time. writes and reads must occur sequentially.
    for more info: https://www.kernel.org/doc/Documentation/spi/spidev

https://www.raspberrypi.com/documentation/computers/raspberry-pi.html
using SPI0: 
* MOSI: GPIO10
* MISO: GPIO19
* SCLK: GPIO11
* CE0: GPIO8
* CE1: GPIO7 (not used)
* GND: any GND pin.
*/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "pi_spi_lib.h"

#define PI_SPI_0_0 "/dev/spidev0.0"
#define PI_SPI_0_1 "/dev/spidev0.1"
#define PI_SPI_1_0 "/dev/spidev1.0"
#define PI_SPI_1_1 "/dev/spidev1.1"

#ifndef DEBUG
#define DEBUG 1 // if nonzero, prints debug statements
#endif

int rhd_reg_read(int fd, uint8_t reg_num, uint8_t *result);
int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data);