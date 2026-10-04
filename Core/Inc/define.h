#ifndef DEFINE_H
#define DEFINE_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


#define TRUE 1 
#define FALSE 0

#define FREQ 16000000  // CPU frequency, 16 Mhz
#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8) // get GPIO

struct systick {
    volatile uint32_t CTRL, LOAD, VAL, CALIB;
};

#define SYSTICK ((struct systick *) 0xE000E010)  // DUI0553A_cortex_m4_dgug 4.4

#define RCC ((struct rcc *) 0x40023800)
#define GPIO(bank) ((struct gpio *) (0x40020000 + 0x400 * (bank)))

void systick_init(uint32_t ticks) ;

struct uart {
    volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
};

#define UART1 ((struct uart *) 0x40011000)
#define UART2 ((struct uart *) 0x40004400)
#define UART6 ((struct uart *) 0x40011400)

#define NVIC_BASE_ADDR	0xE000E100UL
#define NVIC_PRIORITY_BASE 0xE000E400UL
struct nvic_setPrio {
	uint32_t IPRO[60];
};

struct nvic_setenable {
	uint32_t ISER[8] ;
};

#define POSITION_USART1	37

#endif // DEFINE_H
