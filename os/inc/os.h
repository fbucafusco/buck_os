/*
 * sched.h
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#ifndef SCHED_H_
#define SCHED_H_

#include "os_config.h"
#include "os_config_internal.h"
#include "board.h"

#define MAX_STACK_SIZE 				512
#define MIN_STACK_SIZE 				68

#define INVALID_TASK				-1

#define TASK_COUNT_TYPE				uint32_t	//TODO: PROPAGAR ESTA DEFINICION POR TODOS LADOS...
#define OS_PRIORITY_TYPE			unsigned char

#ifndef OS_IDLE_HOOK_STACK_SIZE
#define OS_IDLE_HOOK_STACK_SIZE 	MIN_STACK_SIZE
#endif

/* priority is higher when the value of the priority is the higher */
enum
{
    OS_PRI_LOWEST,
    OS_PRI_LOW,
    OS_PRI_MID,
    OS_PRI_HIGH,
    OS_PRI_HIGHEST,
    OS_PRI_COUNT
};


#define OS_IDLE_HOOK_PRIORITY 		0x55



extern void idle_hook();

/*tTaskSchedulePolicy*/
#define osSchPolicyROUND_ROBIN			0
#define osSchPolicyPRIORITY				1

/* objetos del idle hook */
uint32_t * idle_hook_sp;

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
    OS_PRIORITY_TYPE priority;						/* task priority */
#endif

    tTaskState 	state;	     					/* task state */
    uint32_t	delay;							/* counter for delay ticks */
} tTCB_Dyn;


typedef struct
{
    void 	( *entry_point )( void *arg ); /* pointer to the function associated with the task */
    void      *arg;								/* pointer to the argument for the ask  			*/
    uint32_t  *stackframe;						/* pointer to the array that stores the stack for the task */
    uint32_t   stacksize;						/* size of the stack array */
    tTCB_Dyn  *pDin;							/* pointer to the data representing the dynamic behavior of the task */
    uint32_t  config;							/* task configuration */
    OS_PRIORITY_TYPE	  priority;				/* task priority (ficed) or initial priority (dinamic) */
} tTCB;


typedef struct
{
    int32_t current_task; /* current_running_task */
    int32_t next_task; 	  /* next task to be run when there is a context change. */

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    int32_t main_delay_counter;
#endif
} tSched;


/* this declares a task that uses the same function name */
#define DECLARE_TASK_R( NOMBRE, FCNNAME , ARG , STACKSIZE, PRI, CONFIG)	uint32_t	NOMBRE##_stack[(STACKSIZE)/4];	\
																tTCB_Dyn 			NOMBRE##_Din;		            \
																const tTCB 			NOMBRE##_TCB =				    \
																{									 			\
																	.entry_point = &FCNNAME,		   		    \
																	.arg		 = ARG,							\
																	.stackframe  = &NOMBRE##_stack,				\
																	.stacksize   = sizeof(NOMBRE##_stack),		\
																	.pDin        = &NOMBRE##_Din, 				\
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
							TASK_COUNT_TYPE TASK_COUNT = sizeof(os_tcbs)/sizeof(tTCB *) - 1 ;  /*the -1 is because the idle hook is the last in this array*/ \
							OS_TASKS_END_WITH_PRIO();

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
