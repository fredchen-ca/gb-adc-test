#ifndef _SPI_ACCESS_H_
#define _SPI_ACCESS_H_

/** 
 * \file    spi_access.h
 * \author  Frederic Chen
 * \date    April 8, 2016
 * \brief   RPi SPI Access Utility for Gertboard ADC/DAC access
 *
 * Support RPi SPI0 access in GPIO ALT0 mode only
 */

/**
 * \brief   Basic SPI Initialization API
 */
void spi_access_init();

/**
 * \brief   Basic SPI Access Clean Up API
 */
void spi_access_cleanup();

/**
 * \brief   Write one 8-bit value to DAC channel 0 or 1
 *
 * \param   chan    DAC MCP4802 channel: (0/1)
 * \param   val     8-bit value to write 
 *
 * Note: Expected Gertboard DAC output voltage: 2.048v * (val/256)
 */
void write_dac(int chan, int val);

/**
 * \brief   Read one 10-bit value from ADC channel 0 or 1
 *
 * \param   chan    ADC MCP3002 channel: (0/1)
 * \return  10-bit value buffer for read 
 *
 * Note: Expected Gertboard measured input voltage: 3.3v * (readout/1024)
 */
int read_adc(int chan);

#endif  // _SPI_ACCESS_H_
