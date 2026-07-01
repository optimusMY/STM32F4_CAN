#ifndef __UART_H__
#define __UART_H__

#include "stm32f4xx.h"
#include "main.h"

#define DBG_UART_BAUDRATE		115200


void usart2_init(uint32_t baudrate);

#endif
