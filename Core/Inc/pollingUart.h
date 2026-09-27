#ifndef POLLING_UART_H
#define POLLING_UART_H

#include "define.h"


namespace DriverArmCotexM4 {

    enum GPIO_Mode_Type { INPUT, OUTPUT, AF, ANALOG };
    
    struct gpio {
        volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2];
    };

    // RCC register map
    struct rcc {
        volatile uint32_t CR, PLLCFGR, CFGR, CIR, AHB1RSTR, AHB2RSTR, RESERVED[2],
            APB1RSTR, APB2RSTR, RESERVED1[2], AHB1ENR, AHB2ENR, RESERVED2[2],
            APB1ENR, APB2ENR, RESERVED3[2], AHB1LPENR, AHB2LPENR,RESERVED4[2],
            APB1LPENR, APB2LPENR, RESERVED5[2], BDCR, CSR,
            RESERVED6[2], SSCGR, PLLI2SCFGR, RESERVED7, DCKCFGR;
    };


    class GPIO {

    public:
        static inline void gpio_set_mode(uint16_t pin, uint8_t mode) {
            struct gpio *gpio = GPIO(PINBANK(pin));  // GPIO bank
            int n = PINNO(pin);                      // Pin number
            RCC->AHB1ENR |= BIT(PINBANK(pin));       // Enable GPIO clock
            gpio->MODER &= ~(3U << (n * 2));         // Clear existing setting
            gpio->MODER |= (mode & 3U) << (n * 2);   // Set new mode
        }

        static inline void gpio_set_af(uint16_t pin, uint8_t af_num) {
            struct gpio *gpio = GPIO(PINBANK(pin));  // GPIO bank
            int n = PINNO(pin);                      // Pin number
            gpio->AFR[n >> 3] &= ~(15UL << ((n & 7) * 4));
            gpio->AFR[n >> 3] |= ((uint32_t) af_num) << ((n & 7) * 4);
        }

        static inline void gpio_write(uint16_t pin, bool val) {
            struct gpio *gpio = GPIO(PINBANK(pin));
            gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
        }
    };

    static bool timer_expired(uint32_t *t, uint32_t prd, uint32_t now, uint32_t* start) {
   	
        if (*t == 0) 
        {
            *t = now + prd;
            *start = now;
        }

        if( ((uint32_t) (now - *start)) >= prd ) 
        {
            if ( (now - *t) >= prd ) 
            {
                *t = now + prd ;
                *start = now ;
            } 
            else 
            {
                *t = *t + prd;
                *start += prd;
            }
            return true;
        }
        else return false;
    }

    class PollingUART { 
        
        uint32_t isCarry(uint32_t frac) ;
        uint32_t* calBaud(uint32_t* buf, uint32_t baudrate, uint32_t fclk);
    public:
        void uart_init(struct uart *uart, unsigned long baud);
        void uart_write_byte(struct uart *uart, uint8_t byte);
        void uart_write_buf(struct uart *uart, char *buf, uint8_t len);
        uint32_t uart_read_ready(struct uart *uart);
        uint8_t uart_read_byte(struct uart *uart);

    };

};


#endif // POLLING_UART_H
