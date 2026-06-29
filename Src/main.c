#include "stm32f4xx.h"
#include "can_driver.h"
//#include "uart.h"
//#include "bsp.h"
//#include "fpu.h"
//#include "adc.h"
//#include "timebase.h"
#include <stdint.h>

#define  GPIOAEN		(1U<<0)
#define  PIN5			(1U<<5)
#define  LED_PIN		PIN5


uint8_t can1_rx_data[8]={};
uint8_t can1_tx_data[8]={};

uint32_t can1_tx_mailbox[3];

can_rx_header_typedef can1_rx_header;
can_tx_header_typedef can1_tx_header;

volatile uint8_t count;


void CAN1_RX0_IRQHandler(void)
{
	if((CAN1->RF0R & CAN_RF0R_FMP0) != 0U)
	{
		can_get_rx_message(CAN1, CAN_FIFO_0, &can1_rx_header, can1_rx_data);
		count++;
	}
}

void CAN1_RX1_IRQHandler(void)
{
	if((CAN1->RF1R & CAN_RF1R_FMP1) != 0U)//CAN receive FIFO 1 register (RF1R) ->FMP1 bits are
	{
		can_get_rx_message(CAN1, CAN_FIFO_1, &can1_rx_header, can1_rx_data);
		count++;
	}
}
/*
void CAN2_RX0_IRQHandler(void)
{
	if((CAN2->RF0R & CAN_RF0R_FMP0) != 0U)
	{
		can_get_rx_message(CAN2, CAN_FIFO_0, &can2_rx_header, can2_rx_data); // can_get_rx_message'ı da jenerik yapmalısın
		count++;
	}
}

void CAN2_RX1_IRQHandler(void)
{
	if((CAN2->RF1R & CAN_RF1R_FMP1) != 0U)//CAN receive FIFO 1 register (RF1R) ->FMP1 bits are
	{
		can_get_rx_message(CAN2, CAN_FIFO_1, &can2_rx_header, can2_rx_data);
		count++;
	}
}
*/



int main()
{

	can_filter_config_t filcfg ={
			.filterbank_fifo = CAN_FIFO_0,
			.filterbank_mask_or_list_mode = CAN_FILTER_MODE_MASK,
			.filterbank_scaling = CAN_FILTER_SCALING_SINGLE32BIT,
			.filterbank_number = 15,
			.id_mask_arr={
					[0] = {//id
							.id = 0x244,
							.ide =0,
							.rtr=0,
							.id_style = CAN_ID_STD
							},
					[1] = {//mask
							.id = 0x7FF, //all bits of id must match
							.ide =1, //ide bit must match
							.rtr=1, //rtr bit must match
							.id_style = CAN_ID_STD
							},
					[2] = {},
					[3] = {}
			}
	};

	can_gpio_init();

	can_params_init(CAN1, CAN_TRANS_MODE_LOOPBACK, CAN_BAUD_500K);

	/*//debug using errflag, live expressions
	int errflag;
	if(0 == can_filter_config(&filcfg))
	{
		//printf("filter config successful!\r\n");
		errflag=0;
	}
	else
	{
		//printf("filter config failed!\r\n");
		errflag=1;
	}*/
	can_filter_config(CAN1, &filcfg);
	can_start(CAN1, CAN_FIFO_0);// Eger sadece belirli bir FIFO kesmesini acacaksan: can_fifo_interrupt_enable(CAN_FIFO_0); seklinde func yaz


	while(1)
	{

		can1_tx_header.id =  0x244;
		can1_tx_header.ide = CAN_ID_STD;
		can1_tx_header.rtr =  CAN_FRAME_TYPE_DATA;
		can1_tx_header.transmit_global_time = 0;
		can1_tx_header.dlc = 3;//num of bytes to be written in data section of tx can frame   aka data len [bytes]
		can1_tx_data[0] = count;
		can1_tx_data[1] = count;
		can1_tx_data[2] = count;
		can1_tx_data[3] = count;
		can1_tx_data[4] = count;

		can_add_tx_message(CAN1, &can1_tx_header, &can1_tx_data[0],can1_tx_mailbox);

		//some delay (approx 1 sec  @ HSI 16MHz)
		for(volatile int i=0; i<1000000; i++){}

	}
}
