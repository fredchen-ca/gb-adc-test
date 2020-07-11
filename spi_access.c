/** 
 * \file    spi_access.c
 * \author  Frederic Chen
 * \date    April 8, 2016
 * \brief   RPi SPI Access Utility for Gertboard ADC/DAC access
 *
 * Support RPi SPI0 access in GPIO ALT0 mode only
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>   // mmap lib
#include <fcntl.h>      // file I/O
#include <unistd.h>     // file I/O
#include "apps_logger.h"    // private lightweight application logging macros
#include "gpio_access.h"// GPIO utility headers
#include "spi_access.h" // SPI0 and Gertboard ADC/DAC public API headers

// SPI0 Register Address 
#define SPI0_CNTLSTAT   *(spi0 + 0)
#define SPI0_FIFO       *(spi0 + 1)
#define SPI0_CLKSPEED   *(spi0 + 2)
#define SPI0_DATALEN    *(spi0 + 3)
#define SPI0_LOSSTOH    *(spi0 + 4)
#define SPI0_DMACTRL    *(spi0 + 5)

// SPI0_CNTLSTAT register bits
#define SPI0_CS_CS2ACTHIGH  0x00800000  // CS2 active high
#define SPI0_CS_CS1ACTHIGH  0x00400000  // CS1 active high
#define SPI0_CS_CS0ACTHIGH  0x00200000  // CS0 active high
#define SPI0_CS_RXFIFOFULL  0x00100000  // Receive FIFO full
#define SPI0_CS_RXFIFO3_4   0x00080000  // Receive FiFO 3/4 full
#define SPI0_CS_TXFIFOSPCE  0x00040000  // Transmit FIFO has space
#define SPI0_CS_RXFIFODATA  0x00020000  // Receive FIFO has data
#define SPI0_CS_DONE        0x00010000  // SPI transfer done, WRT to CLR!
#define SPI0_CS_MOSI_INPUT  0x00001000  // MOSI is input, read from MOSI (bi-dir mode)
#define SPI0_CS_DEASRT_CS   0x00000800  // De-assert CS at end
#define SPI0_CS_RX_IRQ      0x00000400  // Receive irq enable
#define SPI0_CS_DONE_IRQ    0x00000200  // irq when done
#define SPI0_CS_DMA_ENABLE  0x00000100  // Run in DMA mode
#define SPI0_CS_ACTIVATE    0x00000080  // Activate:be high before starting
#define SPI0_CS_CS_POLARIT  0x00000040  // Chip selects active high
#define SPI0_CS_CLRTXFIFO   0x00000020  // Clear TX FIFO (auto clear bit)
#define SPI0_CS_CLRRXFIFO   0x00000010  // Clear RX FIFO (auto clear bit)
#define SPI0_CS_CLRFIFOS    0x00000030  // Clear TX/RX FIFO (auto clear bit)
#define SPI0_CS_CLK_IDLHI   0x00000008  // Clock pin is high when idle
#define SPI0_CS_CLKTRANS    0x00000004  // 0:first clock in middle of data bit
                                        // 1:first clock at begin of data bit
#define SPI0_CS_CHIPSEL0    0x00000000  // Use chip select 0
#define SPI0_CS_CHIPSEL1    0x00000001  // Use chip select 1
#define SPI0_CS_CHIPSEL2    0x00000002  // No chip select (eg. use GPIO pin)
#define SPI0_CS_CHIPSELN    0x00000003  // No chip select (eg. use GPIO pin)

#define SPI0_CS_CLRALL     (SPI0_CS_CLRFIFOS|SPI0_CS_DONE)

// SPI Bus Speed = 250MHz / Divsor(in power of 2)
#define SPI_SPEED           256     // speed divisor, speed derived to ~1MHz

// Gertboard DAC/ADC H/W configuration
#define GB_DAC_SPI0_CS  SPI0_CS_CHIPSEL0    // DAC MCP4802 uses RPi SPI0 CS_0
#define GB_ADC_SPI0_CS  SPI0_CS_CHIPSEL1    // ADC MCP3002 uses RPi SPI0 CS_1

const unsigned int SPI0_BASE = BCM_IO_MAP + 0x204000;   // SPI0 base address
const unsigned int SPI0_LEN = 0x18;     // 6 SPI0 registers memory block to map

volatile unsigned int *spi0 = NULL;     // mapped SPI0 I/O block

unsigned int spi0_clients = 0;    // ensure exclusive access


// setup SPI Bus Speed = 250MHz / Divsor(in power of 2)
static void setup_spi0(unsigned int speed_div) {

    SPI0_CLKSPEED = speed_div;
    
    // clear FIFO and status bits
    SPI0_CNTLSTAT = SPI0_CS_CLRALL;
}

/**
 * \brief   Basic SPI Initialization API
 */
