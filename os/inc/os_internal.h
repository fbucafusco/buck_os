/*
 * sched.h
 *
 *  Created on: 22/2/2017
 *  Author: franco.bucafusco
 */

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "os_config.h"
#include "os_config_internal.h"
#include "os_tasks.h"
#include "os_event.h"
#include "board.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

#define MAX_STACK_SIZE 				512
#define MIN_STACK_SIZE 				68      // FIXME..... 68 ES EL STACK PARA GUARDAR EL CONTEXTO, PERO ADEMAS SE NECESITA STACK PARA LA ISR DE TICK, Y LA DE PEND SV. Y TODOS SUS LLAMADOS.

#define INVALID_TASK				-1


#ifndef OS_IDLE_HOOK_STACK_SIZE
#define OS_IDLE_HOOK_STACK_SIZE 	MIN_STACK_SIZE
#endif



#define OS_PRI_COUNT 	5

#define OS_IDLE_HOOK_PRIORITY 			0x55



/* Schedule Policies */
#define osSchPolicyROUND_ROBIN			0
#define osSchPolicyPRIORITY				1

/* objetos del idle hook */
uint32_t * idle_hook_sp;


typedef struct
{
    TASK_COUNT_TYPE active_tasks;	/* number of tasks that are active */
    TASK_COUNT_TYPE current_task; 	/* current_running_task */
    TASK_COUNT_TYPE next_task; 	  	/* next task to be run when there is a context change. */

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    int32_t main_delay_counter;
#endif
} tSched;

/* ********** OS PRIVATE INTERFACES ********** */

/* RETURNS THE REFERENCE TO THE TCB FOR THE "TASK" */
#define TASK_TCB(TASK)				(&(TASK##_TCB))

/* RETURNS THE REFERENCE TO THE STACK FOR THE "TASK" */
#define TASK_STACK_TOP(TASK)		(&TASK##_stack[0])

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
#define OS_TASKS_END_WITH_PRIO()	TASK_COUNT_TYPE PRIORITIES_COUNT[OS_PRI_COUNT];						 /* THIS ARRAY IS FILLED IN RUN TIME WITH THE AMOUNT OF TASKS THAT HAS OF EACH PRIO */ \
									tTCB *os_sorted_Tcbs[sizeof(os_tcbs)/sizeof(os_tcbs[0]) ];
#else
#define OS_TASKS_END_WITH_PRIO()	;
#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
/* For this policy, the current task tcb index is the one stored in the array os_sorted_Tcbs.
 * Sched.current_task just stores the index of this vector.
 *  is configured, the current task index is directly stored in Sched.current_task*/
#define OS_TASK_TCB_REF_(index)			os_sorted_Tcbs[(index)]
#define OS_CURRENT_TASK_TCB_REF			OS_TASK_TCB_REF_(Sched.current_task)
#define OS_NEXT_TASK_TCB_REF			os_sorted_Tcbs[Sched.next_task]
#else
/* if round robin is configured, the current task index is directly stored in Sched.current_task*/
#define OS_CURRENT_TASK_TCB_REF		Sched.current_task
#define OS_NEXT_TASK_TCB_REF			Sched.next_task
//#define OS_TASK_TCB_REF_INVALID
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


/* FOR USE IN USER FUNCTIONS AS SetEvent, <agragar las otras que se puedan usar. s> */
#define _I_TASK_REF(TASK)       TASK_TCB(TASK)

/* */

/* this declares a task that uses the same function name */
#define _I_DECLARE_TASK_R_( NOMBRE , FCNNAME , ARG , STACKSIZE, PRI, CONFIG, MEM)                   														\
																uint32_t	NOMBRE##_stack[(STACKSIZE)/4];	    /* stack array  */        			\
																tTCB_Dyn    NOMBRE##_Din;		                /* Dynamic Part of each TCB  */    	\
																MEM tTCB  NOMBRE##_TCB =				        /* Fixed part of each TCB   */  	\
																{									 			        \
																	.entry_point = &FCNNAME,		   		            \
																	.arg		 = (ARG),						        \
																	.stackframe  = TASK_STACK_TOP(NOMBRE),	            \
																	.stacksize   = sizeof(NOMBRE##_stack),		        \
																	.pDin        = &NOMBRE##_Din, 				        \
																	.def_priority= PRI, 						        \
																	.config      = CONFIG,						        \
																};														\
																MEM tTCB* NOMBRE = &(NOMBRE##_TCB);


/* this declares a task that uses the same function name WITH THE FUNCTION PROTOTIPE*/
#define _I_DECLARE_TASK_R( NOMBRE , FCNNAME , ARG , STACKSIZE, PRI, CONFIG)                                             							      \
																	    extern void FCNNAME(void*);					    /* prototype for the FCN name */  \
																		_I_DECLARE_TASK_R_(NOMBRE , FCNNAME , ARG , STACKSIZE, PRI, CONFIG, const )

#define _I_OS_TASKS_START() \
							_I_DECLARE_TASK_R(  idle_hook_ , idle_hook,  NULL , OS_IDLE_HOOK_STACK_SIZE , OS_IDLE_HOOK_PRIORITY , TASK_AUTOSTART ); \
							const tTCB *os_tcbs[] = {

#define _I_OS_ADD_TASK(TASK)						    &(TASK##_TCB),

#define _I_OS_TASKS_END()	 							&idle_hook__TCB, \
													}; 				    \
							TASK_COUNT_TYPE TASK_COUNT = sizeof(os_tcbs)/sizeof(tTCB *) - 1 ;  /*the -1 is because the idle hook is the last in this array*/ \
							OS_TASKS_END_WITH_PRIO();

#endif /* OS_INTERNAL_H */


