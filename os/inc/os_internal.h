/*
 * sched.h
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "os_config.h"
#include "os_config_internal.h"
#include "os_tasks.h"
#include "os_event.h"
#include "board.h"

#define MAX_STACK_SIZE 				512
#define MIN_STACK_SIZE 				68      //TODO: CHANGE THIS..... 68 ES EL STACK PARA GUARDAR EL CONTEXTO, PERO ADEMAS SE NECESITA STACK PARA LA ISR DE TICK, Y LA DE PEND SV. Y TODOS SUS LLAMADOS. 

#define INVALID_TASK				-1


#ifndef OS_IDLE_HOOK_STACK_SIZE
#define OS_IDLE_HOOK_STACK_SIZE 	MIN_STACK_SIZE
#endif

/* priority is higher when the value of the priority is the higher */
enum
{
    OS_PRI_LOWEST 	,
    OS_PRI_LOW 		,
    OS_PRI_MID		,
    OS_PRI_HIGH	 	,
    OS_PRI_HIGHEST
};

#define OS_PRI_COUNT 5

#define OS_IDLE_HOOK_PRIORITY 			0x55

extern void idle_hook();

/* Schedule Policies */
#define osSchPolicyROUND_ROBIN			0
#define osSchPolicyPRIORITY				1

/* objetos del idle hook */
uint32_t * idle_hook_sp;


typedef struct
{
    int32_t current_task; /* current_running_task */
    int32_t next_task; 	  /* next task to be run when there is a context change. */

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    int32_t main_delay_counter;
#endif
} tSched;

/* ********** OS PRIVATE INTERFACES ********** */

/* RETURNS THE REFERENCE TO THE TCB FOR THE "TASK" */
#define TASK_TCB(TASK)				(&TASK##_TCB)

/* RETURNS THE REFERENCE TO THE STACK FOR THE "TASK" */
#define TASK_STACK_TOP(TASK)		(&TASK##_stack[0])


#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
#define OS_TASKS_END_WITH_PRIO()	unsigned char PRIORITIES_COUNT[OS_PRI_COUNT];						 /* THIS ARRAY IS FILLED IN RUN TIME WITH THE AMOUNT OF TASKS THAT HAS OF EACH PRIO */ \
									TASK_COUNT_TYPE PRIO_TASKS[sizeof(os_tcbs)/sizeof(os_tcbs[0]) ];   /*  \
									DECLARE_PRIORITY_QUEUE(prioq, sizeof(os_tcbs)/sizeof(tTCB *) - 1, INVALID_TASK);*/
#else
#define OS_TASKS_END_WITH_PRIO()	;
#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
/* For this policy, the current task tcb index is the one stored in the array PRIO_TASKS.
 * Sched.current_task just stores the index of this vector.
 *  is configured, the current task index is directly stored in Sched.current_task*/
#define OS_CURRENT_TASK_TCB_INDEX		PRIO_TASKS[Sched.current_task]
#define OS_NEXT_TASK_TCB_INDEX			PRIO_TASKS[Sched.next_task]
#else
/* if round robin is configured, the current task index is directly stored in Sched.current_task*/
#define OS_CURRENT_TASK_TCB_INDEX		Sched.current_task
#define OS_NEXT_TASK_TCB_INDEX			Sched.next_task
//#define OS_CURRENT_TASK_TCB_INDEX_INVALID
#endif



#define OS_IDLE_TASK_INDEX	TASK_COUNT
#define TASK_COUNT_WIH		(TASK_COUNT+1)	//WITH IDLE HOOK

/* otras macros */
#ifdef _WIN32
#define OS_DISABLE_ISR()
#define OS_ENABLE_ISR()

#else
#define OS_DISABLE_ISR()	__asm volatile("cpsid f\n");
#define OS_ENABLE_ISR()		__asm volatile("cpsie f\n");

#endif

/* flags for task confoguration */
#define TASK_NOCONFIG	0x0000000
#define TASK_AUTOSTART	0x8000000

/* os methods declaration */
void osStart();
void osDelay( uint32_t delay_ms );
void idle_hook();



/* FOR USE IN USER FUNCTIONS AS SetEvent, <agragar las otras que se puedan usar. s> */
#define _I_TASK_REF(TASK)       TASK_TCB(TASK)

/* */


/* this declares a task that uses the same function name */
#define _I_DECLARE_TASK_R( NOMBRE , FCNNAME , ARG , STACKSIZE, PRI, CONFIG)                                             							\
																void FCNNAME(void*);							/* prototype for the FCN name */ 	\
                                                                uint32_t	NOMBRE##_stack[(STACKSIZE)/4];	    /* stack array  */        			\
																tTCB_Dyn    NOMBRE##_Din;		                /* Dynamic Part of each TCB  */    	\
																const tTCB  NOMBRE##_TCB =				        /* Fixed part of each TCB   */  	\
																{									 			        \
																	.entry_point = &FCNNAME,		   		            \
																	.arg		 = (ARG),							        \
																	.stackframe  = TASK_STACK_TOP(NOMBRE),	            \
																	.stacksize   = sizeof(NOMBRE##_stack),		        \
																	.pDin        = &NOMBRE##_Din, 				        \
																	.priority    = PRI, 						        \
																	.config      = CONFIG,						        \
																};



#define _I_OS_TASKS_START() \
                            DECLARE_TASK(  idle_hook , NULL , OS_IDLE_HOOK_STACK_SIZE , OS_IDLE_HOOK_PRIORITY  , TASK_NOCONFIG); \
							const tTCB *os_tcbs[] = {

#define _I_OS_ADD_TASK(TASK)						    TASK_TCB(TASK),

#define _I_OS_TASKS_END()	 							&idle_hook_TCB, \
													}; 				    \
							TASK_COUNT_TYPE TASK_COUNT = sizeof(os_tcbs)/sizeof(tTCB *) - 1 ;  /*the -1 is because the idle hook is the last in this array*/ \
							OS_TASKS_END_WITH_PRIO();

#endif /* OS_INTERNAL_H */


