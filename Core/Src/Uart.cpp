#include "Uart.h"

volatile uint32_t DriverArmCotexM4::s_ticks = 0;
char DriverArmCotexM4::message[] = "Hello, Phat\n";
int DriverArmCotexM4::leng = sizeof(message) - 1;

static inline void spin(volatile uint32_t count) {
  while (count--) asm("nop");
}

void systick_init(uint32_t ticks) 
{
  SYSTICK->LOAD = ticks - 1;
  SYSTICK->VAL = 0;
  SYSTICK->CTRL = BIT(0) | BIT(1) | BIT(2);  // Enable systick
}

uint32_t DriverArmCotexM4::DriverUART::isCarry(uint32_t frac)
{
    uint32_t n = frac / 100;
    return (frac % 100) > 50 ? (n + 1) : n;
}

uint32_t* DriverArmCotexM4::DriverUART::calBaud(uint32_t* buf, uint32_t baudrate, uint32_t fclk)
{
    uint32_t mantissa = fclk/(baudrate*16);
    uint32_t fraction = ( ((fclk*100/(baudrate*16) ) ) %  (mantissa * 100) ) * 16;
    
    //std::cout << fraction;
    fraction = isCarry(fraction);
    
    if(fraction > 15)
    {
        mantissa += 1;
        fraction = 0;
    }
    //std::cout << fraction;
    buf[0] = mantissa;
    buf[1] = fraction;
    return buf;
}

void DriverArmCotexM4::DriverUART::uart_init(struct uart *uart, unsigned long baud) {
  // datasheet DS_stm32f401re page 46 Table 9. Alternate function mapping (continued
  uint8_t AF7 = 7;           // Alternate function
  uint8_t AF8 = 8;           // Alternate function
  uint16_t rx = 0, tx = 0;  // pins

  if (uart == UART1) RCC->APB2ENR |= BIT(4);
  if (uart == UART2) RCC->APB1ENR |= BIT(17);
  if (uart == UART6) RCC->APB2ENR |= BIT(5);

  if (uart == UART1) tx = PIN('A', 9), rx = PIN('A', 10);
  if (uart == UART2) tx = PIN('A', 2), rx = PIN('A', 3);
  if (uart == UART6) tx = PIN('A', 11), rx = PIN('A', 12);

  GPIO::gpio_set_mode(tx, AF);
  GPIO::gpio_set_mode(rx, AF);
  if (uart == UART1 || uart == UART2) {
    GPIO::gpio_set_af(tx, AF7);
    GPIO::gpio_set_af(rx, AF7);
  }
  if (uart == UART6) {
    GPIO::gpio_set_af(tx, AF8);
    GPIO::gpio_set_af(rx, AF8);
  }

  uart->CR1 = 0;                           // Disable this UART

  uint32_t buf[2];
  calBaud(buf, baud, FREQ);
  uart->BRR = buf[0] << 4 | buf[1];  // Set baud rate
  uart->CR1 |= BIT(13) | BIT(2) | BIT(3);  // Set UE, RE, TE
  
  if (uart == UART1) NVIC_Enable(POSITION_USART1, 15);
  if (uart == UART2) NVIC_Enable(POSITION_USART2, 14);

}

void DriverArmCotexM4::DriverUART::uart_dma2_init(struct uart *uart, uint32_t source, uint16_t length)
{
  RCC->AHB1ENR |= BIT(22);  // Enable DMA2 clock
  DMA2->STREAM[7].CR &= ~BIT(0);  // Disable DMA2 Stream 7
  while (DMA2->STREAM[7].CR & BIT(0)) spin(1);  // Wait until DMA2 Stream 7 is disabled
  DMA2->HIFCR = BIT(27); 
  DMA2->HIFCR = BIT(26);
  DMA2->HIFCR = BIT(25); 
  DMA2->HIFCR = BIT(24);
  DMA2->HIFCR = BIT(22);

  DMA2->STREAM[7].PAR = (uint32_t) &uart->DR;  // Peripheral address
  DMA2->STREAM[7].M0AR = source;  // Memory address
  DMA2->STREAM[7].NDTR = length;  // Number of data items

  DMA2->STREAM[7].CR |= BIT(27) ;
  DMA2->STREAM[7].CR |= BIT(10) ;
  DMA2->STREAM[7].CR |= BIT(6) ;
  DMA2->STREAM[7].CR |= BIT(4) ;
  DMA2->STREAM[7].FCR = 0x00UL;  // Disable FIFO

  DMA2->STREAM[7].CR |= BIT(0);
  uart->CR3 |= BIT(7);  // Enable DMA for transmission

  NVIC_Enable(POSITION_DMA2_Stream7, 13);
}

void DriverArmCotexM4::DriverUART::uart_dma2_reTransmit(struct uart *uart, uint32_t source, uint16_t length)
{
  DMA2->STREAM[7].CR &= ~BIT(0);  // Disable DMA2 Stream 7
  while (DMA2->STREAM[7].CR & BIT(0)) spin(1);  // Wait until DMA2 Stream 7 is disabled
  DMA2->HIFCR = BIT(27);  // Clear TCIF7
  DMA2->STREAM[7].PAR = (uint32_t) &uart->DR;  // Peripheral address
  DMA2->STREAM[7].M0AR = source;  // Memory address
  DMA2->STREAM[7].NDTR = length;  // Number of data items
  DMA2->STREAM[7].CR |= BIT(0);
}

void DriverArmCotexM4::DriverUART::uart_write_byte_for_interrupt(struct uart *uart, uint8_t byte) {
  uart->DR = byte; spin(5);
}

void DriverArmCotexM4::DriverUART::uart_write_byte_for_polling(struct uart *uart, uint8_t byte) {
  uart->DR = byte;
 while (uart_write_complete(uart) == 0) spin(1);
}

void DriverArmCotexM4::DriverUART::uart_write_buf(struct uart *uart, char *buf, uint8_t len) {
  while (len-- > 0) uart_write_byte_for_polling(uart, *(uint8_t *) buf++);
}


bool DriverArmCotexM4::DriverUART::uart_read_ready(struct uart *uart) {
  return uart->SR & BIT(5);  // If RXNE bit is set, data is ready
}

bool DriverArmCotexM4::DriverUART::uart_write_complete(struct uart *uart) {
  return uart->SR & BIT(6);  // If TC bit is set, transmission is complete
}


bool DriverArmCotexM4::DriverUART::uart_write_ready(struct uart *uart) {
  return uart->SR & BIT(7);  // If TXE bit is set, UART is ready to transmit
}

uint8_t DriverArmCotexM4::DriverUART::uart_read_byte(struct uart *uart) {
  return (uint8_t) (uart->DR & 255);
}

void DriverArmCotexM4::DriverUART::disable_TXEIE(struct uart *uart) 
{
    uart->CR1 &= ~BIT(7);  // Disable TXEIE Bit
}
void DriverArmCotexM4::DriverUART::enable_TXEIE(struct uart *uart) 
{
  if( ( uart->CR1 & BIT(7) ) == 0)  // Check if TXEIE Bit is not already set
  {
    uart->CR1 |= BIT(7);  // Enable TXEIE Bit
  }
}

void DriverArmCotexM4::DriverUART::disable_RXEIE(struct uart *uart) 
{
    uart->CR1 &= ~BIT(5);  // Disable RXEIE Bit
}
void DriverArmCotexM4::DriverUART::enable_RXEIE(struct uart *uart) 
{
    uart->CR1 |= BIT(5);  // Enable RXEIE Bit
}