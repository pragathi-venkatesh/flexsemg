#include "rhd2216_lib.h"


// rx buf must have same size as tx buf
// result stored in rx_buf
int transfer(int fd, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf, PiSPIProps *props) {
    int ret;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = ARRAY_SIZE(tx_buf),
        .delay_usecs = props->delay,
        .speed_hz = props->speed,
        .bits_per_word = props->bpw,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    return ret;
}
