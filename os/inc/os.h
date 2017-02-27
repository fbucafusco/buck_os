/*
 * sched.h
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#ifndef SCHED_H_
#define SCHED_H_

#include "os_config.h"
#include "board.h"

#define MAX_STACK_SIZE 		512


#define INVALID_TASK		-1

#define TASK_COUNT_TYPE		unsigned short

typedef struct
{
	uint32_t * sp;								/* stack pointer saved during context switching */
} tTCB_Dyn;


typedef struct
{
	void 	( *entry_point )( void );			/* pointer to the function associated with the task */
	void      *arg;								/* pointer to the argument for the ask  			*/
	uint32_t * stackframe;						/* pointer to the array that stores the stack for the task */
	uint32_t   stacksize;						/* size of the stack array */
	tTCB_Dyn  *pDin;							/* pointer to the data representing the dynamic behavior of the task */
} tTCB;


typedef struct
{
    int32_t current_task; /* current_running_task */
    int32_t next_task; 	  /* next task to be run when there is a context change. */
} tSched;

/*
#define DECLARE_TASK( FCNNAME , ARG , STACKSIZE) 	uint8_t  FCNNAME##_stack[STACKSIZE];			\
													tTCB_Dyn FCNNAME##_Din;		         			\
													const tTCB FCNNAME##_TCB =				 	    \
													{									 			\
														.entry_point = &FCNNAME,		   		 	\
														.stackframe  = &FCNNAME##_stack,			\
														.stacksize   = sizeof(NOMBRE##_stack),		\
														.pDin        = &NOMBRE##_Din, 				\
														.arg		 = ARG,							\
													}*/

/* this declares a task that uses the same function name */
#define DECLARE_TASK_R( NOMBRE, FCNNAME , ARG , STACKSIZE)	uint8_t  NOMBRE##_stack[STACKSIZE];			    \
															tTCB_Dyn NOMBRE##_Din;		         			\
															const tTCB NOMBRE##_TCB =				 	    \
															{									 			\
																.entry_point = &FCNNAME,		   		    \
																.stackframe  = &NOMBRE##_stack,				\
																.stacksize   = sizeof(NOMBRE##_stack),		\
																.pDin        = &NOMBRE##_Din, 				\
																.arg		 = ARG,							\
															}

/* declares a task which its function name is not reused by any other. */
#define DECLARE_TASK( FCNNAME , ARG , STACKSIZE) 			DECLARE_TASK_R( FCNNAME , FCNNAME , ARG , STACKSIZE)

#define OS_TASKS_START() 	const tTCB *os_tcbs[] = {

#define OS_ADD_TASK(TASK)							&TASK##_TCB,

#define OS_TASKS_END()	 							}; \
							TASK_COUNT_TYPE TASK_COUNT = sizeof(os_tcbs)/sizeof(tTCB *);



/* os methods declaration */
void start_os();
void os_init();
void taskSetup( void ( *entry_point )( void ), uint32_t * stack, uint32_t stack_size );

#endif /* SCHED_H_ */
