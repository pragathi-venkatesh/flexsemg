/*
RHD utility functions. 
* If using pi controller, use "pi_spi_lib.h".
* If using NRF-52DK board as controller, uhhhh (TODO create nrfdk_spi_lib.h)
RHD2000 series chips datasheet: 
https://intantech.com/files/Intan_RHD2000_series_datasheet.pdf

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

#ifndef DEBUG
#define DEBUG 1 // if nonzero, prints debug statements
#endif

#ifndef DONT_CARE
#define DONT_CARE 0b00000000
#endif

typedef struct rhd_reg {
    uint8_t reg_num;
    uint8_t config_write_val;
    uint8_t config_check_val;
    int read_only;
    const char* short_desc;
    const char* long_desc;
} rhd_reg_t;

// get/set
int get_dsp_offset_rem_en(void);
int set_dsp_offset_rem_en(int en);

// util functions
int rhd_reg_read(int fd, uint8_t reg_num, uint8_t *result);
int rhd_reg_write(int fd, uint8_t reg_num, uint8_t reg_data);
int rhd_convert(int fd, uint16_t active_chs_msk, uint16_t *data_buf, size_t data_buf_len);
int rhd_reg_config_default(int fd);