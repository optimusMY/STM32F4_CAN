#include "stm32f4xx.h"
#include "main.h"
#include "uart.h"
#include "can_driver.h"


#define RX_DATA_STD_ID  0x244

uint8_t count = 0;
uint8_t message_buff[20];

uint8_t can1_rx_data[8]={};
uint8_t can1_tx_data[8]={};
uint32_t can1_tx_mailbox[3];
can_rx_header_typedef can1_rx_header;
can_tx_header_typedef can1_tx_header;
volatile uint8_t count1=1;

void CAN1_RX0_IRQHandler(void)
{
	if((CAN1->RF0R & CAN_RF0R_FMP0) != 0U)
	{
		can_get_rx_message(CAN1, CAN_FIFO_0, &can1_rx_header, can1_rx_data);
		//sprintf((char *)message_buff, "New Data : %d",rx_data);
		//printf("%s\n\r",message_buff);

		//print id
		U32RegAccessBytes_t tmp ={.Reg={.L32=can1_rx_header.id,.H32=0}};
		for(volatile int i=0; i<4; i++){ printf("%02x", tmp.bytes[3-i]); }
		//print dlc
		printf("   %lu   ", can1_rx_header.dlc);
		//print data[i]
		for(volatile int i=0; i<can1_rx_header.dlc; i++)
		{ printf("%02x  ", can1_rx_data[i]);}
		//seperate next msg by newline
		printf("\r\n\r\n");
		count1++;
	}
}


void CAN1_RX1_IRQHandler(void)
{
	if((CAN1->RF1R & CAN_RF1R_FMP1) != 0U)//CAN receive FIFO 1 register (RF1R) ->FMP1 bits are
	{
		can_get_rx_message(CAN1, CAN_FIFO_1, &can1_rx_header, can1_rx_data);
		count1++;
	}
}





/*
uint8_t can2_rx_data[8]={};
uint8_t can2_tx_data[8]={};
uint32_t can2_tx_mailbox[3];
can_rx_header_typedef can2_rx_header;
can_tx_header_typedef can2_tx_header;
volatile uint8_t count2;

void CAN2_RX0_IRQHandler(void)
{
	if((CAN2->RF0R & CAN_RF0R_FMP0) != 0U)
	{
		can_get_rx_message(CAN2, CAN_FIFO_0, &can2_rx_header, can2_rx_data);
		count2++;
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

	//init usart2 for debug using printf
	usart2_init(DBG_UART_BAUDRATE);


	//create a filter to receive only frames of specified id
	can_filter_config_t filcfg ={
			.filterbank_fifo = CAN_FIFO_0,
			.filterbank_mask_or_list_mode = CAN_FILTER_MODE_MASK,
			.filterbank_scaling = CAN_FILTER_SCALING_SINGLE32BIT,
			.filterbank_number = 10, //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!IMPORTANT RIGHT REGION FOR CAN1!!!!!!!!!!!!!!!!!!
			.id_mask_arr={
					[0] = {//id
							.id = RX_DATA_STD_ID,	/*!!!!!!!! ACCEPT FRAME FROM THIS ID !!!!!!!!*/
							.ide = CAN_IDE_STDID,
							.rtr= CAN_RTR_DATA_FRAME,
							.id_style = CAN_ID_STD
							},
					[1] = {//mask
							.id = CAN_STDID_MASK, //all bits of id must match
							.ide = CAN_IDE_MASK, //ide bit must match
							.rtr= CAN_RTR_MASK, //rtr bit must match
							.id_style = CAN_ID_STD
							},
					[2] = {},
					[3] = {}
			}
	};

	can_gpio_init(CAN1);

	can_params_init(CAN1, CAN_TRANS_MODE_NORMAL, CAN_BAUD_125K);


	//int errflag;//debug using errflag, live expressions
	if(0 == can_filter_config(CAN1, &filcfg))
	{
		printf("CAN1 filter config successful!\r\n");
		//errflag=0;
	}
	else
	{
		printf("CAN1 filter config failed!\r\n");
		//errflag=1;
	}

	//can_filter_config(CAN1, &filcfg);
	can_start(CAN1, CAN_FIFO_0);

	printf("CAN1 init: RX, Normal Mode, 125kbps...\r\n");
	printf("------- CAN Read (0x244 ID Frames Accepted)----------\r\n");
	printf("ID   DLC   DATA\r\n");

	while(1)
	{


		/*
		count1++;
		can1_tx_header.id =  0x244; // ID of This Device
		can1_tx_header.ide = CAN_ID_STD;
		can1_tx_header.rtr =  CAN_FRAME_TYPE_DATA;
		can1_tx_header.transmit_global_time = 0;
		can1_tx_header.dlc = 5;//num of bytes to be written in data section of tx can frame   aka data len [bytes]
		can1_tx_data[0] = count1;
		can1_tx_data[1] = count1;
		can1_tx_data[2] = count1;
		can1_tx_data[3] = count1;
		can1_tx_data[4] = count1;

		can_add_tx_message(CAN1, &can1_tx_header, &can1_tx_data[0], can1_tx_mailbox);
		 */

		//some delay (approx 10 sec  @ HSI 16MHz)
		for(volatile int i=0; i<10000000; i++){}
		printf("ID   DLC   DATA\r\n");

	}

}
