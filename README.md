This repository is for debugging the RHD22xx module with a pi or any other controllers. Follow below instructions for 

TBD: support for controllers other than pi

# rpi headless setup
* using Raspberry Pi v4 B https://www.raspberrypi.com/products/raspberry-pi-4-model-b/
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
  * set `0b11011110`
  * enable internal ADC bandwidth, disable amp fast settle, enable Vref enable, enable comparator bias current.
* reg 1: 
  * set `0b00100000`
  * enable on chip voltage sensor, set ADC buffer bias. optimum value depends on sample rate, check datasheet.
* reg 2:
  * set `0b00101000`
  * configure bias current, optimum value depends on sample rate, check datasheet
* reg 3:
  * set `0`
  * disable on-die temperature sensors
  * disable off-chip circuitry like switches and LEDs and put into HiZ mode
* reg 4:
  * set `0b11010110`
  * set weak MISO (MISO goes HiZ when CS pulled high), enable twoscomp, disable absmode, enable DSPen (performs first order high-pass filter), set cutoff freq field to `0b0110` (= 0.002506 * srate)
    * note: if set to 0, acts as perfect differentiator
    * note: srate is sampling frequency of each channel and not overall ADC sampling frequency
* reg 5: 
  * set `0b00000000`
  * disable impedance check
* reg 6:
  * set `0b00000000`
  * set voltage for impedance check DAC (we don't do impedance checking so set to 0)
* reg 7:
  * set 0b0000`0000
  * selects electrodes to connect to impedance testing (none of them)
* reg 8-13 BW select:
  * note: programmable resistors RH1, RH2, and RL programmable, is another way to set BW. we dont use these off-chip registers through.
  * reg 8: set `0b00011110` --> don't use offchip resistor RH1. set RH1 DAC1 to 30
  * reg 9: set `0b00000101` --> disable aux1 ADC input (ch 32), set RH1 DAC2 to 5  
  * reg 10: set `0b00101011` --> don't use offchip reg RH2, set RH2 DAC1 to 43
  * reg 11: set `0b00000110` --> disable ADC aux2, set RH2 DAC2 to 6
  * reg 12: set `0b00101000` --> disable offchip RL, set RL DAC1 to 40
  * reg 13: set `0b00000001` --> disable ADC aux3, set RL DAC3 to 0. set RL DAC2 to 1
    * --> see pg 25, sets upper BW to 500 Hz.
    * --> see pg 26, sets lower BW to 5 Hz.
* reg 14-17:
  * enable amplifiers. 
  * NOTE: For testing, I only enable ch 0, so I only set reg 14 to 1. -PV 2024-May-08
## register fields for low power mode:
* reg 0: 
  * amp Vref enable[4]: set `0` 
  * ADC comparator bias[3:2]: set `0`
* reg 1:
  * VDD sense enable[6]: set `0`
* reg 14-17: set `0` to power down amplifiers, saving power if there are channels that don't need to be observed. current saved is 7.6 uA/kHz per amplifier. 
  * e.g. if powering down 16 channels that originally operate at 1 kHz, saves 0.1216 mA.
