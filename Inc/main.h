/*
 * main.h
 *
 *  Created on: 1 Tem 2026
 *      Author: 941401
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdint.h>

#define GPIO_MODER_MODESEL_INPUT	(0x0UL)		//00: Input (reset state)
#define GPIO_MODER_MODESEL_OUTPUT	(0x1UL)		//01: General purpose output mode
#define GPIO_MODER_MODESEL_ALTFUNC	(0x2UL)		//10: Alternate function mode
#define GPIO_MODER_MODESEL_ANALOG	(0x3UL)		//11: Analog mode
#define GPIO_AFR_AFSEL_AF0	(0x0UL)		//0b0000: AF0
#define GPIO_AFR_AFSEL_AF7	(0x7UL)		//0b0111: AF7
#define GPIO_AFR_AFSEL_AF9	(0x9UL)		//0b1001: AF9

//BIT_MASK(POS)	(1UL<<POS)
//SET_BIT(REG, BIT)
//CLEAR_BIT(REG, BIT)
#define WRITE_REG_PORTION(REGISTER,PORTION_MASK,PORTION_POS,VAL)	((REGISTER) = (((REGISTER) & (~(PORTION_MASK)))|(((VAL)<<(PORTION_POS))&(PORTION_MASK))))

#define SYS_FREQ				(16000000UL)
#define APB1_freqHz				SYS_FREQ


// U32RegAccessBytes definition is used to map U32 registers(RegH and RegL) to 8bytes long data[] array
typedef union
{
    struct {
        uint32_t L32; // Takes bytes 0-3
        uint32_t H32; // Takes bytes 4-7
    }Reg;
    uint8_t bytes[8];      // Overlaps bytes 0-7
    uint16_t words[4];		// Overlaps
}U32RegAccessBytes_t;



#endif /* MAIN_H_ */