void spi_access_init() {

    int mem_fd = 0;
    void *mmap_addr = MAP_FAILED;

    // currently limit to one client only
    if (spi0_clients > 0) {
        LOG_ERR("SPI0 already been used, program abort!");
        exit(1);
    }

    // initialize GPIO node first
    gpio_access_init();
    
    if (!spi0) {
        mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
        if (mem_fd < 0) {
            LOG_ERR("%s", "cannot open /dev/mem");
            exit(1);
        }
        mmap_addr = mmap(NULL,   // any memory space
                         SPI0_LEN,   // mapped memory block size
                         PROT_READ|PROT_WRITE|PROT_EXEC, // access permission
                         MAP_SHARED|MAP_LOCKED,
                         mem_fd,
                         SPI0_BASE);

        close(mem_fd);

        if (mmap_addr == MAP_FAILED) {
            LOG_ERR("mmap error");
            exit(1);
        }

        spi0 = (volatile unsigned int *) mmap_addr;
        LOG_DBG("spi0 mapped to %p", spi0);

        // using GPIO ALT0 mode for SPI0
        gpio_function_sel( 7, GPIO_ALT0);  // SPI_CE1_N   
        gpio_function_sel( 8, GPIO_ALT0);  // SPI_CE0_N   
        gpio_function_sel( 9, GPIO_ALT0);  // SPI_MISO   
        gpio_function_sel(10, GPIO_ALT0);  // SPI_MOSI   
        gpio_function_sel(11, GPIO_ALT0);  // SPI_CLK

        // setup SPI bus speed
        setup_spi0(SPI_SPEED);
    }                   
    spi0_clients++;
}

/**
 * \brief   Basic SPI Access Clean Up API
 */
void spi_access_cleanup() {
    
    if (spi0 && (--spi0_clients == 0)) {
        if (munmap((void *) spi0, SPI0_LEN) < 0) {
            LOG_ERR("munmap(%p) failed", spi0);
            exit(1);
        }

        spi0 = (volatile unsigned int *) NULL;
        LOG_DBG("unmap spi0 done");
    }

    // clean up GPIO node resource
    gpio_access_cleanup();
}

/**
 * \brief   Write one 8-bit value to DAC channel 0 or 1
 *
 * \param   chan    DAC MCP4802 channel: (0/1)
 * \param   val     8-bit value to write 
 *
 * Note: Expected Gertboard DAC output voltage: 2.048v * (val/256)
 */
void write_dac(int chan, int val) {

    unsigned char v1, v2, dummy;
    int status;

    // we only use least significant 8 bits 
    val &= 0xff;

    // build 1st byte: Write + channel + most significant 4 bits of 8-bit value
    v1 = 0x30 | (chan << 7) | (val >> 4);
    // build 2nd byte: remained least significant 4 bits of 8-bit value at MSB end
    v2 = (val << 4) & 0xf0;
    LOG_DBG("Write DAC v1:%#0x, v2:%#0x from input val:%#0x", v1, v2, val);

    // wait for previous registering operation finished, if has
    short_wait();

    SPI0_CNTLSTAT = GB_ADC_SPI0_CS | SPI0_CS_ACTIVATE;

    SPI0_FIFO = v1; // write 1st byte
    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_RXFIFODATA) == 0);
    dummy = SPI0_FIFO; // read 1st dummy byte (don't care)

    SPI0_FIFO = v2; // write 2nd byte
    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_RXFIFODATA) == 0);
    dummy = SPI0_FIFO; // read 2nd dummy byte (don't care)

    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_DONE) == 0);
    SPI0_CNTLSTAT = SPI0_CS_DONE; // clear DONE status bit

    LOG_DBG("Write DAC val:%#0x from input v1:%#0x, v2:%#0x", val, v1, v2);
    dummy = dummy; // to avoid compiler warning
}

/**
 * \brief   Read one 10-bit value from ADC channel 0 or 1
 *
 * \param   chan    ADC MCP3002 channel: (0/1)
 * \return  10-bit value buffer for read 
 *
 * Note: Expected Gertboard measured input voltage: 3.3v * (readout/1024)
 */
int read_adc(int chan) {

    unsigned char v1, v2;
    int status;
    int val;

    // set mode as single ended, MS-bit comes out first, channel with leading zero bit as
    // 0110 1000 + chan
    v1 = 0x68 | (chan << 4);

    // wait for previous registering operation finished, if has
    short_wait();

    SPI0_CNTLSTAT = GB_DAC_SPI0_CS | SPI0_CS_ACTIVATE;

    SPI0_FIFO = v1; // write 1st byte
    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_RXFIFODATA) == 0);
    v1 = SPI0_FIFO; // read 1st byte

    SPI0_FIFO = 0; // write 2nd byte (dummy data)
    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_RXFIFODATA) == 0);
    v2 = SPI0_FIFO; // read 2nd byte

    do {
        status = SPI0_CNTLSTAT;
    } while ((status & SPI0_CS_DONE) == 0);
    SPI0_CNTLSTAT = SPI0_CS_DONE; // clear DONE status bit

    // combine the LSB 8-bit and MSB 2-bit values into 10-bit integer
    val = ((v1 << 8) | v2) & 0x3ff;
    LOG_DBG("Read ADC val:%#0x from input v1:%#0x, v2:%#0x", val, v1, v2);

    return val;   
}


