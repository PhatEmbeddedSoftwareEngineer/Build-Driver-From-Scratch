#include "Uart.h"
DriverArmCotexM4::DriverUART uartPollingMode;
volatile uint32_t s_ticks = 0;
extern "C" void SysTick_Handler(void)
{   
    s_ticks++;
}

char message[] = "Hello, Phat\n";  // Message to send
int leng = sizeof(message) - 1;  // Length of the message
void disable_TXEIE() ;
extern "C" void USART1_IRQHandler()
{
	static int cnt = 0; 
    if(leng == cnt)
    {
        cnt = 0;
        disable_TXEIE();  // Disable TXEIE Bit
    }
    else 
    {
        uartPollingMode.uart_write_byte(UART1, message[cnt++]);  // Send message
    }
    
}

void disable_TXEIE() 
{
    UART1->CR1 &= ~BIT(7);  // Disable TXEIE Bit
}
void enable_TXEIE() 
{
    UART1->CR1 |= BIT(7);  // Enable TXEIE Bit
}


int main(void)  
{
	systick_init(16000000 / 1000); // 1ms tick
	uartPollingMode.uart_init(UART1, 115200);
    uint32_t timer = 0, period = 500, start = 0;      // Declare timer and 500ms period

    

    while (1)
    {
        if (DriverArmCotexM4::timer_expired(&timer, period, s_ticks, &start))  // Check if timer expired
        {
            enable_TXEIE();  // Enable TXEIE Bit to trigger USART1 interrupt
        }

        if(uartPollingMode.uart_read_ready(UART1))
        {
            uint8_t received_byte = uartPollingMode.uart_read_byte(UART1);  // Read received byte
            uartPollingMode.uart_write_byte(UART1, received_byte);          // Echo back the received byte
        }

    }

}

