#ifndef CAN_DRIVER_H_
#define CAN_DRIVER_H_

#include "stm32f4xx.h"
#include "stm32f446xx.h"
#include <stdint.h>
#include <stdbool.h>

/* CAN PERIPHERAL MEMORY MAP STRUCTURE (for easy navigation between registers)
//Controller Area Network TxMailBox
typedef struct
{
  __IO uint32_t TIR;  // CAN TX mailbox identifier register
  __IO uint32_t TDTR; // CAN mailbox data length control and time stamp register
  __IO uint32_t TDLR; // CAN mailbox data low register
  __IO uint32_t TDHR; // CAN mailbox data high register
} CAN_TxMailBox_TypeDef;

//Controller Area Network FIFOMailBox
typedef struct
{
  __IO uint32_t RIR;  // CAN receive FIFO mailbox identifier register
  __IO uint32_t RDTR; // CAN receive FIFO mailbox data length control and time stamp register
  __IO uint32_t RDLR; // CAN receive FIFO mailbox data low register
  __IO uint32_t RDHR; // CAN receive FIFO mailbox data high register
} CAN_FIFOMailBox_TypeDef;

//Controller Area Network FilterRegister
typedef struct
{
  __IO uint32_t FR1; // CAN Filter bank register 1
  __IO uint32_t FR2; // CAN Filter bank register 1
} CAN_FilterRegister_TypeDef;

typedef struct
{
  __IO uint32_t              MCR;                 // CAN master control register,         Address offset: 0x00
  __IO uint32_t              MSR;                 // CAN master status register,          Address offset: 0x04
  __IO uint32_t              TSR;                 // CAN transmit status register,        Address offset: 0x08
  __IO uint32_t              RF0R;                // CAN receive FIFO 0 register,         Address offset: 0x0C
  __IO uint32_t              RF1R;                // CAN receive FIFO 1 register,         Address offset: 0x10
  __IO uint32_t              IER;                 // CAN interrupt enable register,       Address offset: 0x14
  __IO uint32_t              ESR;                 // CAN error status register,           Address offset: 0x18
  __IO uint32_t              BTR;                 // CAN bit timing register,             Address offset: 0x1C
  uint32_t                   RESERVED0[88];       // Reserved, 0x020 - 0x17F
  CAN_TxMailBox_TypeDef      sTxMailBox[3];       // CAN Tx MailBox,                      Address offset: 0x180 - 0x1AC
  CAN_FIFOMailBox_TypeDef    sFIFOMailBox[2];     // CAN FIFO MailBox,                    Address offset: 0x1B0 - 0x1CC
  uint32_t                   RESERVED1[12];       // Reserved, 0x1D0 - 0x1FF
  __IO uint32_t              FMR;                 // CAN filter master register,          Address offset: 0x200
  __IO uint32_t              FM1R;                // CAN filter mode register,            Address offset: 0x204
  uint32_t                   RESERVED2;           // Reserved, 0x208
  __IO uint32_t              FS1R;                // CAN filter scale register,           Address offset: 0x20C
  uint32_t                   RESERVED3;           // Reserved, 0x210
  __IO uint32_t              FFA1R;               // CAN filter FIFO assignment register, Address offset: 0x214
  uint32_t                   RESERVED4;           // Reserved, 0x218
  __IO uint32_t              FA1R;                // CAN filter activation register,      Address offset: 0x21C
  uint32_t                   RESERVED5[8];        // Reserved, 0x220-0x23F
  CAN_FilterRegister_TypeDef sFilterRegister[28]; // CAN Filter Register,                 Address offset: 0x240-0x31C
} CAN_TypeDef;
*/


//#define CAN_MODE_LOOPBACK  0x00
//#define CAN_MODE_NORMAL  0x01
//#define CAN_ID_STD		0x00
//#define CAN_ID_EXT		0x04
//#define CAN_RX_FIFO0	0x00
//#define CAN_RX_FIFO1	0x01

#define CAN_NUM_OF_FILTERS_ASSIGNED_TO_CAN1	20    //must be max 27 because  28 filterbank registers 0->27

/*
Baud Rate (Data Rate),Maximum Safe Cable Length,Common Application Areas
1 Mbps (1000 kbps),40 Meters,"In-vehicle communication (High-Speed CAN), Drone/UAV internal communication (UAVCAN), Short-distance robotic systems"
800 kbps,50 Meters,"Industrial automation, short-range control networks"
500 kbps,100 Meters,"Automotive diagnostic networks (OBD-II), Heavy-duty vehicle networks (SAE J1939), Factory automation"
250 kbps,250 Meters,"Marine electronics (NMEA 2000), Agricultural machinery (ISOBUS), Industrial CANopen networks"
125 kbps,500 Meters,"Long-distance industrial facilities, building automation"
50 kbps,1000 Meters (1 km),"Large-scale infrastructure projects, mining operations"
20 kbps / 10 kbps,1200 - 5000 Meters,"Very long-distance telemetry links (Low speed, high noise tolerance)"
*/
typedef enum {
		CAN_BAUD_1000K = 1000,
	    CAN_BAUD_800K  = 800,
	    CAN_BAUD_500K  = 500,
	    CAN_BAUD_250K  = 250,
	    CAN_BAUD_125K  = 125,
	    CAN_BAUD_100K  = 100,
	    CAN_BAUD_95K   = 95,    // 95.238 kbps	CiA  CAN in Automation
	    CAN_BAUD_83K   = 83,    // 83.333 kbps	Otomotive SW-CAN
	    CAN_BAUD_50K   = 50,
	    CAN_BAUD_33K   = 33,    // 33.333 kbps Low-Speed Otomotive
	    CAN_BAUD_20K   = 20,
	    CAN_BAUD_10K   = 10
} can_baudrate_t;

