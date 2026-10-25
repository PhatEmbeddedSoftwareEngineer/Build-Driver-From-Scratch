#ifndef DEFINE_H
#define DEFINE_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define NULL (0)
#define TRUE 1 
#define FALSE 0

#define FREQ 16000000  // CPU frequency, 16 Mhz
#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8) // get GPIO
#define DMA2_BASE_ADDR 0x40026400UL
#define DMA1_BASE_ADDR 0x40026000UL

#define DEVFS_DRIVER_DECLARE_CONFIG_STATE(object)                                        \
  const object##_config_t *config = handle->config;                                      \
  object##_state_t *state = (object##_state_t *)handle->state


struct DMA_Stream {
    volatile uint32_t CR, NDTR, PAR, M0AR, M1AR, FCR;
};

struct DMA_Type
{
    volatile uint32_t LISR;   // 0x00
    volatile uint32_t HISR;   // 0x04
    volatile uint32_t LIFCR;  // 0x08
    volatile uint32_t HIFCR;  // 0x0C

    DMA_Stream STREAM[8];     //  0x10
};

#define DMA1 ((DMA_Type*)DMA1_BASE_ADDR)
#define DMA2 ((DMA_Type*)DMA2_BASE_ADDR)

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

#define MCU_UART_PORTS 6

#define UART1 ((struct uart *) 0x40011000)
#define UART2 ((struct uart *) 0x40004400)
#define UART6 ((struct uart *) 0x40011400)
#define MCU_UART_REGS { UART1, UART2, 0, 0, 0, UART6 }
#define PORT_UART1 0
#define PORT_UART2 1
#define PORT_UART6 5


#define NVIC_BASE_ADDR	0xE000E100UL
#define NVIC_PRIORITY_BASE 0xE000E400UL
struct nvic_setPrio {
	uint32_t IPRO[60];
};

struct nvic_setenable {
	uint32_t ISER[8] ;
};

#define POSITION_USART1	37
#define POSITION_USART2	38
#define POSITION_USART6	71
#define POSITION_DMA2_Stream7	70
#define POSITION_DMA2_Stream5	68

#endif // DEFINE_H
