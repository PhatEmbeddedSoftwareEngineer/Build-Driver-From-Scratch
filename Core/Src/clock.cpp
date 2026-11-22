#include "clock.h"

CLOCK_TREE_DRIVER::ClockSource& CLOCK_TREE_DRIVER::ClockSource::instance()
{
    static ClockSource object;
    return object;
}

void CLOCK_TREE_DRIVER::ClockSource::clocking_HSI()
{
    RCC->CR |= BIT(0); // HSI ON
    while( ! ( RCC->CR & BIT(1) ) ) ;

    // Flash wait state 
    /*
    HCLK	        LATENCY
    0 - 30 MHz	    0 WS
    30 - 60 MHz	    1 WS
    60 - 84 MHz	    2 WS
    */

    FLASH->flash_acr |= BIT(2); // latency 2ws
    FLASH->flash_acr |= BIT(8); // Prefetch enable
    FLASH->flash_acr |= BIT(9); // Instruction cache enable
    FLASH->flash_acr |= BIT(10); // Data cache enable


    // configure PLL
    RCC->PLLCFGR  = 0x00000000; 
    RCC->PLLCFGR |= BIT(3); // PLLM = 8
    RCC->PLLCFGR |= (168 << 6); // PLLN = 168
    RCC->PLLCFGR |= BIT(16) ; // PLLP = 4
    RCC->PLLCFGR &= ~ BIT(22) ; // PLLSRC HSI

    // enable PLL
    RCC->CR |= BIT(24); // PLL ON
    while(! (RCC->CR & BIT(25))); // PLLRDY

    RCC->CFGR = (RCC->CFGR & ~(7UL << 10)) | (4UL << 10); // APB1 = HCLK / 2
    // Switch system clock to PLL
    RCC->CFGR |= BIT(1); //  PLL selected as system clock
    while(! (RCC->CFGR & BIT(3))); // System clock switch status PLL

}