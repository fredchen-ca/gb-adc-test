/** 
 * \file    gb_adc_test.c
 * \author  Frederic Chen
 * \date    April 8, 2016
 * \brief   Gertboard ADC/DAC loopback test on RPi
 *
 * Test Setup:
 * - RPi: Use SPI0 in GPIO_ALT0 mode (via S/W)
 * - Gertboard:
 *      - ADC: Use MCP3002 Channel 0, connect to RPi SPI0_CS_1
 *      - DAC: Use MCP4802 Channel 1, connect to RPi SPI0_CS_0
 *      - Jumpers: loopback above both ADC/DAC I/O pins
 *      - Jumpers: close GP7~GP11 to all corespondent SPI0 pins
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>   // mmap lib
#include <fcntl.h>      // file I/O
#include <unistd.h>     // file I/O
#include "spi_access.h" // SPI0 and Gertboard ADC/DAC public API headers

int main(int argc, char **argv) {
    unsigned int adc_factor;
    unsigned int val;
    float volt;
    int	rc;

    // initialize GPIO and SPI0 first
    spi_access_init();

    printf("Raspberry Pi Gertboard ADC/DAC test program start ...\n");
    while (true) {
        adc_factor = 0;
        printf("> Please enter the ADC output 8 bits factor (1~255, 0 to exit): ");
        rc = scanf("%d", &adc_factor);

        if (!rc || adc_factor == 0) {
            break;
        }

        // write a value to DAC channel 1
        volt = 2.048 * (adc_factor/256.0);
        printf("Expected DAC output voltage:%g\n", volt);
        write_dac(1, adc_factor);

        // read a value from ADC channel 0
        val = read_adc(0);
        volt = 3.3 * (val/1024.0);
        printf("Measured ADC inout voltage:%g\n", volt);

    }

    // avoid compiler warning
    val = val;
    volt = volt;

    spi_access_cleanup();
    printf("Program exists\n");

    return 0;
}
