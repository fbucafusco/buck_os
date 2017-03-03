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

#define MAX_STACK_SIZE 				512
#define MIN_STACK_SIZE 				68

#define INVALID_TASK				-1

#define TASK_COUNT_TYPE				unsigned short

#ifndef OS_IDLE_HOOK_STACK_SIZE
#define OS_IDLE_HOOK_STACK_SIZE 	MIN_STACK_SIZE
#endif

#define OS_IDLE_HOOK_PRIORITY 		0xFF

extern void idle_hook();

typedef enum
{
    osTskNOT_ACTIVE,
    osTskREADY,
    osTskRUNNING,
    osTskBLOCKED
} tTaskState;

typedef struct
{
    uint32_t * sp;								/* stack pointer saved during context switching */
#if OS_FIXED_PIORITY == 0
    unsigned char priority;						/* task priority */
#endif

    tTaskState 	state;	     					/* task state */
    uint32_t	delay;							/* counter for delay ticks */
} tTCB_Dyn;


typedef struct
{
    void 	( *entry_point )( void );			/* pointer to the function associated with the task */
    void      *arg;								/* pointer to the argument for the ask  			*/
    uint32_t  *stackframe;						/* pointer to the array that stores the stack for the task */
    uint32_t   stacksize;						/* size of the stack array */
    tTCB_Dyn  *pDin;							/* pointer to the data representing the dynamic behavior of the task */
    uint32_t  config;							/* task configuration */
    unsigned char priority;						/* task priority (ficed) or initial priority (dinamic) */
} tTCB;


typedef struct
{
    int32_t current_task; /* current_running_task */
    int32_t next_task; 	  /* next task to be run when there is a context change. */
} tSched;


/* this declares a task that uses the same function name */
#define DECLARE_TASK_R( NOMBRE, FCNNAME , ARG , STACKSIZE, PRI, CONFIG)	uint8_t  	NOMBRE##_stack[STACKSIZE];		    \
																tTCB_Dyn 	NOMBRE##_Din;		         	    \
																const tTCB 	NOMBRE##_TCB =				 	    \
																{									 			\
																	.entry_point = &FCNNAME,		   		    \
																	.stackframe  = &NOMBRE##_stack,				\
																	.stacksize   = sizeof(NOMBRE##_stack),		\
																	.pDin        = &NOMBRE##_Din, 				\
																	.arg		 = ARG,							\
																	.priority    = PRI, 						\
																	.config      = CONFIG,						\
																};

/* declares a task which its function name is not reused by any other. */
#define DECLARE_TASK( FCNNAME , ARG , STACKSIZE, PRI, CONFIG) 			DECLARE_TASK_R( FCNNAME , FCNNAME , ARG , STACKSIZE, PRI, CONFIG)

#define OS_TASKS_START() 	DECLARE_TASK(  idle_hook , NULL , OS_IDLE_HOOK_STACK_SIZE , OS_IDLE_HOOK_PRIORITY  , TASK_NOCONFIG); \
							const tTCB *os_tcbs[] = {

#define OS_ADD_TASK(TASK)							&TASK##_TCB,

#define OS_TASKS_END()	 							&idle_hook_TCB, \
													}; 				\
							TASK_COUNT_TYPE TASK_COUNT = sizeof(os_tcbs)/sizeof(tTCB *) - 1 ; //the -1 is because the idle hook is the last in this array

#define OS_IDLE_TASK_INDEX	TASK_COUNT
#define TASK_COUNT_WIH		(TASK_COUNT+1)	//WITH IDLE HOOK

/* otras macros */
#define OS_DISABLE_ISR()	__asm volatile("cpsid f\n");
#define OS_ENABLE_ISR()		__asm volatile("cpsie f\n");



/* flags for task confoguration */
#define TASK_NOCONFIG	0x0000000
#define TASK_AUTOSTART	0x8000000

/* os methods declaration */
void osStart();

#endif /* SCHED_H_ */

/*
 * AGREGAR ESTADOS
 * estados fundamentales
 * ready
 * running
 *
 * otros
 * blocked
 *
 *
 * */
