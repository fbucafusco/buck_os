/*
 * os_defs.h
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_DEFS_H_
#define OS_DEFS_H_

#include "stdint.h"


/* types */
#define TASK_COUNT_TYPE				uint32_t	//TODO: PROPAGAR ESTA DEFINICION POR TODOS LADOS...
#define OS_PRIORITY_TYPE			unsigned char
#define OS_EVENT_TYPE				uint32_t		//It stores FLAGS
#define OS_DELAY_TYPE				uint32_t		//It stores a number of TICKS.

/* Enum: task states */
typedef enum
{
    osTskNOT_ACTIVE,
    osTskREADY,
    osTskRUNNING,
    osTskBLOCKED
} tTaskState;


typedef struct
{
    uint32_t 		*sp;							/* stack pointer saved during context switching */

#if OS_FIXED_PIORITY == 0
    OS_PRIORITY_TYPE	priority;		            /* task priority */
#endif

    tTaskState 		state;	     					/* task state */
    OS_DELAY_TYPE		delay;					    /* counter for delay ticks */
    OS_EVENT_TYPE   events_waiting;					/* events_waiting:  */
    OS_EVENT_TYPE   events_setted;					/* events_setted:  */
} tTCB_Dyn;


typedef struct
{
    void 	( *entry_point )( void *arg ); /* pointer to the function associated with the task */
    void      *arg;								/* pointer to the argument for the ask  			*/
    uint32_t  *stackframe;						/* pointer to the array that stores the stack for the task */
    uint32_t   stacksize;						/* size of the stack array */
    uint32_t  config;							/* task configuration */
    OS_PRIORITY_TYPE	  priority;				/* task priority (ficed) or initial priority (dinamic) */
    tTCB_Dyn  *pDin;							/* pointer to the data representing the dynamic behavior of the task */
} tTCB;



#endif /* OS_DEFS_H_ */
