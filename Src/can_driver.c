#include "can_driver.h"

#define GPIO_MODER_MODESEL_INPUT	(0x0UL)		//00: Input (reset state)
#define GPIO_MODER_MODESEL_OUTPUT	(0x1UL)		//01: General purpose output mode
#define GPIO_MODER_MODESEL_ALTFUNC	(0x2UL)		//10: Alternate function mode
#define GPIO_MODER_MODESEL_ANALOG	(0x3UL)		//11: Analog mode

#define GPIO_AFR_AFSEL_AF0	(0x0UL)		//0b0000: AF0
#define GPIO_AFR_AFSEL_AF9	(0x9UL)		//0b1001: AF9

//BIT_MASK(POS)	(1UL<<POS)
//SET_BIT(REG, BIT)
//CLEAR_BIT(REG, BIT)
#define WRITE_REG_PORTION(REGISTER,PORTION_MASK,PORTION_POS,VAL)	((REGISTER) = (((REGISTER) & (~(PORTION_MASK)))|(((VAL)<<(PORTION_POS))&(PORTION_MASK))))

#define APB1_freqHz		(16000000UL)






// function for designating GPIO pins for CAN BUS
void can_gpio_init(void)
{
	/*Enable clock access to gpiob*/
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN); 		//RCC->AHB1ENR |= GPIOBEN;

	/*Set PB8 and PB9 to alternate function mode*/
	/*
	 Bits 2y:2y+1 MODERy[1:0]: Port x configuration bits (y = 0..15)
		These bits are written by software to configure the I/O direction mode.
		00: Input (reset state)
		01: General purpose output mode
		10: Alternate function mode
		11: Analog mode

		so PB8-> bit16:17 -> 0b10: Alternate function mode
		   PB9-> bit18:19 -> 0b10: Alternate function mode
	 */
	//GPIOB->MODER &=~BIT_MASK(16); //clear MODER_BIT16
	//GPIOB->MODER |=BIT_MASK(17); //set MODER_BIT17
	//GPIO_MODER_MODER8_Pos --> MODER8 Position in GPIO_MODER Register
	//GPIO_MODER_MODER8_Msk --> A value that corresponding MODER8 bits are 1 and remained are 0  in GPIO_MODER Register
	//GPIO_MODER_MODESEL_ALTFUNC---> Value to select the GPIO as ALTFUNCTION
	WRITE_REG_PORTION(GPIOB->MODER, GPIO_MODER_MODER8_Msk, GPIO_MODER_MODER8_Pos, GPIO_MODER_MODESEL_ALTFUNC); //PB8-> bit16:17 -> 0b10: Alternate function mode
	//GPIOB->MODER &=~BIT_MASK(18);
	//GPIOB->MODER |=BIT_MASK(19);
	WRITE_REG_PORTION(GPIOB->MODER, GPIO_MODER_MODER9_Msk, GPIO_MODER_MODER9_Pos, GPIO_MODER_MODESEL_ALTFUNC); //PB9-> bit18:19 -> 0b10: Alternate function mode


	/*Set PB8 and PB9 alternate function to CAN1 RX and TX*/
	// PB8 -> CAN1_RX -> AF9 -----> AFRH.AFRH8[3:0] = AFR[1].AFRH8[3:0] = 0b1001: AF9
	WRITE_REG_PORTION(GPIOB->AFR[1],GPIO_AFRH_AFSEL8_Msk,GPIO_AFRH_AFSEL8_Pos, GPIO_AFR_AFSEL_AF9);		//GPIOB->AFR[1] |=(0x09 << 0);

	// PB9 -> CAN1_TX -> AF9 -----> AFRH.AFRH9[3:0] = AFR[1].AFRH9[3:0] = 0b1001: AF9
	WRITE_REG_PORTION(GPIOB->AFR[1],GPIO_AFRH_AFSEL9_Msk,GPIO_AFRH_AFSEL9_Pos, GPIO_AFR_AFSEL_AF9);		//GPIOB->AFR[1] |=(0x09 << 4);


}



