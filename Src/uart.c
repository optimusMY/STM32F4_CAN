#include <stdint.h>
#include "uart.h"

static void uart2_write(int ch);


//this function binds  uart2 to printf
int __io_putchar(int ch)
{
	uart2_write(ch);
	return ch;
}

void usart2_init(uint32_t baudrate)
{
	/*Enable clock access to GPIOA*/
	//RCC->AHB1ENR |= GPIOAEN;
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);

	/*Set the mode of PA2 (USART2TX)to alternate function mode*/
	//PA2 --> GPIOA->MODER_MODER2 = 0b10 (ALT_FUNC)
	//GPIOA->MODER &=~(1U<<4);//MODER2_L =0
	//GPIOA->MODER |=(1U<<5);//MODER2_H =1
	WRITE_REG_PORTION(GPIOA->MODER, GPIO_MODER_MODER2_Msk, GPIO_MODER_MODER2_Pos, GPIO_MODER_MODESEL_ALTFUNC);
	/*Set the mode of PA3 (USART2RX) to alternate function mode*/
	WRITE_REG_PORTION(GPIOA->MODER, GPIO_MODER_MODER3_Msk, GPIO_MODER_MODER3_Pos, GPIO_MODER_MODESEL_ALTFUNC);

	/*Set alternate function type to AF7(UART2_TX)*/
	//AFR[0] means AFRL  , AFR[1] means AFRH
	//GPIOA->AFR[0] |=(1U<<8);
	//GPIOA->AFR[0] |=(1U<<9);
	//GPIOA->AFR[0] |=(1U<<10);
	//GPIOA->AFR[0] &=~(1U<<11);
    // AFR (Alternate Function Register - AF7)
    WRITE_REG_PORTION(GPIOA->AFR[0], GPIO_AFRL_AFSEL2_Msk, GPIO_AFRL_AFSEL2_Pos, GPIO_AFR_AFSEL_AF7);
    WRITE_REG_PORTION(GPIOA->AFR[0], GPIO_AFRL_AFSEL3_Msk, GPIO_AFRL_AFSEL3_Pos, GPIO_AFR_AFSEL_AF7);


	/*Enable clock access to UART2*/
     //RCC->APB1ENR |=	UART2EN;
     SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2EN);

	/*Configure uart baudrate*/
     USART2->BRR = (uint16_t)((APB1_freqHz + (baudrate/2U))/baudrate);


	/*Configure transfer direction*/
     USART2->CR1 = USART_CR1_TE_Msk;

	/*Enable UART Module*/
     USART2->CR1 |= USART_CR1_UE_Msk;

     //TODO RX en and IRQ en will be added
}



static void uart2_write(int ch)
{
	/*Make sure transmit data register is empty*/
	while(!(USART2->SR & USART_SR_TXE_Msk)){}

	/*Write to transmit data register*/
	USART2->DR =(ch & 0xFF);
}

