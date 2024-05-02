# rpi headless setup
* installed recommended OS (64-bit Raspbian). supplied wifi network name and pw so pi will connect to wifi on boot.
* set custom username/hostname.
```bash
$ uname -a
Linux pragpiv4b 6.6.20+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.6.20-1+rpt1 (2024-03-07) aarch64 GNU/Linux
```

* logged into pi via ssh (enabled by default)
* installed vnc using command ` sudo apt-get install realvnc-vnc-server`
* downloaded real vnc viewer on my own pc
* used pi headless through vnc after this.
* go to "Raspberry Pi Configuration" and enable all interfaces
![image](./rpi/raspi_config_interfaces.png)

# rpi SPI control
[https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#software](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#serial-peripheral-interface-spi)

1. check SPI 0 and 1 is enabled @ boot by checking `/boot/firmware/config.txt`

    ```
    # NOTE: check for this line
    dtparam=spi=on
    # NOTE: add this line if it doesn't exist
    dtoverlay=spi1-1cs # enable auxiliary SPI with 1 chip select
    ```
1. give full control of spi files - run following commands:
    ```
    sudo chmod 777 /dev/spidev*
    ls -al /dev | grep spi
    ```
    output should be
    ```
    crwxrwxrwx   1 root spi     153,   0 Mar 25 00:17 spidev0.0
    crwxrwxrwx   1 root spi     153,   1 Mar 25 00:17 spidev0.1
    ```

# what rhd register configs actually means:
## default configuration (do this during initialization)
NOTE: LPM: "low power mode"
* reg 0: 
  * set 0b11011110
  * enable internal ADC bandwidth, disable amp fast settle, enable Vref enable, enable comparator bias current.
* reg 1: 
  * set 0b00100000
  * enable on chip voltage sensor, set ADC buffer bias. optimum value depends on sample rate, check datasheet.
* reg 2:
  * set 0b00101000
  * configure bias current, optimum value depends on sample rate, check datasheet
* reg 3:
  * set 0
  * disable on-die temperature sensors
  * disable off-chip circuitry like switches and LEDs and put into HiZ mode
* reg 4:
  * 

## register fields for low power mode:
* reg 0: 
  * amp Vref enable[4]: set 0 
  * ADC comparator bias[3:2]: set 0
* reg 1:
  * VDD sense enable[6]: set 0