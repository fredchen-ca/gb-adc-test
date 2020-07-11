#ifndef _GPIO_ACCESS_H_
#define _GPIO_ACCESS_H_

/** 
 * \file    gpio_access.h
 * \author  Frederic Chen
 * \date    April 8, 2016
 * \brief   RPi GPIO Basic Access Utility
 *
 * Support BCM2835/2836 full range GPIO pins 0~53
 */

// BCM SoC I/O Address on different RPi variants
#define RPi2    // We use RPi 2
#ifdef  RPi2    // RPi 2
#define BCM_IO_MAP  0x3f000000
#else           // Legacy RPi
#define BCM_IO_MAP  0x20000000
#endif

// GPIO registers set functional access macros
#define GPIO_FSEL(x)    *(gpio+x)
#define GPIO_SET(x)     *(gpio+7+x)
#define GPIO_CLR(x)     *(gpio+10+x)
#define GPIO_LEV(x)     *(gpio+13+x)
#define GPIO_PUD(x)     *(gpio+37+x)
#define GPIO_PUDCLK(x)  *(gpio+38+x)

// GPIO pin selected function values
typedef enum gpio_sei_mode_e {
    GPIO_INPUT  = 0x0,
    GPIO_OUTPUT = 0x1,
    GPIO_ALT0   = 0x4,
    GPIO_ALT1   = 0x5,
    GPIO_ALT2   = 0x6,
    GPIO_ALT3   = 0x7,
    GPIO_ALT4   = 0x3,
    GPIO_ALT5   = 0x2,
} gpio_sel_mode_t;

// GPIO read/write control values
typedef enum gpio_pin_val_e {
    GPIO_LOW    = 0,
    GPIO_HIGH   = 1,
} gpio_pin_val_t;

// GPIO pin internal pull up/down control settings
typedef enum gpio_pud_val_e {
    GPIO_PUD_OFF    = 0,
    GPIO_PUD_DN     = 1,
    GPIO_PUD_UP     = 2,
} gpio_pud_val_t;

/**
 * \brief   Basic GPIO Initialization API
 */
void gpio_access_init();

/**
 * \brief   Basic GPIO Access Clean Up API
 */
void gpio_access_cleanup();

/**
 * \brief   GPIO pin function selection
 *
 * \param   pin     pin number: 0~23
 * \param   mode    selected pin function
 */
void gpio_function_sel(unsigned int pin, gpio_sel_mode_t mode);

/**
 * \brief  GPIO pin write function
 *
 * \param   pin     pin number: 0~23
 * \param   val     value to write 
 */
void gpio_pin_write(unsigned int pin, gpio_pin_val_t val);

/**
 * \brief  GPIO pin read function
 *
 * \param   pin     pin number: 0~23
 * \param   val     read value buffer
 */
void gpio_pin_read(unsigned int pin, gpio_pin_val_t *val);

/**
 * \brief   GPIO pin internal pull up/down control
 *
 * \param   pin     pin number: 0~23
 * \param   mode    control setting
 */
void gpio_pin_pullup(unsigned int pin, gpio_pud_val_t val);

// delay 150 ticks for BCM2835 peripherals to be registered
void short_wait();

#endif  // _GPIO_ACCESS_H_
