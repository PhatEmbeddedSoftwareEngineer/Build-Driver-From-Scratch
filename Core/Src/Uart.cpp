#include "Uart.h"

volatile uint32_t DriverArmCotexM4::s_ticks = 0;
char DriverArmCotexM4::message[] = "Hello, Phat\n";
int DriverArmCotexM4::leng = sizeof(message) - 1;

uint8_t DriverArmCotexM4::DriverUART::s_uart1_tx_buf[UART1_TX_BUF_SIZE] = {0};
uint8_t DriverArmCotexM4::DriverUART::s_uart1_rx_buf[UART1_RX_BUF_SIZE] = {0};
uint8_t DriverArmCotexM4::DriverUART::s_uart1_dmarx_buf[UART1_DMA_RX_BUF_SIZE] = {0};
uint8_t DriverArmCotexM4::DriverUART::s_uart1_dmatx_buf[UART1_DMA_TX_BUF_SIZE] = {0};

static inline void spin(volatile uint32_t count) {
  while (count--) asm("nop");
}

static const DriverArmCotexM4::uart_config_t uart1_config = {
    .port = PORT_UART1,
    .baudrate = 115200
};

static const DriverArmCotexM4::uart_config_t uart2_config = {
    .port = PORT_UART2,
    .baudrate = 115200
};

static const DriverArmCotexM4::uart_config_t uart6_config = {
    .port = PORT_UART6,
    .baudrate = 115200
};

static DriverArmCotexM4::uart_state_t uart1_state = {0};
static DriverArmCotexM4::uart_state_t uart2_state = {0};
static DriverArmCotexM4::uart_state_t uart6_state = {0};

static const DriverArmCotexM4::devfs_handle_t uart1_handle = {
    .config = &uart1_config,
    .state = &uart1_state
};

static const DriverArmCotexM4::devfs_handle_t uart2_handle = {
    .config = &uart2_config,
    .state = &uart2_state
};

static const DriverArmCotexM4::devfs_handle_t uart6_handle = {
    .config = &uart6_config,
    .state = &uart6_state
};


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

  int result ;
  if (uart == UART1) {
      result = uart_local_open(&uart1_handle);
      if (result != 0) {
          return;
      }
  }
  else if (uart == UART2) {
      result = uart_local_open(&uart2_handle);
      if (result != 0) {
          return;
      }
  } else if (uart == UART6) {
      result = uart_local_open(&uart6_handle);
      if (result != 0) {
          return;
      }
  } else {}

  //if (uart == UART1) RCC->APB2ENR |= BIT(4);
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
  
  //if (uart == UART1) NVIC_Enable(POSITION_USART1, 15);
  //if (uart == UART2) NVIC_Enable(POSITION_USART2, 14);

}

void DriverArmCotexM4::DriverUART::uart_dma2_init_tx(struct uart *uart)
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
  DMA2->STREAM[7].M0AR = 0;
  DMA2->STREAM[7].NDTR = 0;

  DMA2->STREAM[7].CR |= BIT(27) ;
  DMA2->STREAM[7].CR &= ~ BIT(17) ; // priority level: 01 = Medium
  DMA2->STREAM[7].CR |= BIT(16) ;

  DMA2->STREAM[7].CR &= ~ BIT(14) ; // Memory data size 00: Byte (8-bit)
  DMA2->STREAM[7].CR &= ~ BIT(13) ;

  DMA2->STREAM[7].CR &= ~ BIT(12) ; // Peripheral data size 00: Byte (8-bit)
  DMA2->STREAM[7].CR &= ~ BIT(11) ;

  DMA2->STREAM[7].CR |= BIT(10) ; //  Memory increment mode enable
  DMA2->STREAM[7].CR &= ~ BIT(9) ; // Peripheral increment mode disable

  DMA2->STREAM[7].CR &= ~ BIT(7) ; // direction: Memory-to-peripheral
  DMA2->STREAM[7].CR |= BIT(6) ;
  DMA2->STREAM[7].CR |= BIT(4) ;
  DMA2->STREAM[7].FCR = 0x00UL;  // Disable FIFO

  uart->CR3 |= BIT(7);  // Enable DMA for transmission

  NVIC_Enable(POSITION_DMA2_Stream7, 13);

  if (uart == UART1) {
    uint8_t uart_id = PORT_UART1;
    fifo.fifo_register(&s_uart_dev[uart_id].tx_fifo, &s_uart1_tx_buf[0],
                       sizeof(s_uart1_tx_buf), fifo.fifo_lock, fifo.fifo_unlock);
    s_uart_dev[uart_id].dmatx_buf = &s_uart1_dmatx_buf[0];
    s_uart_dev[uart_id].dmatx_buf_size = sizeof(s_uart1_dmatx_buf);
    s_uart_dev[uart_id].status = 0;
  }
}

