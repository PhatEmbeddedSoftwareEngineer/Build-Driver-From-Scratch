#include "Uart.h"
DriverArmCotexM4::DriverUART uartPollingMode;


extern "C" void SysTick_Handler(void)
{   
    DriverArmCotexM4::s_ticks++;
}
volatile uint8_t receivedByte = 0;
extern "C" void USART1_IRQHandler()
{
	static int cnt = 0; 

    if(uartPollingMode.uart_read_ready(UART1))  // Check if data is ready to be read
    {
        receivedByte = uartPollingMode.uart_read_byte(UART1);  // Read the received byte
        uartPollingMode.enable_TXEIE(UART2);  // Enable TXEIE Bit to trigger USART2 interrupt
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
        uartPollingMode.uart_write_byte_for_interrupt(UART2, receivedByte); 
        // uartPollingMode.disable_TXEIE(UART2);  // Disable TXEIE Bit
    }

}

extern "C" void DMA2_Stream7_IRQHandler()
{
    if(DMA2->HISR & BIT(27))  // Check if transfer complete flag is set
    {
        DMA2->HIFCR = BIT(27);  // Clear transfer complete flag
        // uartPollingMode.disable_TXEIE(UART1);  // Disable TXEIE Bit
    }
}

void feature_interrupt()
{
    static uint32_t timer = 0, start = 0;
    const uint32_t period = 500;
    if (DriverArmCotexM4::timer_expired(&timer, period, DriverArmCotexM4::s_ticks, &start))  // Check if timer expired
    {
        //uartPollingMode.enable_TXEIE(UART1);  // Enable TXEIE Bit to trigger USART1 interrupt
        uartPollingMode.uart_dma2_reTransmit(UART1, (uint32_t)DriverArmCotexM4::message, DriverArmCotexM4::leng);
        
    }
}

int main(void)  
{
	systick_init(16000000 / 1000); // 1ms tick
	uartPollingMode.uart_init(UART1, 115200);
    uartPollingMode.uart_init(UART2, 115200);
    uartPollingMode.enable_RXEIE(UART1);  // Enable RXEIE Bit to trigger USART1 interrupt
    
    char message[50] = "Good day, Alligator from DMA UART TX\n\r";
    uartPollingMode.uart_dma2_init(UART1, (uint32_t)message, 50);

    while (1)
    {
        feature_interrupt();  // Call feature_interrupt function to check if timer expired
    }

}

