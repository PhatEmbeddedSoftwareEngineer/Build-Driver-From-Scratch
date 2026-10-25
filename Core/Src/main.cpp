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