void DriverArmCotexM4::DriverUART::uart_dma2_init_rx(struct uart *uart)
{
  RCC->AHB1ENR |= BIT(22);  // Enable DMA2 clock
  DMA2->STREAM[5].CR &= ~BIT(0);  // Disable DMA2 Stream 5
  while (DMA2->STREAM[5].CR & BIT(0)) spin(1);  // Wait until DMA2 Stream 5 is disabled
  DMA2->HIFCR = BIT(11); 
  DMA2->HIFCR = BIT(10);
  DMA2->HIFCR = BIT(9); 
  DMA2->HIFCR = BIT(8);
  DMA2->HIFCR = BIT(6);

  DMA2->STREAM[5].PAR = (uint32_t) &uart->DR;  // Peripheral address

  DMA2->STREAM[5].CR |= BIT(27) ;
  DMA2->STREAM[5].CR &= ~ BIT(17) ; // priority level: 00 = low
  DMA2->STREAM[5].CR &= ~ BIT(16) ;

  DMA2->STREAM[5].CR &= ~ BIT(14) ; // Memory data size 00: Byte (8-bit)
  DMA2->STREAM[5].CR &= ~ BIT(13) ;

  DMA2->STREAM[5].CR &= ~ BIT(12) ; // Peripheral data size 00: Byte (8-bit)
  DMA2->STREAM[5].CR &= ~ BIT(11) ;

  DMA2->STREAM[5].CR |= BIT(10) ; //  Memory increment mode enable
  DMA2->STREAM[5].CR &= ~ BIT(9) ; // Peripheral increment mode disable
  DMA2->STREAM[5].CR |= BIT(8) ; // enable Circular mode
  DMA2->STREAM[5].CR &= ~ BIT(7) ; // direction: 0 = Peripheral to Memory
  DMA2->STREAM[5].CR &= ~ BIT(6) ;
  
  DMA2->STREAM[5].CR |= BIT(4) ;
  DMA2->STREAM[5].CR |= BIT(3) ; //  Half transfer interrupt enable

  DMA2->STREAM[5].FCR = 0x00UL;  // Disable FIFO

  if(uart == UART1) {
    uint8_t uart_id = 0;
    fifo.fifo_register(&s_uart_dev[uart_id].rx_fifo, &s_uart1_rx_buf[0], 
                      sizeof(s_uart1_rx_buf), fifo.fifo_lock, fifo.fifo_unlock);
    s_uart_dev[uart_id].dmarx_buf = &s_uart1_dmarx_buf[0];
    s_uart_dev[uart_id].dmarx_buf_size = sizeof(s_uart1_dmarx_buf);
    DMA2->STREAM[5].M0AR = (uint32_t) s_uart_dev[uart_id].dmarx_buf;  // Memory address
    DMA2->STREAM[5].NDTR = sizeof(s_uart1_dmarx_buf);  // Number of data items
    s_uart_dev[uart_id].status  = 0;
  }

  DMA2->STREAM[5].CR |= BIT(0);
  uart->CR3 |= BIT(6);  // Enable DMA for reception
  uart->CR1 |= BIT(4);  // IDLE interrupt enable

  NVIC_Enable(POSITION_DMA2_Stream5, 12);
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

bool DriverArmCotexM4::DriverUART::uart_read_idle(struct uart *uart) {
  return uart->SR & BIT(4);  // If IDLE bit is set, UART is idle
}

bool DriverArmCotexM4::DriverUART::is_uart_set_idle_isr(struct uart *uart) {
  return uart->CR1 & BIT(4);  // If IDLE bit is set, UART is idle isr is enabled
}

uint32_t DriverArmCotexM4::DriverUART::uart_read_register_status(struct uart *uart)
{
  return uart->SR;
}

uint32_t DriverArmCotexM4::DriverUART::uart_read_register_data(struct uart *uart)
{
  return uart->DR;
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

int DriverArmCotexM4::uart_local_open(const devfs_handle_t *handle)
{
    DEVFS_DRIVER_DECLARE_CONFIG_STATE(uart);
    if (state->ref_count == 0) 
    {
      uart_enable_clock(config->port);
      cortexm_enable_irq(config->port);
    }
    state->ref_count++;
    return 0;
}

int DriverArmCotexM4::uart_enable_clock(uint8_t port)
{
    switch (port) {
    case PORT_UART1:
        RCC->APB2ENR |= BIT(4);
        return 0;

    case PORT_UART2:
        RCC->APB1ENR |= BIT(17);
        return 0;

    case PORT_UART6:
        RCC->APB2ENR |= BIT(5);
        return 0;

    default:
        return -1;
    }
}

int DriverArmCotexM4::cortexm_enable_irq(uint8_t port)
{
  switch (port) {
    case PORT_UART1:
        NVIC_Enable(POSITION_USART1, 15);
        return 0;

    case PORT_UART2:
        NVIC_Enable(POSITION_USART2, 14);
        return 0;

    case PORT_UART6:
        NVIC_Enable(POSITION_USART6, 13);
        return 0;

    default:
        return -1;
  }
}

uint16_t DriverArmCotexM4::DriverUART::bsp_uart1_get_dma2_stream5_rx_buf_remain_size(void)
{
	return (uint16_t)DMA2->STREAM[5].NDTR;
}

uint16_t DriverArmCotexM4::DriverUART::uart_write(uint8_t uart_id, const uint8_t *buf, uint16_t size)
{
	return fifo.fifo_write(&s_uart_dev[uart_id].tx_fifo, buf, size);
}

uint16_t DriverArmCotexM4::DriverUART::uart_read(uint8_t uart_id, uint8_t *buf, uint16_t size)
{
	return fifo.fifo_read(&s_uart_dev[uart_id].rx_fifo, buf, size);
}

void DriverArmCotexM4::DriverUART::uart_dmarx_done_isr(uint8_t uart_id)
{
  uint16_t recv_size;
  recv_size = s_uart_dev[uart_id].dmarx_buf_size - s_uart_dev[uart_id].last_dmarx_size;
  s_UartTxRxCount[uart_id*2+1] += recv_size;
  fifo.fifo_write(&s_uart_dev[uart_id].rx_fifo, 
				   (const uint8_t *)&(s_uart_dev[uart_id].dmarx_buf[s_uart_dev[uart_id].last_dmarx_size]), recv_size);
  s_uart_dev[uart_id].last_dmarx_size = 0;
}

void DriverArmCotexM4::DriverUART::uart_dmarx_half_done_isr(uint8_t uart_id)
{
  uint16_t recv_total_size;
  uint16_t recv_size;

  recv_total_size = s_uart_dev[uart_id].dmarx_buf_size - uart1_get_dma2rx_remain_size();
  recv_size = recv_total_size - s_uart_dev[uart_id].last_dmarx_size;
  s_UartTxRxCount[uart_id*2+1] += recv_size;

  fifo.fifo_write(&s_uart_dev[uart_id].rx_fifo, 
				   (const uint8_t *)&(s_uart_dev[uart_id].dmarx_buf[s_uart_dev[uart_id].last_dmarx_size]), recv_size);
  s_uart_dev[uart_id].last_dmarx_size = recv_total_size;
}

uint16_t DriverArmCotexM4::DriverUART::uart1_get_dma2rx_remain_size(void)
{
  return (uint16_t)DMA2->STREAM[5].NDTR;
}

void DriverArmCotexM4::DriverUART::uart_dmarx_idle_isr(uint8_t uart_id) 
{
  uint16_t recv_total_size;
  uint16_t recv_size;

  recv_total_size = s_uart_dev[uart_id].dmarx_buf_size - uart1_get_dma2rx_remain_size();
  recv_size = recv_total_size - s_uart_dev[uart_id].last_dmarx_size;
  if(recv_size == 0) return;

  s_UartTxRxCount[uart_id*2+1] += recv_size;
  fifo.fifo_write(&s_uart_dev[uart_id].rx_fifo, 
				   (const uint8_t *)&(s_uart_dev[uart_id].dmarx_buf[s_uart_dev[uart_id].last_dmarx_size]), recv_size);
  s_uart_dev[uart_id].last_dmarx_size = recv_total_size;
}

void DriverArmCotexM4::DriverUART::uart_dmatx_done_isr(uint8_t uart_id)
{
  s_uart_dev[uart_id].status = 0;
  uart_poll_dma_tx(uart_id);
}

void DriverArmCotexM4::DriverUART::uart_poll_dma_tx(uint8_t uart_id)
{
  uint16_t size = 0;
  if (0x01 == s_uart_dev[uart_id].status) return;
  size = fifo.fifo_read(&s_uart_dev[uart_id].tx_fifo, s_uart_dev[uart_id].dmatx_buf,
					 s_uart_dev[uart_id].dmatx_buf_size);
  
  if (size != 0)
  {
    s_UartTxRxCount[uart_id*2+0] += size;
    s_uart_dev[uart_id].status = 0x01;
    uart_dma2_reTransmit(UART1,(uint32_t)s_uart_dev[uart_id].dmatx_buf, size);
  }
}