/*
 * os_defs.h
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_DEFS_H_
#define OS_DEFS_H_

#include "stdint.h"

/* priorities */
/* priority is higher when the value of the priority is the higher */
enum
{
    OS_PRI_LOWEST = 0,
    OS_PRI_LOW 	  = 1,
    OS_PRI_MID	  = 2,
    OS_PRI_HIGH	  = 3,
    OS_PRI_HIGHEST = 4
};

/* flags for task confoguration */
#define TASK_NOCONFIG	0x0000000
#define TASK_AUTOSTART	0x8000000


/* types */
typedef uint32_t 			TASK_COUNT_TYPE;	//TODO: PROPAGAR ESTA DEFINICION POR TODOS LADOS...
typedef unsigned char		OS_PRIORITY_TYPE;
typedef uint32_t			OS_EVENT_TYPE;		//It stores FLAGS
typedef uint32_t			OS_DELAY_TYPE;		//It stores a number of TICKS.

typedef struct
{
    TASK_COUNT_TYPE owner_task;
    char counter;
} OS_MUTEX;

/* constants */
#define OS_INFINITE					(~((int)0))



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
    uint32_t 		   *sp;						/* stack pointer saved during context switching */

    OS_PRIORITY_TYPE	current_priority;		/* task priority */

    tTaskState 			state;	     			/* task state */

    OS_DELAY_TYPE		delay;					/* counter for delay ticks */

    OS_EVENT_TYPE   	events_waiting;			/* events_waiting:  */
    OS_EVENT_TYPE   	events_setted;			/* events_setted:  */

    OS_MUTEX			*pWainingMut;			/* mutex that is wainting the task */

#if OS_FIXED_PIORITY == 0
    OS_PRIORITY_TYPE	priority_back;			/* task backup priority for priority inheritance*/
#endif
} tTCB_Dyn;


typedef struct
{
    void 	( *entry_point )( void *arg ); 		/* pointer to the function associated with the task */
    void      *arg;								/* pointer to the argument for the ask  			*/
    uint32_t  *stackframe;						/* pointer to the array that stores the stack for the task */
    uint32_t   stacksize;						/* size of the stack array */
    uint32_t  config;							/* task configuration */
    OS_PRIORITY_TYPE	def_priority;		    /* task priority (fixed) or default priority (dynamic) */
    tTCB_Dyn  *pDin;							/* pointer to the data representing the dynamic behavior of the task */
} tTCB;

#endif /* OS_DEFS_H_ */
