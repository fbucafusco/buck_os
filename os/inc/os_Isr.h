/*
 * os_Isr.h
 *
 *  Created on: 29/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_ISR_H_
#define OS_ISR_H_

#include "cmsis.h"

#define Interrupt(A)

#define _I_DECLARE_ISR_HANDLER_R_(IRQN, HANDLERNAME)	[IRQN]=&HANDLERNAME,
#define _I_OS_ISR_START 	const void (* os_User_Isr_Handlers[])() =\
							{
#define _I_OS_ISR_END 		};	\
							const uint16_t USER_ISR_COUNT = sizeof(os_User_Isr_Handlers)/sizeof(void*);



#endif /* OS_ISR_H_ */