//CAN BUS baudrate and mode selection( loopback or Normal mode)
void can_params_init(CAN_TypeDef *CANx, can_trans_mode_sel mode, uint16_t baudrate)//(can_trans_mode_sel, can_baudrate_t)
{
	/*Enable clock access to CAN1*/
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_CAN1EN);	//RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

	if(CANx == CAN2) // clock of CAN2 is bounded to CAN1 clock, so  activating both in RCC when we use CAN2
	{ SET_BIT(RCC->APB1ENR, RCC_APB1ENR_CAN2EN); }	//RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;



	/*Enter initialization mode*/
	SET_BIT(CANx->MCR, CAN_MCR_INRQ);	//CANx->MCR |= CAN_MCR_INRQ;

	/*Wait until CANx is in initialization mode*/
	while((CANx->MSR & CAN_MSR_INAK) == 0){}

	/*Exit sleep mode*/
	CLEAR_BIT(CANx->MCR, CAN_MCR_SLEEP);	//CANx->MCR &=~CAN_MCR_SLEEP;

	/*Wait until CANx is out of sleep mode*/
	while((CANx->MSR & CAN_MSR_SLAK) != 0){}

	/*Configure timing parameters including baudrate by configuring time segment 1 and 2
	 * and prescaler*/
	/*
		Sample Point: The sampling point setting,
  		for which we wrote the algorithm in the previous question,
  		is critical for line stability, especially at high speeds such as 500 kbps and 1 Mbps.
  		The Sample Point ratio ({1 + TS1}/{1 + TS1 + TS2}) should generally be between
  		75% and 87.5% (ideally around 80%) according to CANopen/SAE J1939 standards.

		BaudRate = 1/NominalBitTime
		NominalBitTime = sync_seg + tbitseg1 + tbitseg2
		sync_seg = 1*tq
		tbitseg1 = tq*(TS1[3:0] + 1)
		tbitseg2 = tq*(TS2[2:0] + 1)
		where,
		(TimeQuantum)  tq = (BRP[9:0] + 1) x tPCLK
		(peripheral bus clock period)   tPCLK = time period of the APB clock  --> 1/(APBclkfreq)
		normally no config at reset --> 16MHz HSI no presc  division --> APB1 = 16MHz --> tPCLK = 1/16000000

		for  TS1[3:0] = 1   , TS2[3:0] = 0 ,  BRP[9:0] = 9
		tbs1 = (1+1)*tq  , tbs2 = (0+1)*tq  , BRP = 9
		tq = (9+1)*tPCLK  = 10* 1/(16Mhz)
		NominalBitTime = 1*tq + 2*tq + 1*tq = 4*tq
		BaudRate = 1/(4*tq) = 1/(4*(10/16MHz)) = 16_000_000 /40 = 400_000 = 400Kbit/s  mid speed

		if RCC configured max 180MHz then ABP1 can be max 45 MHz  (because 1/4 div mandatory)
		BaudRate = 1/(4*tq) = 1/(4*(10/45MHz)) = 45_000_000 /40 = 1.125Mbit over speed --> not supported by CANBUS, we have to tune BTR reg

		TS1[3:0] -> 4bit  --> range: 0-15	--> TS1+1 => range: 1-16
		TS2[2:0] -> 3bit  --> range: 0-7	--> TS2+1 => range: 1-8
		BRP[9:0] -> 10bit --> range: 0-1023	--> BRP+1 => range: 1-1024	---> tq = 1*tPCLK  to 1024*tPCLK

		NominalBitTime = tq*(TS1+1+TS2+1+1) = 	tq*(TS1+TS2+3) = ((BRP+1)*tPCLK)*(TS1+TS2+3)
		Baudrate = 1/((BRP+1)*tPCLK)*(TS1+TS2+3)) = fPCLK/((BRP+1)*(TS1+TS2+3))

	*/

	//CAN1->BTR = (1<< CAN_BTR_TS1_Pos) | (0 << CAN_BTR_TS2_Pos) | (9 << CAN_BTR_BRP_Pos);

	// define a bit timing params variable to hold the best values for the required baudrate
	can_bit_timing_t can_timing_params ={};
	// get proper can bit timing parameters according to APB1_freqHz and required baudrate[Kbps]
	can_get_standard_timing(APB1_freqHz, baudrate, &can_timing_params);

	uint32_t temp_btr = 0; //empty temporary timing register
	temp_btr |= (can_timing_params.BRP << CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk; //write BRP value to CAN_BTR_BRP position
	temp_btr |= (can_timing_params.TS1 << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk; //write TS1 value to  CAN_BTR_TS1 position
	temp_btr |= (can_timing_params.TS2 << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk; //write TS2 value to  CAN_BTR_TS2 position
	temp_btr |= (can_timing_params.SJW << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk; //write SJW value to  CAN_BTR_SJW position
	//WRITE_REG_PORTION(temp_btr, CAN_BTR_BRP_Msk, CAN_BTR_BRP_Pos, can_timing_params.BRP);
	//WRITE_REG_PORTION(temp_btr, CAN_BTR_TS1_Msk, CAN_BTR_TS1_Pos, can_timing_params.TS1);
	//WRITE_REG_PORTION(temp_btr, CAN_BTR_TS2_Msk, CAN_BTR_TS2_Pos, can_timing_params.TS2);
	//WRITE_REG_PORTION(temp_btr, CAN_BTR_SJW_Msk, CAN_BTR_SJW_Pos, can_timing_params.SJW);
	// write prepared value to real CAN_BTR register
	CANx->BTR = temp_btr;


	/*Select mode*/
	if(CAN_TRANS_MODE_LOOPBACK == mode)
	{
		/*Loopback mode*/
		//CAN1->BTR |= (1U<<30);
		SET_BIT(CANx->BTR, CAN_BTR_LBKM);	//set  LoopBackMode bit on CAN1->BTR Register
	}
	else
	{	/*Normal mode*/
		//CAN1->BTR &=~(1U<<30);
		CLEAR_BIT(CANx->BTR, CAN_BTR_LBKM);	//Clear LoopBackMode bit on CAN1->BTR Register
	}
}





//activating interrupt for selected fifo
void can_start(CAN_TypeDef *CANx, can_fifo_sel selected_fifo) //(can_fifo_sel)
{
	/*Exit initialization mode*/
	//CANx->MCR &=~ CAN_MCR_INRQ;
	CLEAR_BIT(CANx->MCR, CAN_MCR_INRQ);

	/*Wait until CAN1 is out of initialization mode*/
	while((CANx->MSR & CAN_MSR_INAK) == 1){}//be sure that CAN1 is quit from init mode

	if(CAN_FIFO_0 == selected_fifo)
	{
		/*Enable interrupt for FIFO0 message pending*/
		//CANx->IER |= (1U<<1);
		SET_BIT(CANx->IER, CAN_IER_FMPIE0);
	}
	else
	{
		/*Enable interrupt for FIFO1 message pending*/
		//CANx->IER |= (1U<<4);
		SET_BIT(CANx->IER, CAN_IER_FMPIE1);
	}

	/*Enable CAN RX0 interrupt for message reception*/
	if(CANx == CAN1)
		NVIC_EnableIRQ(CAN1_RX0_IRQn);
	else if (CANx == CAN2)
		NVIC_EnableIRQ(CAN2_RX0_IRQn);

}


/**
  * @brief  Add a message to the first free Tx mailbox and activate the
  *         corresponding transmission request.
  * @param  pHeader pointer to a CAN_TxHeaderTypeDef structure.
  * @param  aData array containing the payload of the Tx frame.
  * @param  pTxMailbox pointer to a variable where the function will return
  *         the TxMailbox used to store the Tx message.
  *         This parameter can be a value of @arg CAN_Tx_Mailboxes.
  * @retval  status
  */
uint8_t can_add_tx_message(can_tx_header_typedef *pHeader, uint8_t aData[], uint32_t *pTxMailbox)
{
	/* TX MailBox structure
	 * there are 3 tx_mailboxes (x -> 0:2)
	 * CAN_TIxR -- holds ID , holds  TXRQ(transmit request bit)
	 * CAN_TDTxR -- holds DLC(how many bytes to send = data length)  , holds TGT(TransmitGlobalTime mode) bit
	 * CAN_TDLxR -- holds data bytes [byte3][byte2][byte1][byte0] as one u32 Low Register
	 * CAN_TDHxR -- holds data bytes [byte7][byte6][byte5][byte4] as one u32 High Register
	 */

    // Variable to hold the selected transmit mailbox
	uint32_t transmitmailbox;

	// Read the Transmit Status Register
	uint32_t tsr = READ_REG(CAN1->TSR);

	// Check that at least one Tx mailbox is empty
    if (((tsr & CAN_TSR_TME0) != 0U) ||
        ((tsr & CAN_TSR_TME1) != 0U) ||
        ((tsr & CAN_TSR_TME2) != 0U))
    {
      /* Select an empty transmit mailbox */
      transmitmailbox = (tsr & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;

      /* Check transmitmailbox validity */
      if (transmitmailbox > 2U)
      {
    	  return 1; // Invalid mailbox selected
      }

      /* Store the Tx mailbox */
      *pTxMailbox = (uint32_t)1UL << transmitmailbox;


      //CAN_TI0R register bit map for std_id: (MSB)  STDID[31:21], rsv[zeros], IDE_BIT,RTR_BIT,TXRQ_BIT (LSB)  -->total 32bit
      //CAN_TI0R register bit map for std_id: (MSB)  EXTID[31:3], IDE_BIT,RTR_BIT,TXRQ_BIT (LSB)  -->total 32bit
      /* Set up the Id */
      uint32_t tmpreg = 0;
      tmpreg |= ((pHeader->ide & 0x1UL) << CAN_TI0R_IDE_Pos); // put ide bit
      tmpreg |= ((pHeader->rtr & 0x1UL) << CAN_TI0R_RTR_Pos); // put rtr bit
      //TXRQ_BIT leaved 0 because it must be set at the end (after putting DLC, data bytes and TGT bit) .

      if (pHeader->ide == CAN_ID_EXT)
      { tmpreg |= ((pHeader->id & 0x3FFFFUL)<< CAN_TI0R_EXID_Pos); }//put extd_id
      else //CAN_ID_STD
      { tmpreg |= ((pHeader->id & 0x7FFUL) << CAN_TI0R_STID_Pos); }//put std_id

      CAN1->sTxMailBox[transmitmailbox].TIR = tmpreg; //put tmp register to real Transmit register


      //CAN_TDT0R register bit map : TIME[31:16],rsv[15:9], TGT_BIT, rsv[7:4],DLC[3:0] -->total 32bit
      /* Set up the DLC */
      CAN1->sTxMailBox[transmitmailbox].TDTR = (pHeader->dlc & 0x0000000FUL); //first 4 bit LSB of TDTR --> DLC[3:0]

      /* Set up the Transmit Global Time mode */
      if (pHeader->transmit_global_time == 1)
      {
        SET_BIT(CAN1->sTxMailBox[transmitmailbox].TDTR, CAN_TDT0R_TGT); //set TGT bit
      }

      //TIME[31:16] not set

      /* Set up the data field*/
      //(x:mailbox_number) CAN_TDLxR -- holds data bytes [byte3][byte2][byte1][byte0] as one u32 Low Register
      //(x:mailbox_number) CAN_TDHxR -- holds data bytes [byte7][byte6][byte5][byte4] as one u32 High Register
      U32RegAccessBytes_t DataMapU32Bytes;// a union typedef mapping two u32 L and H registers to 8byte_array
      for(int i=0; i<pHeader->dlc; i++){ DataMapU32Bytes.bytes[i] = aData[i]; }
      CAN1->sTxMailBox[transmitmailbox].TDLR = DataMapU32Bytes.Reg.L32;
      CAN1->sTxMailBox[transmitmailbox].TDHR = DataMapU32Bytes.Reg.H32;

      /* Request transmission */
      SET_BIT(CAN1->sTxMailBox[transmitmailbox].TIR, CAN_TI0R_TXRQ); //setting TXRQ bit tells CAN to send the frame in the mailbox_x

      /* Return function status */
      return 0;
    }
    return 0;
}

/**
  * @brief  Get an CAN frame from the Rx FIFO zone into the message RAM.
  * @param  RxFifo Fifo number of the received message to be read.
  * @param  pHeader pointer to a can_rx_header_typedef structure where the header
  *         of the Rx frame will be stored.
  * @param  aData array where the payload of the Rx frame will be stored.
  * @retval  status
  */
uint8_t can_get_rx_message(can_fifo_sel RxFifo, can_rx_header_typedef *pHeader, uint8_t aData[])//(can_fifo_sel , can_rx_header_typedef *, uint8_t[])
{

    /* Check the Rx FIFO */
    if (RxFifo == CAN_FIFO_0) /* Rx element is assigned to Rx FIFO 0 */
    {
      /* Check that the Rx FIFO 0 is not empty */
      if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0U)
      {
        return 1;
      }
    }
    else /* Rx element is assigned to Rx FIFO 1 */
    {
      /* Check that the Rx FIFO 1 is not empty */
      if ((CAN1->RF1R & CAN_RF1R_FMP1) == 0U)
      {
        return 1;
      }
    }


    // Read the header information from the FIFO mailbox
    // Extract identifier, DLC, timestamp, etc.

    pHeader->ide = (CAN_RI0R_IDE & CAN1->sFIFOMailBox[RxFifo].RIR) >> CAN_RI0R_IDE_Pos;
    if (pHeader->ide == CAN_ID_STD)
    {
      pHeader->id = (CAN_RI0R_STID & CAN1->sFIFOMailBox[RxFifo].RIR) >> CAN_RI0R_STID_Pos;
    }
    else
    {
      pHeader->id = (CAN_RI0R_EXID & CAN1->sFIFOMailBox[RxFifo].RIR) >> CAN_RI0R_EXID_Pos;
    }
    pHeader->rtr = ((CAN_RI0R_RTR & CAN1->sFIFOMailBox[RxFifo].RIR)) >> CAN_RI0R_RTR_Pos;
    pHeader->dlc = (CAN_RDT0R_DLC & CAN1->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_DLC_Pos;
    pHeader->filter_match_index = (CAN_RDT0R_FMI & CAN1->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_FMI_Pos;
    pHeader->timestamp = (CAN_RDT0R_TIME & CAN1->sFIFOMailBox[RxFifo].RDTR) >> CAN_RDT0R_TIME_Pos;


    //(x:fifo_number) CAN_RDLxR -- holds data bytes [byte3][byte2][byte1][byte0] as one u32 Low Register
    //(x:fifo_number) CAN_RDHxR -- holds data bytes [byte7][byte6][byte5][byte4] as one u32 High Register
    // a union typedef mapping two u32 L and H registers to 8byte_array
    U32RegAccessBytes_t DataMapU32Bytes = {
    		.Reg = {
    				.L32 = CAN1->sFIFOMailBox[RxFifo].RDLR,
					.H32 = CAN1->sFIFOMailBox[RxFifo].RDHR
    				}
    		};
    for(int i=0; i<pHeader->dlc; i++){ aData[i] = DataMapU32Bytes.bytes[i]; }

	/*
	 * `SET_BIT` operates internally as `CAN1->RF0R |= CAN_RF0R_RFOM0;`,
	 * which performs a **Read-Modify-Write (RMW)** operation.
	 * In the **bxCAN** peripheral, the `RF0R` register contains error status flags such as `FULL0` and `FOVR0` (FIFO Overrun).
	 *  These flags are of type **rc_w1 (Read/Clear on Write 1)**.This means that if you read the register and then
	 *  write the value back unchanged, you may inadvertently clear these error flags. For example,
	 *  if a FIFO overrun occurs during flight, using this approach could silently clear the `FOVR0` flag before the
	 *  software has a chance to detect it, causing the system to lose all indication that the overrun ever happened.
	 * */

    /* Release the FIFO */
    if (RxFifo == CAN_FIFO_0) /* Rx element is assigned to Rx FIFO 0 */
    {
      /* Release RX FIFO 0 */
      SET_BIT(CAN1->RF0R, CAN_RF0R_RFOM0);
    }
    else /* Rx element is assigned to Rx FIFO 1 */
    {
      /* Release RX FIFO 1 */
      SET_BIT(CAN1->RF1R, CAN_RF1R_RFOM1);
    }

  return 0;// Message read successfully
}





/* Notes about filtering:
  MASK MODE: Only Masked bits of ID must match with the incoming ID
  LIST MODE: All bits of incoming ID must match one of the IDs in the list

  There are 28 filterbanks composed of two u32 bank registers (CAN_FxR1 and CAN_FxR2)
  FilterBank Registers are all U32 Registers -> [CAN_F0R1 ,CAN_F0R2]  .... [CAN_F27R1 ,CAN_F27R2]
  We can access these FilterBank Registers  through CAN1->sFilterRegister[n]	  where n : 0 to 27  FilterBank number

  we can obtain 2x filters when we use 16bit scaling mode.
  FilterBank Registers Can be arranged according to scaling modes (16bit and 32bit filter)

  if 32bit scaling mode is used then Filterbank_x Registers arranged like below  (where x:0 to 27)
	  !!!All ID MSB  are placed on MSB of the arranged register (U32_R1, U32R2, U16_R1L, U16_R1H, U16R2L, U16R2H )
	  in MASK mode -- 1x 32bitFilter(ID+MASK)
		(U32_R1)  CAN_FxR1[31:0] -->ID
		(U32_R2)  CAN_FxR2[31:0] -->MASK
	  in List mode -- 2x 32bitFilter(IDlist)
		(U32_R1)  CAN_FxR1[31:0] -->ID1
		(U32_R2)  CAN_FxR2[31:0] -->ID2

  if 16bit scaling mode is used then Filterbank_x Registers arranged like below  (where x:0 to 27)
	  in MASK mode , -- 2x 16bitFilters(ID+MASK , ID+MASK)
		(U16_R1L)	CAN_FxR1[15:0]	-->ID1
		(U16_R1H)	CAN_FxR1[31:16]	-->MASK1
		(U16_R2L)	CAN_FxR2[15:0]	-->ID2
		(U16_R2H)	CAN_FxR2[31:16]	-->MASK2
	  in List mode , -- 4x 16bitFilters(IDList)
		(U16_R1L)	CAN_FxR1[15:0]	-->ID1
		(U16_R1H)	CAN_FxR1[31:16]	-->ID2
		(U16_R2L)	CAN_FxR2[15:0]	-->ID3
		(U16_R2H)	CAN_FxR2[31:16]	-->ID4

	!!! filter assignments are managed by CAN1->FMR register
	CAN1 (Master bxCAN): The primary controller that has direct ownership and
	management of the filter banks (28 in total). CAN2 depends on CAN1 to operate.
	Default (Reset) State: When the device powers up, the default value of the CANSB bits is 14.
	This means that, unless you explicitly change it, the filter banks are split evenly: 14 filters are assigned to CAN1 (0-13)
	and 14 filters are assigned to CAN2 (14-27). If you want to change this allocation,
	you must configure it during the initialization phase.


	CAN2 (Slave bxCAN): Cannot directly control its own filter banks.
	Even the allocation of filters to CAN2 (for example, determining how many of the 28 filters are assigned to CAN2)
	is configured through CAN1's registers (CAN1->FMR).

	!!! Clock dependency of CAN2  to CAN1
	CAN1 receives its clock signal directly from the APB1 bus.
	For CAN2's internal logic circuitry and register interface to function, the CAN1 clock must also be enabled:
	RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
	In other words, even if your application uses only CAN2, you are still required to enable the CAN1 hardware clock in the background.
	Otherwise, you will not be able to access the CAN2 registers.


	!!!In the  Extended ID  format of  CAN 2.0B , the 29-bit identifier is divided into two parts:
	Base ID (11 bits):  Stored in the  STID[10:0]  field. Extension ID (18 bits):  Referred to as  EXID[17:0] .
	A  16-bit filter window  is very limited in size. After allocating space for the  IDE  bit,
	only  3 bits remain available. These correspond to the three most significant bits of the Extension ID:
	EXID[17] , EXID[16] , and EXID[15].
	For this reason, the hardware designers mapped EXID[17:15] into the remaining filter bits.
	The other  15 bits of the Extension ID (EXID[14:0]) do not fit within the 16-bit filter element
	and are therefore discarded for filtering purposes.
	in other words, they cannot participate in the filtering decision when using the 16-bit filter scale.

	Checking Only the IDE Bit (Common Usage):
	in 16-bit filter mode, you can set the IDE bit to 1 to specify:
	"Accept messages whose Standard ID is X and that are also transmitted in the Extended frame format."
	This allows the filter to distinguish between Standard and Extended CAN frames by
	matching the IDE (identifier Extension) bit,
	ensuring that only frames with the desired identifier format are accepted.

	So if you set a 16bit filter and caught a extended id frame,
	then you have to check if received entire ext_id matches your ext_id

	==================================================================================================
	STM32 bxCAN BARE-METAL FILTER BANK REGISTER ORGANIZATION GUIDE
	==================================================================================================

	[INPUT PARAMETERS (LINEAR / FLAT FORMAT)]
	  - std_id    : 11-bit flat integer ([10:0])
	  - std_mask  : 11-bit flat mask ([10:0]), For exact match: 0x7FF
	  - ext_id    : 29-bit flat integer ([28:0]),  Upper 11-bits = STID, Lower 18-bits = EXID
	  - ext_mask  : 29-bit flat mask ([28:0]), For exact match: 0x1FFFFFFF

	--------------------------------------------------------------------------------------------------
	1) 32-BIT SCALE MODE
	--------------------------------------------------------------------------------------------------
	* In this mode, each filter bank can hold only 1 ID + MASK pair.
	* FR1 is entirely dedicated to the target ID, and FR2 is entirely dedicated to the Mask value.

	SCENARIO A: 32-bit Extended ID
	  -> FR1 (ID) Register Bit Mapping:
		 [31:21] : (ext_id & 0x1FFC0000) >> 18  --> STID (Upper 11 bits of the Extended ID)
		 [20:3]  : (ext_id & 0x0003FFFF) << 3   --> EXID (Lower 18 bits of the Extended ID)
		 [2]     : 1                            --> IDE  (1 = Strictly Extended format)
		 [1]     : 0                            --> RTR  (0 = Data Frame, not a Remote Request)
		 [0]     : 0                            --> Reserved / Not used

	  -> FR2 (Mask) Register Bit Mapping:
		 [31:21] : (ext_mask & 0x1FFC0000) >> 18--> STID Mask
		 [20:3]  : (ext_mask & 0x0003FFFF) << 3 --> EXID Mask
		 [2]     : 1                            --> IDE Mask (IDE bit must be matched/checked)
		 [1]     : 1                            --> RTR Mask (RTR bit must be matched/checked)
		 [0]     : 0                            --> Reserved / Not used

	SCENARIO B: 32-bit Standard ID
	  -> FR1 (ID) Register Bit Mapping:
		 [31:21] : std_id & 0x7FF               --> STID (11-bit Standard ID)
		 [20:3]  : 0                            --> EXID (These fields are cleared for Standard ID)
		 [2]     : 0                            --> IDE  (0 = Strictly Standard format)
		 [1]     : 0                            --> RTR  (0 = Data Frame, not a Remote Request)
		 [0]     : 0                            --> Reserved / Not used

	  -> FR2 (Mask) Register Bit Mapping:
		 [31:21] : std_mask & 0x7FF             --> STID Mask (0x7FF for exact match)
		 [20:3]  : 0                            --> EXID Mask (Ignore incoming extended extensions)
		 [2]     : 1                            --> IDE Mask (IDE bit must be matched/checked)
		 [1]     : 1                            --> RTR Mask (RTR bit must be matched/checked)
		 [0]     : 0                            --> Reserved / Not used

	--------------------------------------------------------------------------------------------------
	2) 16-BIT SCALE MODE
	--------------------------------------------------------------------------------------------------
	* In this mode, each filter bank can hold 2 ID + MASK pairs.
	* Position Definition (pos):
		pos = 0 -> Lower 16 bits of the registers are used ([15:0] range).
		pos = 1 -> Upper 16 bits of the registers are used ([31:16] range).

	SCENARIO C: 16-bit Extended ID (Truncated Filtering)
	* WARNING: Only the upper 14 bits of the Extended ID can be filtered. Lower 15 bits (EXID[14:0]) are discarded.

	  -> FR1 (ID Cell - [15:0] for pos=0 / [31:16] for pos=1) Layout:
		 [15:5]  : (ext_id & 0x1FFC0000) >> 13  --> STID (Upper 11 bits of the Extended ID)
		 [4]     : 0                            --> RTR  (0 = Data Frame)
		 [3]     : 1                            --> IDE  (1 = Strictly Extended format)
		 [2:0]   : (ext_id & 0x00038000) >> 15  --> EXID (Only the upper 3 bits of EXID: [17:15])

	  -> FR2 (Mask Cell - [15:0] for pos=0 / [31:16] for pos=1) Layout:
		 [15:5]  : (ext_mask & 0x1FFC0000) >> 13--> STID Mask
		 [4]     : 1                            --> RTR Mask (RTR state must be strictly verified)
		 [3]     : 1                            --> IDE Mask (IDE state must be strictly verified)
		 [2:0]   : (ext_mask & 0x00038000) >> 15--> EXID Mask (Remaining lower 15 bits cannot be checked)

	SCENARIO D: 16-bit Standard ID (Full Filtering)
	* NOTE: Standard ID fits perfectly and without any data loss into this narrow 16-bit cell.

	  -> FR1 (ID Cell - [15:0] for pos=0 / [31:16] for pos=1) Layout:
		 [15:5]  : (std_id & 0x7FF) << 5        --> STID (11-bit Standard ID shifted to the left)
		 [4]     : 0                            --> RTR  (0 = Data Frame , 1 = Request Frame)
		 [3]     : 0                            --> IDE  (0 = Strictly Standard format)
		 [2:0]   : 0                            --> EXID (Cleared when filtering Standard IDs)

	  -> FR2 (Mask Cell - [15:0] for pos=0 / [31:16] for pos=1) Layout:
		 [15:5]  : (std_mask & 0x7FF) << 5      --> STID Mask (0x7FF << 5 for exact match)
		 [4]     : 1                            --> RTR Mask (RTR state must be strictly verified)
		 [3]     : 1                            --> IDE Mask (IDE state must be strictly verified)
		 [2:0]   : 0                            --> EXID Mask (0 = Ignore incoming extended extensions)

	==================================================================================================
	GOLDEN DRIVER RULES (CHEAT SHEET):
	==================================================================================================
	1. IDE MASK PROTECTION: You must ALWAYS set the corresponding IDE bit in the FR2 mask register to
	   '1', regardless of whether you are filtering Standard or Extended IDs. Otherwise, the filter
	   cannot distinguish between frame formats.
	2. 16-BIT EXTENDED LIMITATION: Filtering extended IDs in 16-bit scale mode is risky; since the
	   hardware does not check the lower 15 bits, foreign extended packets with identical upper bits
	   can pass through your filter.
	3. HARDWARE SHIFTING: In 32-bit extended mode, a '>> 18' shift is required for STID to align properly,
	   whereas in compressed 16-bit extended mode, a '>> 13' shift is performed to position the same
	   bits into the [15:5] range.
	==================================================================================================



	==================================================================================================
	STM32 bxCAN BARE-METAL FILTER BANK REGISTER ORGANIZATION GUIDE (IDENTIFIER LIST MODE)
	==================================================================================================

	[INPUT PARAMETERS (LINEAR / FLAT FORMAT)]
	  - std_id    : 11-bit flat integer ([10:0])
	  - ext_id    : 29-bit flat integer ([28:0]), Upper 11-bits = STID, Lower 18-bits = EXID

	--------------------------------------------------------------------------------------------------
	1) 32-BIT SCALE + IDENTIFIER LIST MODE
	--------------------------------------------------------------------------------------------------
	* In this mode, each filter bank can hold exactly 2 independent target IDs.
	* FR1 behaves entirely as ID_0, and FR2 behaves entirely as ID_1.
	* There are no mask bits; incoming packets must match these IDs exactly.

	SCENARIO A: 32-bit Extended ID (List Mode)
	  -> FR1 (Target ID 0) or FR2 (Target ID 1) Register Bit Mapping:
		 [31:21] : (ext_id & 0x1FFC0000) >> 18  --> STID (Upper 11 bits of the Extended ID)
		 [20:3]  : (ext_id & 0x0003FFFF) << 3   --> EXID (Lower 18 bits of the Extended ID)
		 [2]     : 1                            --> IDE  (1 = Strictly Extended format)
		 [1]     : 0                            --> RTR  (0 = Accept Data Frame ONLY)
		 [0]     : 0                            --> Reserved / Not used

	  * Note: If you want to accept a Remote Frame instead, set RTR (Bit 1) to 1.

	SCENARIO B: 32-bit Standard ID (List Mode)
	  -> FR1 (Target ID 0) or FR2 (Target ID 1) Register Bit Mapping:
		 [31:21] : std_id & 0x7FF               --> STID (11-bit Standard ID)
		 [20:3]  : 0                            --> EXID (Must be cleared for Standard ID)
		 [2]     : 0                            --> IDE  (0 = Strictly Standard format)
		 [1]     : 0                            --> RTR  (0 = Accept Data Frame ONLY)
		 [0]     : 0                            --> Reserved / Not used

	--------------------------------------------------------------------------------------------------
	2) 16-BIT SCALE + IDENTIFIER LIST MODE
	--------------------------------------------------------------------------------------------------
	* In this mode, each filter bank is split into 4 independent 16-bit slots (slots 0, 1, 2, 3).
	* This allows a single filter bank to store up to 4 independent target IDs.
	* Register Layout breakdown:
		FR1 [15:0]  -> Slot 0 (Target ID 0)
		FR1 [31:16] -> Slot 1 (Target ID 1)
		FR2 [15:0]  -> Slot 2 (Target ID 2)
		FR2 [31:16] -> Slot 3 (Target ID 3)

	SCENARIO C: 16-bit Extended ID (Truncated List Mode)
	* WARNING: Only the upper 14 bits of the Extended ID can be stored. Lower 15 bits (EXID[14:0]) are discarded.

	  -> Target ID Slot Layout (Applicable to any of the 4 slots defined above):
		 [15:5]  : (ext_id & 0x1FFC0000) >> 13  --> STID (Upper 11 bits of the Extended ID)
		 [4]     : 0                            --> RTR  (0 = Accept Data Frame ONLY)
		 [3]     : 1                            --> IDE  (1 = Strictly Extended format)
		 [2:0]   : (ext_id & 0x00038000) >> 15  --> EXID (Only the upper 3 bits of EXID: [17:15])

	SCENARIO D: 16-bit Standard ID (Full List Mode)
	* NOTE: Standard ID fits perfectly and without any data loss into these narrow 16-bit list slots.

	  -> Target ID Slot Layout (Applicable to any of the 4 slots defined above):
		 [15:5]  : (std_id & 0x7FF) << 5        --> STID (11-bit Standard ID shifted to the left)
		 [4]     : 0                            --> RTR  (0 = Accept Data Frame ONLY)
		 [3]     : 0                            --> IDE  (0 = Strictly Standard format)
		 [2:0]   : 0                            --> EXID (Must be cleared when storing Standard IDs)

	==================================================================================================
	GOLDEN LIST-MODE RULES (CHEAT SHEET):
	==================================================================================================
	1. IDE BIT IMPORTANCE: In List Mode, the IDE bit (Bit 2 in 32-bit, Bit 3 in 16-bit) is a mandatory
	   filter criteria. For Standard IDs, it MUST be 0. For Extended IDs, it MUST be 1. If configured
	   incorrectly, the hardware will reject incoming frames because frame formats will mismatch.
	2. RTR BIT IMPORTANCE: Since there is no mask register to make the RTR bit "don't care", the hardware
	   will explicitly check the RTR bit. Setting RTR = 0 means that specific slot will ONLY accept
	   Data Frames. If a Remote Frame with the exact same ID arrives, it will be blocked.
	3. CAPACITY ADVANTAGE: List mode doubles your filtering capacity compared to Mask mode. You can
	   filter 2 full Extended IDs (or 2 Standard IDs) per bank in 32-bit scale, and up to 4 Standard
	   IDs per bank in 16-bit scale.
	==================================================================================================

 */

static uint16_t map_id_to_16bitfilterbankreg(can_id_mask_handle_t* id_obj)
{
	//16BITFILTER_REG_MAPPING
	//STD_ID[10:0],RTR_BIT,IDE_BIT, EXID[17:15]  --> total 16bit

	uint32_t reg_tmp = 0;

	if(id_obj->rtr)// add RTR bit
	{ SET_BIT(reg_tmp, (1UL<<4U)); } //reg_tmp |= (1UL<<4U) ;
	else
	{ CLEAR_BIT(reg_tmp,(1UL<<4U)); } //reg_tmp &= ~(1UL<<4U) ;

	if(id_obj->ide)// add IDE bit
	{ SET_BIT(reg_tmp,(1UL<<3U)); }//reg_tmp |= (IDE_msk) ; //add IDE bit=1 for extd_id , IDE bit=1 for extd_id mask as well
	else
	{ CLEAR_BIT(reg_tmp,(1UL<<3U)); }

	if(CAN_ID_STD == id_obj->id_style)
	{
		reg_tmp|= ((id_obj->id & 0x000007FFUL) << 5) ;  //id
		//std_id --> exid[17:15] = 0

		return (uint16_t)reg_tmp;
	}
	else //CAN_ID_EXT
	{
		/* to understand more clear  please check definition below
				typedef union {
					uint32_t extid_or_mask_u32; // full val to be assigned to FRx reg

					//STM32 denotes upper11bit of exid is std part for 16bitfilterbank mapping
					//STM32 denotes lower18bit of exid is ext part for 16bitfilterbank mapping ->EXID[17:15]
					struct {
						uint32_t exid_14_0  	: 15; // Bit 0..14
						uint32_t exid_17_15  	: 3; // Bit 15..17
						uint32_t std  			: 11; // Bit 18..28
						uint32_t rsvd   		: 3; //  Bit 29..31
					}idpart;

					// full 29bit single part ext_id
					struct {
						uint32_t ext_id    : 29; // Bit 0..28 (Kullanıcının bildiği kesintisiz ID)
						uint32_t rsvd      : 3;
					}idfull;

				} extid_or_mask;

				extid_or_mask.idpart.exid_14_0 --> will be trash  when use 16bitfilterbank organization
				extid_or_mask.idpart.exid_17_15 --> will be first (lsb) 3 bits of 16bitfilterbank
		*/

		/*put std_id part of given extended_id in the list	*/
		//move extid_or_mask[28:18]  to reg_tmp[15:5]
		reg_tmp |= ((id_obj->id & 0x1FFC0000UL) >> 13);

		//adding EXT[17:15] (EXT[17:15] part of ext_id is extid_or_mask->[17:15]bits)
		reg_tmp |= ((id_obj->id & 0x00038000UL)>>15);

		return (uint16_t)reg_tmp;
	}

}


/**
 *
  * @brief  Configure Filters according to params
  * @param  filter_index  filterbank number (range 0 to 27)
  * @param  filter_assigned_to_fifo  selected fifo(FIFO0 or FIFO1) which our filter to be assigned
  * @param  scaling_mode   R1 and R2  filter bank register will be organized as two 16bit or single 32bit filter
  * @param  filtermode Filterbank Registers will be treated in List or Mask mode
  * @retval  success  (0 success, 1 fail)
  */
int can_filter_config(can_filter_config_t* filtercfg)
{
	/****invariant checking****/
	if(filtercfg->filterbank_number > 27)
	{return 1;}//fail due to wrong parameter


	/*Enter filter initialization mode*/
	//CAN1->FMR |= CAN_FMR_FINIT;
	/*Filter Assignment Mode (FINIT): To modify the FMR_CANSB value, the filter management unit must first be placed into
	initialization Mode. This is done by setting the FINIT (Filter initialization Mode) bit in the CAN1->FMR register to 1.
	CAN1->FMR Bit 0 FINIT: Filter initialization mode
			initialization mode for filter banks
			0: Active filters mode.
			1: initialization mode for the filters.
	 */
	SET_BIT(CAN1->FMR,CAN_FMR_FINIT);

	/*Set the slave filter to start from 20*/
	//CAN1->FMR &=~(CAN_FMR_CAN2SB_Msk);
	//CAN1->FMR |=(20 << CAN_FMR_CAN2SB_Pos);
	/*
	 The default value of the CANSB bits is 14 at reset.
	 So the filter banks are split evenly: 14 filters are assigned to CAN1 (0-13) and 14 filters are assigned to CAN2 (14-27).

	 CAN1->FMR[13:8] bits are CANSB[5:0]: CAN start bank
		These bits are set and cleared by software. When both CAN are used, they define the start bank of each CAN interface:
		000001 = 1 filter assigned to CAN1 and 27 assigned to CAN2
		...
		011011 = 27 filters assigned to CAN1 and 1 filter assigned to CAN2

		example:
		 CANSB[5:0]= 20 means that 20 filters are assigned to CAN1 and 8 filters assigned to CAN2.
		 [CAN_F0R1,CAN_F0R2] to [CAN_F19R1,CAN_F19R2]    assigned to CAN1
		 [CAN_F20R1,CAN_F20R2] to [CAN_F27R1,CAN_F27R2]    assigned to CAN2
	*/
	//if(filtercfg->num_of_filters_assigned_to_can1 < 27)
	if(CAN_NUM_OF_FILTERS_ASSIGNED_TO_CAN1 < 27)
	{
		//WRITE_REG_PORTION(CAN1->FMR, CAN_FMR_CAN2SB_Msk, CAN_FMR_CAN2SB_Pos, filtercfg->num_of_filters_assigned_to_can1);
		WRITE_REG_PORTION(CAN1->FMR, CAN_FMR_CAN2SB_Msk, CAN_FMR_CAN2SB_Pos, CAN_NUM_OF_FILTERS_ASSIGNED_TO_CAN1);
	}
	else
	{return 1;}//fail due to wrong param

	/*************************************************Filter activation sequence*****************************************************/
	/*Deactive filter filter_index*/
	//CAN1->FA1R &=~(CAN_FA1R_FACT18);
	/* CAN1->FA1R[27:0] Bits  FACTx: Filter active
		The software sets this bit to activate Filter x. To modify the Filter x registers (CAN_FxR[0:7]),
		the FACTx bit must be cleared or the FINIT bit of the CAN_FMR register must be set.
		0: Filter x is not active
		1: Filter x is active
	*/
	//CLEAR_BIT(CAN1->FA1R,CAN_FA1R_FACT18);//clear bit18
	CLEAR_BIT(CAN1->FA1R,(0x1UL << filtercfg->filterbank_number));//clear bit filter_index

	/*Set scale configuration*/
	//CAN1->FS1R  |= CAN_FS1R_FSC18;
	/*
	   	CAN1->FS1R[27:0] Bits  FSCx: Filter scale configuration
		These bits define the scale configuration of Filters 27-0.
		0: Dual 16-bit scale configuration
		1: Single 32-bit scale configuration
	*/

	if(CAN_FILTER_SCALING_TWO16BIT == filtercfg->filterbank_scaling )
	{
		//CLEAR_BIT(CAN1->FS1R,CAN_FS1R_FSC18); //clear bit18
		CLEAR_BIT(CAN1->FS1R,(0x1UL << filtercfg->filterbank_number)); //clear bit filter_index
	}
	else if(CAN_FILTER_SCALING_SINGLE32BIT == filtercfg->filterbank_scaling)
	{
		//SET_BIT(CAN1->FS1R,CAN_FS1R_FSC18); //set bit18
		SET_BIT(CAN1->FS1R,(0x1UL << filtercfg->filterbank_number)); //set bit filter_index
	}
	else
	{return 1;}//fail due to wrong param




	/*Set  Filter mode :  Mask or List mode according to scale*/
	/*
	    CAN1->FM1R[27:0] Bits  FBMx: Filter mode
		Mode of the registers of Filter x.
		0: Two 32-bit registers(R1 and R2) of filter_bank_x are in Identifier Mask mode.
		1: Two 32-bit registers(R1 and R2) of filter_bank_x are in Identifier List mode.
	 */

	if(CAN_FILTER_MODE_MASK == filtercfg->filterbank_mask_or_list_mode)/**************************FILTER MASK MODE******************/
	{

			/*Configure filter mode to identifier mask mode*/
			//CAN1->FM1R &=~CAN_FM1R_FBM18;
			//CLEAR_BIT(CAN1->FM1R,CAN_FM1R_FBM18);
			CLEAR_BIT(CAN1->FM1R,(0x1UL << filtercfg->filterbank_number));


			//PUT ID LIST TO FILTERBANKREGS ACCORDINGTO SCALING MODE(DUAL16BIT or SINGLE32BIT)
			if(CAN_FILTER_SCALING_TWO16BIT == filtercfg->filterbank_scaling)
			{
				U32RegAccessBytes_t data_access_obj = {};

				//16BITFILTER_MASK_MODE_ID_MAPPING
				//STD_ID[10:0],RTR_BIT,IDE_BIT, EXID[17:15]  --> total 16bit
				//filterbank32bitReg FRx[H,L]
				//filterbank32bitReg FR1[16BIT_MASK1, 16BIT_ID1]
				//filterbank32bitReg FR2[16BIT_MASK2, 16BIT_ID2]

				// id style (std or ext) is evaluated in the map_id_to_16bitfilterbankreg() func.
				data_access_obj.words[0] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[0]);  //id1
				data_access_obj.words[1] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[1]);  //mask1
				data_access_obj.words[2] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[2]);  //id2
				data_access_obj.words[3] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[3]);  //mask2

				/*Set identifier1 and mask1 to filterbankregister1*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = data_access_obj.Reg.L32;
				/*Set identifier2 and mask2 to filterbankregister2*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = data_access_obj.Reg.H32;


			}
			else if(CAN_FILTER_SCALING_SINGLE32BIT == filtercfg->filterbank_scaling)
			{
				//32BITFILTER_LIST_MODE_ID_MAPPING
				//MSB-> STD_ID[10:0],...,IDE_BIT,RTR_BIT,bit0  --> total 32bit
				//MSB-> EXT_ID[28:0],IDE_BIT,RTR_BIT,bit0   --> total 32bit
				//filterbank32bitReg Rx[]
				//filterbank32bitReg R1[32BIT_ID1]
				//filterbank32bitReg R2[32BIT_ID2]

				//....................................................ID assignment to FR1 register...................................
				if(CAN_ID_STD == filtercfg->id_mask_arr[0].id_style)
				{
					/*Set standard_identifier1*/
					CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = ((filtercfg->id_mask_arr[0].id) << 21); //std_id_pos = 21

					CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<2U) );//CLEAR IDE BIT  (0 : std_id)
				}
				else if(CAN_ID_EXT == filtercfg->id_mask_arr[0].id_style)
				{
					/*Set extended identifier1*/
					CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = ((filtercfg->id_mask_arr[0].id) << 3); //SET EXTID1

					SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<2U) );//SET IDE BIT  (1 : ext_id)
				}
				else
				{return 1;}//fail due to wrong param

				/*put rtr bit according to given id in the list*/
				if(filtercfg->id_mask_arr[0].rtr) // set if request frame expected
				{ SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<1U) );}//SET RTR BIT  (1 : request, 0: data)
				else// clear if data frame expected
				{ CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<1U) );}//CLEAR RTR BIT  (1 : request, 0: data)


				//........................................................MASK assignment to FR2 register..............................
				if(CAN_ID_STD == filtercfg->id_mask_arr[1].id_style)
				{
					/*Set standard_identifier2*/
					CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = ((filtercfg->id_mask_arr[1].id) << 21); //set MASK for std_id
				}
				else if(CAN_ID_EXT == filtercfg->id_mask_arr[1].id_style)
				{
					/*Set extended identifier2*/
					CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = ((filtercfg->id_mask_arr[1].id) << 3); //SET MASK for extid
				}
				else
				{return 1;}//fail due to wrong param

				SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<1U) );//SET RTR mask BIT  (rtr will be checked)
				SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<2U) ); //SET IDE mask BIT  (ide will be checked)

			}
			else
			{return 1;}//fail due to wrong param
		}
	else if(CAN_FILTER_MODE_LIST == filtercfg->filterbank_mask_or_list_mode)/*****************FILTER LIST MODE*********************/
	{

		/*Configure filter mode to identifier list mode*/
		//CAN1->FM1R |= CAN_FM1R_FBM18;
		//SET_BIT(CAN1->FM1R,CAN_FM1R_FBM18);
		SET_BIT(CAN1->FM1R,(0x1UL << filtercfg->filterbank_number));


		//PUT ID LIST TO FILTERBANKREGS ACCORDINGTO SCALING MODE(DUAL16BIT or SINGLE32BIT)
		if(CAN_FILTER_SCALING_TWO16BIT == filtercfg->filterbank_scaling)
		{
			U32RegAccessBytes_t data_access_obj = {};

			//16BITFILTER_LIST_MODE_ID_MAPPING
			//STD_ID[10:0],RTR_BIT,IDE_BIT, EXID[17:15]  --> total 16bit
			//filterbank32bitReg Rx[H,L]
			//filterbank32bitReg R1[16BIT_ID2, 16BIT_ID1]
			//filterbank32bitReg R2[16BIT_ID4, 16BIT_ID3]

			// id style (std or ext) is evaluated and ide rtr bits are assigned in the map_id_to_16bitfilterbankreg() func.
			data_access_obj.words[0] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[0]);  //id1
			data_access_obj.words[1] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[1]);  //id2
			data_access_obj.words[2] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[2]);  //id3
			data_access_obj.words[3] = map_id_to_16bitfilterbankreg(&filtercfg->id_mask_arr[3]);  //id4

			/*Set identifier1 and identifier2 to filterbankregister1*/
			CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = data_access_obj.Reg.L32;
			/*Set identifier3 and identifier4 to filterbankregister2*/
			CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = data_access_obj.Reg.H32;

		}
		else if(CAN_FILTER_SCALING_SINGLE32BIT == filtercfg->filterbank_scaling)
		{
			//32BITFILTER_LIST_MODE_ID_MAPPING
			//MSB-> STD_ID[10:0]                   --> total 32bit
			//MSB-> EXT_ID[28:0],IDE_BIT,RTR_BIT,bit0   --> total 32bit
			//filterbank32bitReg Rx[]
			//filterbank32bitReg R1[32BIT_ID1]
			//filterbank32bitReg R2[32BIT_ID2]

			//........................................................id1 assignment to FR1 register..............................
			if(CAN_ID_STD == filtercfg->id_mask_arr[0].id_style)
			{
				/*Set standard_identifier1*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = ((filtercfg->id_mask_arr[0].id) << 21); //std_id_pos = 21

				CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<2U) );//CLEAR IDE BIT  (0 : std_id)
			}
			else if(CAN_ID_EXT == filtercfg->id_mask_arr[0].id_style)
			{
				/*Set extended identifier1*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR1 = ((filtercfg->id_mask_arr[0].id) << 3); //SET EXTID1

				SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<2U) );//SET IDE BIT  (1 : ext_id)
			}
			else
			{return 1;}//fail due to wrong param

			/*put rtr bit according to given id in the list*/
			if(filtercfg->id_mask_arr[0].rtr) // set if request frame expected
			{ SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<1U) );}//SET RTR BIT  (1 : request, 0: data)
			else// clear if data frame expected
			{ CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR1), (1UL<<1U) );}//CLEAR RTR BIT  (1 : request, 0: data)


			//....................................................id2 assignment to FR2 register ...................................
			if(CAN_ID_STD == filtercfg->id_mask_arr[1].id_style)
			{
				/*Set standard_identifier2*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = ((filtercfg->id_mask_arr[1].id) << 21); //std_id_pos = 21

				CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<2U) );//CLEAR IDE BIT  (0 : std_id)
			}
			else if(CAN_ID_EXT == filtercfg->id_mask_arr[1].id_style)
			{
				/*Set extended identifier2*/
				CAN1->sFilterRegister[filtercfg->filterbank_number].FR2 = ((filtercfg->id_mask_arr[1].id) << 3); //SET EXTID2

				SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<2U) );//SET IDE BIT  (1 : ext_id)
			}
			else
			{return 1;}//fail due to wrong param

			/*put rtr bit according to given id in the list*/
			if(filtercfg->id_mask_arr[1].rtr) // set if request frame expected
			{ SET_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<1U) );}//SET RTR BIT  (1 : request, 0: data)
			else// clear if data frame expected
			{ CLEAR_BIT((CAN1->sFilterRegister[filtercfg->filterbank_number].FR2), (1UL<<1U) );}//CLEAR RTR BIT  (1 : request, 0: data)

		}
		else
		{return 1;}//fail due to wrong param
	}
	else
	{return 1;}//fail due to wrong param



	/*Assign filter to FIFOx*/
	//CAN1->FFA1R &=~(CAN_FFA1R_FFA18);
	/*
		CAN1->FFA1R[27:0] Bits  FFAx: Filter FIFO assignment for filter x
		The message passing through this filter is stored in the specified FIFO.
		0: Filter assigned to FIFO 0
		1: Filter assigned to FIFO 1
	 */
	if(CAN_FIFO_0 == filtercfg->filterbank_fifo)
	{
		//CLEAR_BIT(CAN1->FFA1R,CAN_FFA1R_FFA18);
		CLEAR_BIT(CAN1->FFA1R,(0x1UL << filtercfg->filterbank_number));
	}
	else if(CAN_FIFO_1 == filtercfg->filterbank_fifo)
	{
		//SET_BIT(CAN1->FFA1R,CAN_FFA1R_FFA18);
		SET_BIT(CAN1->FFA1R,(0x1UL << filtercfg->filterbank_number));
	}
	else
	{return 1;}//fail due to wrong param


	/*Activate filter filter_index*/
	//CAN1->FA1R |= (CAN_FA1R_FACT18);
	//0: Filter x is not active 	1: Filter x is active
	//SET_BIT(CAN1->FA1R,CAN_FA1R_FACT18);
	SET_BIT(CAN1->FA1R,(0x1UL << filtercfg->filterbank_number));

	/*Exit filter initialization mode*/
	//CAN1->FMR &= ~CAN_FMR_FINIT;
	CLEAR_BIT(CAN1->FMR,CAN_FMR_FINIT);

	return 0;//success
}




