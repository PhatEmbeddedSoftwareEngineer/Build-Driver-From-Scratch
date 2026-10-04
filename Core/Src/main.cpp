#include "Uart.h"
DriverArmCotexM4::DriverUART uartPollingMode;

extern "C" void SysTick_Handler(void)
{   
    DriverArmCotexM4::s_ticks++;
}

extern "C" void USART1_IRQHandler()
{
	static int cnt = 0; 

    if(uartPollingMode.uart_read_ready(UART1))  // Check if data is ready to be read
    {
        uint8_t receivedByte = uartPollingMode.uart_read_byte(UART1);  // Read the received byte
        uartPollingMode.uart_write_byte(UART1, receivedByte);  // Send message
    }

    if(uartPollingMode.uart_write_complete(UART1))  // Check if transmission is complete
    {
        if(DriverArmCotexM4::leng == cnt)
        {
            cnt = 0;
            uartPollingMode.disable_TXEIE();  // Disable TXEIE Bit
        }
        else 
        {
            uartPollingMode.uart_write_byte(UART1, DriverArmCotexM4::message[cnt++]);  // Send message
        }
    }
    
}




int main(void)  
{
	systick_init(16000000 / 1000); // 1ms tick
	uartPollingMode.uart_init(UART1, 115200);
    uint32_t timer = 0, period = 1000, start = 0;      // Declare timer and 500ms period

    while (1)
    {
        if (DriverArmCotexM4::timer_expired(&timer, period, DriverArmCotexM4::s_ticks, &start))  // Check if timer expired
        {
            uartPollingMode.enable_TXEIE();  // Enable TXEIE Bit to trigger USART1 interrupt
        }


    }

}

