/** 
 * \file    gpio_access.c
 * \author  Frederic Chen
 * \date    April 8, 2016
 * \brief   RPi GPIO Base Access Utility
 *
 * Support BCM2835/2836 full range GPIO pins 0~53
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>   // mmap lib
#include <fcntl.h>      // file I/O
#include <unistd.h>     // file I/O
#include "apps_logger.h"    // private lightweight application logging macros
#include "gpio_access.h"


const   unsigned int GPIO_BASE = BCM_IO_MAP + 0x200000; // GPIO Registers Base Address 
const   unsigned int GPIO_LEN = 0xb4;   // total GPIO registers memory block size mapped

volatile unsigned int *gpio = NULL;     // mapped GPIO I/O block

unsigned int gpio_clients = 0;  // ensure exclusive access


/**
 * \brief   Basic GPIO Initialization API
 */
void gpio_access_init() {

    int mem_fd = 0;
    void *mmap_addr = MAP_FAILED;

    // currently limit to one client only
    if (gpio_clients > 0) {
        LOG_ERR("GPIO already been used, program abort!");
        exit(1);
    }

    if (!gpio) {
        mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
        if (mem_fd < 0) {
            LOG_ERR("%s", "cannot open /dev/mem");
            exit(1);
        }
        mmap_addr = mmap(NULL,   // any memory space
                         GPIO_LEN,   // mapped memory block size
                         PROT_READ|PROT_WRITE|PROT_EXEC, // access permission
                         MAP_SHARED|MAP_LOCKED,
                         mem_fd,
                         GPIO_BASE);

        close(mem_fd);

        if (mmap_addr == MAP_FAILED) {
            LOG_ERR("mmap error");
            exit(1);
        }

        gpio = (volatile unsigned int *) mmap_addr;
        LOG_DBG("gpio mapped to %p", gpio);
    }                   
    gpio_clients++;
}

/**
 * \brief   Basic GPIO Access Clean Up API
 */
void gpio_access_cleanup() {
    
    if (gpio && (--gpio_clients == 0)) {
        if (munmap((void *) gpio, GPIO_LEN) < 0) {
            LOG_ERR("munmap(%p) failed", gpio);
            exit(1);
        }

        gpio = (volatile unsigned int *) NULL;
        LOG_DBG("unmap gpio done");
    }
}

/**
 * \brief   GPIO pin function selection
 *
 * \param   pin     pin number: 0~23
 * \param   mode    selected pin function
 */
void gpio_function_sel(unsigned int pin, gpio_sel_mode_t mode) {
    int idx;    // pin index
    int pos;    // bit position

    // validate inputs
    if (pin > 53) {
        LOG_ERR("invalid input pin number(%d)", pin);
        return;
    }

    if (!gpio) {
        LOG_ERR("GPIO address not mapped yet");
        return;
    }

    idx = pin / 10;         // 10 pins per register
    pos = (pin % 10) * 3;   // 3 bits per pin 
    LOG_DBG("got pin:%d: idx:%d, pos:%d", pin, idx, pos);

    // reset current mode first
    GPIO_FSEL(idx) &= ~(7 << pos);
    if (mode != GPIO_INPUT) {
        GPIO_FSEL(idx) |= (mode << pos);
    }
    LOG_DBG("gpio#%d selec function %d", pin, mode);
}

/**
 * \brief  GPIO pin write function
 *
 * \param   pin     pin number: 0~23
 * \param   val     value to write 
 */
void gpio_pin_write(unsigned int pin, gpio_pin_val_t val) {
    int idx;    // pin index
    int pos;    // bit position

    // validate inputs
    if (pin > 53) {
        LOG_ERR("invalid input pin number(%d)", pin);
        return;
    }

    if (!gpio) {
        LOG_ERR("GPIO address not mapped yet");
        return;
    }

    idx = pin / 32;   // 32 pins per register
    pos = pin % 32;   // 1 bit per pin
    LOG_DBG("got pin:%d: idx:%d, pos:%d", pin, idx, pos);

    if (val == GPIO_HIGH) {
        GPIO_SET(idx) = (1 << pos);
        LOG_DBG("gpio#%d output HIGH", pin);
    } else {
        GPIO_CLR(idx) = (1 << pos);
        LOG_DBG("gpio#%d output LOW", pin);
    }
}

/**
 * \brief  GPIO pin read function
 *
 * \param   pin     pin number: 0~23
 * \param   val     read value buffer
 */
void gpio_pin_read(unsigned int pin, gpio_pin_val_t *val) {
    int idx;    // pin index
    int pos;    // bit position

    // validate inputs
    if (pin > 53) {
        LOG_ERR("invalid input pin number(%d)", pin);
        return;
    }
    if (!val) {
        LOG_ERR("NULL input:val detected");
        return;
    }

    if (!gpio) {
        LOG_ERR("GPIO address not mapped yet");
        return;
    }

    idx = pin / 32;   // 32 pins per register
    pos = pin % 32;   // 1 bit per pin

    *val = (GPIO_LEV(idx) & (1 << pos)) ? GPIO_HIGH : GPIO_LOW;
    LOG_DBG("got pin:%d: idx:%d, pos:%d, input:%d", pin, idx, pos, *val);
}

/**
 * \brief   GPIO pin internal pull up/down control
 *
 * \param   pin     pin number: 0~23
 * \param   mode    control setting
 */
void gpio_pin_pullup(unsigned int pin, gpio_pud_val_t val) {
    int idx;    // pin index
    int pos;    // bit position

    // validate inputs
    if (pin > 53) {
        LOG_ERR("invalid input pin number(%d)", pin);
        return;
    }

    if (!gpio) {
        LOG_ERR("GPIO address not mapped yet");
        return;
    }

    idx = pin / 32;   // 32 pins per register
    pos = pin % 32;   // 1 bit per pin
    LOG_DBG("got pin:%d: idx:%d, pos:%d, pullup:%d", pin, idx, pos, val);

    // execute pull up commands sequence
    GPIO_PUD(idx) = val;    // setup pullup control bit
    short_wait();           // wait 150 ticks
    GPIO_PUDCLK(idx) = (1 << pos);  // send strobe high
    short_wait();           // wait 150 ticks
    GPIO_PUD(idx) = 0;      // remove control signal
    GPIO_PUDCLK(idx) = 0;   // send strobe low
}

// delay 150 ticks for BCM2835 peripherals to be registered
void short_wait() {
    int i;

    for (i = 0; i < 150; i++) {
        asm volatile ("");  // nop    
    }
}

