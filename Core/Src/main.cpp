#include "Uart.h"
DriverArmCotexM4::DriverUART uartPollingMode;


extern "C" void SysTick_Handler(void)
{   
    DriverArmCotexM4::s_ticks++;
}
volatile uint8_t receivedByte = 0;
volatile uint8_t receivedByteDMA = 0;
extern "C" void USART1_IRQHandler()
{
	static int cnt = 0;  

    // if(uartPollingMode.uart_read_ready(UART1))  // Check if data is ready to be read
    // {
    //     receivedByte = uartPollingMode.uart_read_byte(UART1);  // Read the received byte
    //     uartPollingMode.enable_TXEIE(UART2);  // Enable TXEIE Bit to trigger USART2 interrupt
    // }

    if(uartPollingMode.uart_read_idle(UART1) && uartPollingMode.is_uart_set_idle_isr(UART1))  // Check if UART is idle and IDLE ISR is enabled
    {
        uartPollingMode.uart_dmarx_idle_isr(PORT_UART1);
        uint32_t tmp = uartPollingMode.uart_read_register_status(UART1);  // Read the status register to clear the IDLE flag
        (void)tmp;
        tmp = uartPollingMode.uart_read_register_data(UART1);  // Read the data register to clear the IDLE flag
        (void)tmp;
    }

    // else if(uartPollingMode.uart_write_complete(UART1))  // Check if transmission is complete
    // {
    //     if(DriverArmCotexM4::leng <= cnt)
    //     {
    //         cnt = 0;
    //         uartPollingMode.disable_TXEIE(UART1);  // Disable TXEIE Bit
    //     }
    //     else 
    //     {
    //         uartPollingMode.uart_write_byte(UART1, DriverArmCotexM4::message[cnt++]);  // Send message
    //     }
    // }
    
}

extern "C" void USART2_IRQHandler()
{
    uartPollingMode.disable_TXEIE(UART2);  // Disable TXEIE Bit
    if(uartPollingMode.uart_write_ready(UART2))  // Check if transmission is complete
    {
        uartPollingMode.uart_write_byte_for_interrupt(UART2, receivedByteDMA); 
        // uartPollingMode.disable_TXEIE(UART2);  // Disable TXEIE Bit
    }

}

extern "C" void DMA2_Stream7_IRQHandler()
{
    if(DMA2->HISR & BIT(27))  // Check if transfer complete flag is set
    {
        DMA2->HIFCR = BIT(27);  // Clear transfer complete flag
        // uartPollingMode.disable_TXEIE(UART1);  // Disable TXEIE Bit
        //uartPollingMode.enable_TXEIE(UART2);
        uartPollingMode.uart_dmatx_done_isr(PORT_UART1);
    }

}

extern "C" void DMA2_Stream5_IRQHandler()
{
    if(DMA2->HISR & BIT(11))  // Check if transfer complete flag is set
    {
        DMA2->HIFCR |= BIT(11);  // Clear transfer complete flag
        // uartPollingMode.disable_TXEIE(UART1);  // Disable TXEIE Bit
        //uartPollingMode.enable_TXEIE(UART2);
        uartPollingMode.uart_dmarx_done_isr(PORT_UART1);
    }

    if(DMA2->HISR & BIT(10))
    {
        uartPollingMode.uart_dmarx_half_done_isr(PORT_UART1);
        DMA2->HIFCR |= BIT(10);
    }

}

void feature_interrupt()
{
    static uint32_t timer = 0, start = 0;
    const uint32_t period = 500;
    static uint8_t buf[UART1_RX_BUF_SIZE] = {0};
    uint16_t size = 0;
    if (DriverArmCotexM4::timer_expired(&timer, period, DriverArmCotexM4::s_ticks, &start))  // Check if timer expired
    {
        //uartPollingMode.enable_TXEIE(UART1);  // Enable TXEIE Bit to trigger USART1 interrupt
        //uartPollingMode.uart_dma2_reTransmit(UART1, (uint32_t)DriverArmCotexM4::message, DriverArmCotexM4::leng);
        //uartPollingMode.uart_write_byte_for_polling(UART2, receivedByteDMA);
        size = uartPollingMode.uart_read(PORT_UART1, buf, sizeof(buf));
        if(size != 0)
        {
            uartPollingMode.uart_write(PORT_UART1, buf, size);
            uartPollingMode.uart_poll_dma_tx(PORT_UART1);
        }
    }
}


int main(void)  
{
	systick_init(16000000 / 1000); // 1ms tick
	uartPollingMode.uart_init(UART1, 115200);
    uartPollingMode.uart_init(UART2, 115200);
    //uartPollingMode.enable_RXEIE(UART1);  // Enable RXEIE Bit to trigger USART1 interrupt
    
    uartPollingMode.uart_dma2_init_tx(UART1);
    uartPollingMode.uart_dma2_init_rx(UART1);
    while (1)
    {
        feature_interrupt();  // Call feature_interrupt function to check if timer expired
    }

}

using init_function = void (*)(void);

