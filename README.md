# gb-adc-test

Gertboard ADC/DAC loopback test on RPi 2B

## Getting Started

This ADC loopback test utilies the SPI ports on RPi 2B communicates with the ADC/DAC chips on the RPi Gertboard I/O board.

### Test Equipment Setup

* RPi: Use SPI0 in GPIO_ALT0 mode (via S/W)
* Gertboard:
  - ADC: Use MCP3002 Channel 0, connect to RPi SPI0_CS_1
  - DAC: Use MCP4802 Channel 1, connect to RPi SPI0_CS_0
  - Jumpers: loopback above both ADC/DAC I/O pins
  - Jumpers: close GP7~GP11 to all corespondent SPI0 pins

## Running the tests

Test procedure will loop through each test sending out the entered voltage in 1~255 scale from ADC loopbacks to DAC and display with measured voltage as result.

## Built With AMD64 Ubuntu 18.04 LTS GNU Tool Chain for ARMv7

* [BCM2836 ARMv7 Tool Chain Configuration]
  - CC: arm-linux-gnueabihf-gcc
  - CXX: arm-linux-gnueabihf-g++
  - CFLAGS: -Werror -Wno-deprecated -O2 -g -DDEBUG
  - INCLUDES: -I/home/fred/Workspaces/Collections/rpi2/common/util-c/apps-logger
  - LIBS: <none>

## Authors

* **Frederic Chen** - *Test Succeed* 

## License

* None