typedef enum
{
	CAN_TRANS_MODE_LOOPBACK = 0x00,
	CAN_TRANS_MODE_NORMAL = 0x01,
}can_trans_mode_sel;

typedef enum
{
	CAN_ID_STD = 0x00,
	CAN_ID_EXT = 0x01,
}can_id_style_sel;

typedef enum
{
	CAN_FRAME_TYPE_DATA = 0x00,
	CAN_FRAME_TYPE_REQUEST = 0x01,
}can_frame_type_sel;

typedef enum
{
	CAN_FIFO_0 = 0x00,
	CAN_FIFO_1 = 0x01,
}can_fifo_sel;

typedef enum
{
	CAN_FILTER_MODE_MASK = 0,
	CAN_FILTER_MODE_LIST = 1
}can_filter_mode_sel;

typedef enum
{
	CAN_FILTER_SCALING_TWO16BIT = 0,
	CAN_FILTER_SCALING_SINGLE32BIT = 1
}can_filter_scaling_sel;


// U32RegAccessBytes definition is used to map U32 registers(RegH and RegL) to 8bytes long data[] array
typedef union {
    struct {
        uint32_t L32; // Takes bytes 0-3
        uint32_t H32; // Takes bytes 4-7
    }Reg;
    uint8_t bytes[8];      // Overlaps bytes 0-7
    uint16_t words[4];		// Overlaps
}U32RegAccessBytes_t;


typedef struct
{
	uint32_t id; //it can hold a standard_id or an extended_id
	uint32_t ide; // it holds  ide bit  that specifies id style (1: ext_id , 0: std_id)
	uint32_t rtr; // it holds rtr bit  that specifies frame type (1:request frame, 0:data frame)
	uint32_t dlc; // it holds the number of bytes in the data section of the frame
	uint8_t transmit_global_time; //it holds tgt bit that specifies global time is used or not
}can_tx_header_typedef;


typedef struct
{
	uint32_t id;   //Specifies the standard identifier or extended identifier
  //uint32_t std_id;
						/*!< Specifies the standard identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF. */

  //uint32_t ext_id;
                       /*!< Specifies the extended identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF. */

  uint32_t ide;      /*!< Specifies the type of identifier for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_identifier_type */

  uint32_t rtr;      /*!< Specifies the type of frame for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_remote_transmission_request */

  uint32_t dlc;      /*!< Specifies the length of the frame that will be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 8. */

  uint32_t timestamp; /*!< Specifies the timestamp counter value captured on start of frame reception.
                          @note: Time Triggered Communication Mode must be enabled.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFFFF. */

  uint32_t filter_match_index; /*!< Specifies the index of matching acceptance filter element.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF. */

} can_rx_header_typedef;




typedef struct {
	uint32_t id;
	uint32_t ide; 	//1:extd_id  or 0: std_id      		   (if this obj is a mask then ide must be 1)
	uint32_t rtr;	//1:request frame or 0:data frame      (if this obj is a mask then rtr must be 1)
	can_id_style_sel id_style; // in mask mode, ide bit must be 1 for ide bit mask even if we use std_id , so this id_style specifies we use std_id or ext_id
}can_id_mask_handle_t;

typedef struct {
	uint32_t				filterbank_number;
	can_filter_scaling_sel 	filterbank_scaling; //can_filter_scaling_sel
	can_filter_mode_sel		filterbank_mask_or_list_mode;//can_filter_mode_sel
	can_fifo_sel 			filterbank_fifo;//can_fifo_sel
	can_id_mask_handle_t	id_mask_arr[4];
}can_filter_config_t;


typedef struct {
    uint32_t BRP;   //10bit:  0 to 1023
    uint32_t TS1;   //4bit:   0 to 15
    uint32_t TS2;   //3bit:   0 to 7
    uint32_t SJW;   //2bit:   0 to 3 Resynchronization Jump Width (default 1)
} can_bit_timing_t;







void can_gpio_init(void);
void can_params_init(CAN_TypeDef *CANx, can_trans_mode_sel mode, uint16_t baudrate);//(can_trans_mode_sel, can_baudrate_t)
bool can_get_standard_timing(uint32_t pclk1_hz, can_baudrate_t baudrate, can_bit_timing_t *timing);
void can_start(CAN_TypeDef *CANx, can_fifo_sel selected_fifo); //(can_fifo_sel)
uint8_t can_add_tx_message(CAN_TypeDef *CANx, can_tx_header_typedef *pHeader, uint8_t aData[], uint32_t *pTxMailbox);
uint8_t can_get_rx_message(CAN_TypeDef *CANx, can_fifo_sel RxFifo, can_rx_header_typedef *pHeader, uint8_t aData[]);
int can_filter_config(CAN_TypeDef *CANx, can_filter_config_t* filtercfg);


#endif