extern "C" {
extern uint32_t _sbss, _ebss, _sdata, _edata, _sidata;
extern void _estack(void);
extern init_function __init_array_start[];
extern init_function __init_array_end[];

void initialize_memory(void)
{
    for (uint32_t *dst = &_sbss; dst < &_ebss; ++dst)
    {
        *dst = 0;
    }

    for (uint32_t *dst = &_sdata, *src = &_sidata; dst < &_edata;)
    {
        *dst++ = *src++;
    }
}

void run_global_constructors(void)
{
    for (auto *function = __init_array_start;
         function < __init_array_end; ++function)
    {
        (*function)();
    }
}

__attribute__((naked, noreturn, used, section(".text.Reset_Handler")))
void _reset(void)
{
    __asm volatile(
        "ldr sp, =_estack\n"
        "bl initialize_memory\n"
        "bl run_global_constructors\n"
        "bl main\n"
        "1: b 1b\n");
}

void Default_Handler(void)
{
    while (1)
    {
    }
}

using vector_handler = void (*)(void);
__attribute__((used, section(".vectors")))
vector_handler const g_vectors[16 + 85] = {
    &_estack,                 // Initial Stack Pointer
    _reset,                   // Reset
    Default_Handler,          // NMI
    Default_Handler,          // HardFault
    Default_Handler,          // MemManage
    Default_Handler,          // BusFault
    Default_Handler,          // UsageFault
    0,                        // Reserved
    0,                        // Reserved
    0,                        // Reserved
    0,                        // Reserved
    Default_Handler,          // SVCall
    Default_Handler,          // Debug Monitor
    0,                        // Reserved
    Default_Handler,          // PendSV
    SysTick_Handler,          // SysTick
    Default_Handler,          // WWDG
    Default_Handler,          // PVD
    Default_Handler,          // TAMP_STAMP
    Default_Handler,          // RTC_WKUP
    Default_Handler,          // FLASH
    Default_Handler,          // RCC
    Default_Handler,          // EXTI0
    Default_Handler,          // EXTI1
    Default_Handler,          // EXTI2
    Default_Handler,          // EXTI3
    Default_Handler,          // EXTI4
    Default_Handler,          // DMA1_Stream0
    Default_Handler,          // DMA1_Stream1
    Default_Handler,          // DMA1_Stream2
    Default_Handler,          // DMA1_Stream3
    Default_Handler,          // DMA1_Stream4
    Default_Handler,          // DMA1_Stream5
    Default_Handler,          // DMA1_Stream6
    Default_Handler,          // ADC
    0,                        // Reserved IRQ 19
    0,                        // Reserved IRQ 20
    0,                        // Reserved IRQ 21
    0,                        // Reserved IRQ 22
    Default_Handler,          // EXTI9_5
    Default_Handler,          // TIM1_BRK_TIM9
    Default_Handler,          // TIM1_UP_TIM10
    Default_Handler,          // TIM1_TRG_COM_TIM11
    Default_Handler,          // TIM1_CC
    Default_Handler,          // TIM2
    Default_Handler,          // TIM3
    Default_Handler,          // TIM4
    Default_Handler,          // I2C1_EV
    Default_Handler,          // I2C1_ER
    Default_Handler,          // I2C2_EV
    Default_Handler,          // I2C2_ER
    Default_Handler,          // SPI1
    Default_Handler,          // SPI2
    USART1_IRQHandler,        // USART1
    USART2_IRQHandler,        // USART2
    0,                        // Reserved IRQ 39
    Default_Handler,          // EXTI15_10
    Default_Handler,          // RTC_Alarm
    Default_Handler,          // OTG_FS_WKUP
    0,                        // Reserved IRQ 43
    0,                        // Reserved IRQ 44
    0,                        // Reserved IRQ 45
    0,                        // Reserved IRQ 46
    Default_Handler,          // DMA1_Stream7
    0,                        // Reserved IRQ 48
    Default_Handler,          // SDIO
    Default_Handler,          // TIM5
    Default_Handler,          // SPI3
    0,                        // Reserved IRQ 52
    0,                        // Reserved IRQ 53
    0,                        // Reserved IRQ 54
    0,                        // Reserved IRQ 55
    Default_Handler,          // DMA2_Stream0
    Default_Handler,          // DMA2_Stream1
    Default_Handler,          // DMA2_Stream2
    Default_Handler,          // DMA2_Stream3
    Default_Handler,          // DMA2_Stream4
    0,                        // Reserved IRQ 61
    0,                        // Reserved IRQ 62
    0,                        // Reserved IRQ 63
    0,                        // Reserved IRQ 64
    0,                        // Reserved IRQ 65
    0,                        // Reserved IRQ 66
    Default_Handler,          // OTG_FS
    DMA2_Stream5_IRQHandler,  // DMA2_Stream5
    Default_Handler,          // DMA2_Stream6
    DMA2_Stream7_IRQHandler,  // DMA2_Stream7
    Default_Handler,          // USART6
    Default_Handler,          // I2C3_EV
    Default_Handler,          // I2C3_ER
    0,                        // Reserved IRQ 74
    0,                        // Reserved IRQ 75
    0,                        // Reserved IRQ 76
    0,                        // Reserved IRQ 77
    0,                        // Reserved IRQ 78
    0,                        // Reserved IRQ 79
    0,                        // Reserved IRQ 80
    Default_Handler,          // FPU
    0,                        // Reserved IRQ 82
    0,                        // Reserved IRQ 83
    Default_Handler           // SPI4
};
}
