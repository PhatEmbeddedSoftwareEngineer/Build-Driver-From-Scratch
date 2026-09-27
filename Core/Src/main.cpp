#include "pollingUart.h"

volatile uint32_t s_ticks = 0;
extern "C" void SysTick_Handler(void)
{   
    s_ticks++;
}

DriverArmCotexM4::PollingUART uartPollingMode;

int main(void)  
{
	systick_init(16000000 / 1000); // 1ms tick
	uartPollingMode.uart_init(UART1, 115200);
    uint32_t timer = 0, period = 500, start = 0;      // Declare timer and 500ms period

    char message[] = "Hello, Phat\n";  // Message to send

    while (1)
    {
        if (DriverArmCotexM4::timer_expired(&timer, period, s_ticks, &start))  // Check if timer expired
        {
            uartPollingMode.uart_write_buf(UART1, message, sizeof(message) - 1);  // Send message
        }

        if(uartPollingMode.uart_read_ready(UART1))
        {
            uint8_t received_byte = uartPollingMode.uart_read_byte(UART1);  // Read received byte
            uartPollingMode.uart_write_byte(UART1, received_byte);          // Echo back the received byte
        }

    }

}