/**
 * @brief  returns std timing parameters for CAN_BTR REG
 * @param
 * @retval  status
 */
bool can_get_standard_timing(uint32_t pclk1_hz, can_baudrate_t baudrate, can_bit_timing_t *timing) {
	// NominalBitTime = ((BRP+1)*(1 + (TS1+1) + (TS2+1)))
	// Baudrate = pclk1_hz / NominalBitTime

    timing->SJW = 1; // 1 TQ enough for many std networks.

    switch (pclk1_hz) {

        // ====================================================================
        // APB1 = 45 MHz (at STM32F446 Max System Clock Frequency 180MHz )
        // ====================================================================
        case 45000000:
            switch (baudrate) {
            case CAN_BAUD_1000K: timing->BRP = 2;   timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			case CAN_BAUD_800K:  timing->BRP = 4;   timing->TS1 = 7;  timing->TS2 = 1; return true; // %81.8 SP
			case CAN_BAUD_500K:  timing->BRP = 5;   timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			case CAN_BAUD_250K:  timing->BRP = 11;  timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			case CAN_BAUD_125K:  timing->BRP = 23;  timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			case CAN_BAUD_100K:  timing->BRP = 29;  timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP

			// 45MHz / (21 * 15TQ) = 142857Hz -> 95.238 kbps -> exact match using 15 TQ:
			case CAN_BAUD_95K:   timing->BRP = 30;  timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP (Err: %0.16)

			// 45MHz / (27 * 20TQ) = 83333.3Hz -> Exact division:
			case CAN_BAUD_83K:   timing->BRP = 26;  timing->TS1 = 14; timing->TS2 = 3; return true; // %80.0 SP

			case CAN_BAUD_50K:   timing->BRP = 59;  timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP

			// 45MHz / (67 * 20TQ) = 33582Hz -> most robust ratio by %0.7 error:
			case CAN_BAUD_33K:   timing->BRP = 66;  timing->TS1 = 14; timing->TS2 = 3; return true; // %80.0 SP

			case CAN_BAUD_20K:   timing->BRP = 149; timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			case CAN_BAUD_10K:   timing->BRP = 299; timing->TS1 = 10; timing->TS2 = 2; return true; // %80.0 SP
			default: return false;
            }
            break;

        // ====================================================================
        // APB1 = 16 MHz ( HSI Default Clock Source at System Startup)
        // ====================================================================
        case 16000000:
            switch (baudrate) {
				case CAN_BAUD_1000K: timing->BRP = 0;   timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP
				case CAN_BAUD_800K:  timing->BRP = 0;   timing->TS1 = 14; timing->TS2 = 3; return true; // %80.0 SP
				case CAN_BAUD_500K:  timing->BRP = 1;   timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP
				case CAN_BAUD_250K:  timing->BRP = 3;   timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP
				case CAN_BAUD_125K:  timing->BRP = 7;   timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP
				case CAN_BAUD_100K:  timing->BRP = 9;  timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP

				// 16MHz / (12 * 14TQ) = 95238Hz -> exact division:
				case CAN_BAUD_95K:   timing->BRP = 11;  timing->TS1 = 9; timing->TS2 = 2; return true; // %78.5 SP

				// 16MHz / (12 * 16TQ) = 83333.3Hz ->  exact division:
				case CAN_BAUD_83K:   timing->BRP = 11;  timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP

				case CAN_BAUD_50K:   timing->BRP = 19;  timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP

				// 16MHz / (30 * 16TQ) = 33333.3Hz -> exact division:
				case CAN_BAUD_33K:   timing->BRP = 29;  timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP

				case CAN_BAUD_20K:   timing->BRP = 49;  timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP
				case CAN_BAUD_10K:   timing->BRP = 99; timing->TS1 = 12; timing->TS2 = 1; return true; // %87.5 SP

				default: return false;
            }
            break;

        default:
            return false; //cases for Other APB1 freq (ex: 42MHz)
    }
}

