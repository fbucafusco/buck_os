/*
 * os_tasks.c
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */


#include "os_internal.h"
#include <string.h>

/* external os objects */
extern const tTCB 	   *os_tcbs[];
extern tSched 			Sched;
extern tTCB *			os_sorted_Tcbs[];
extern TASK_COUNT_TYPE 	PRIORITIES_COUNT[OS_PRI_COUNT];
extern unsigned short 	TASK_COUNT;

/* external os functions */
extern void _os_task_block( tTCB *pTCB );
extern void _os_schedule();
extern void _os_pp_sort_prio_array();
extern TASK_COUNT_TYPE _os_pp_sort_prio_array_ti( TASK_COUNT_TYPE index );
extern void _os_trigger_cc_conditional( tSched *pSched  );
extern TASK_COUNT_TYPE _os_get_next( TASK_COUNT_TYPE current_task );
/* idle_hook
 * es llaado cuando el OS no tiene que ejecutar ninguna tarea. */
__attribute__( ( weak ) ) void return_hook()
{
    while( 1 )
    {

    };
}

/*
 * cambia el estado de la tarea
 * debe llamarse dentro de una zona critica.
 * */
void _os_task_change_state( tTCB *pTCB ,  tTaskState state )
{
    pTCB->pDin->state = state;
}


/* bloquea la tarea "index" , y selecciona la siguiente para pasar a running.
 * deben estar deshabilitadas las isr... */
void _os_task_block( tTCB *pTCB )
{
    /* the task goes to blocking state */
    _os_task_change_state( pTCB  , osTskBLOCKED );

    /* end critical section */
    OS_ENABLE_ISR();

    /* call the scheduler */
    _os_schedule();
}





void _os_tcb_init_stack( tTCB *pTcb )
{
    uint32_t * stackframe_;
    uint32_t   stacksize_;

    /* inicializo el stack en cero */
    memset( pTcb->stackframe , 0 , pTcb->stacksize );

    /* cargo en variables locales para lectura mas amena */
    stackframe_ = pTcb->stackframe;
    stacksize_  = pTcb->stacksize;

    /* armo el frame inicial */
    stackframe_[stacksize_/4-1] = 1<<24; 							/* xPSR.T = 1 */
    stackframe_[stacksize_/4-2] = ( uint32_t ) pTcb->entry_point; 	/* PC */
    stackframe_[stacksize_/4-3] = ( uint32_t ) return_hook; 		/* LR */

    stackframe_[stacksize_/4-8] = ( uint32_t ) pTcb->arg; 			/* R0 <- arg */
    stackframe_[stacksize_/4-9] = 0xFFFFFFF9; 				    	/* EXC_RETURN _ como apila 1ro el LR */

    /* guardo el stackpointer en tcb*/
    pTcb->pDin->sp = stackframe_ + stacksize_/4 - 17; 		        /* sp inicial  el 17 es porque pusheo de r4 a r11 + lr   */

}

/* it sets the task and puts in not active state,
 * just task logic
 * NOT THREAD SAFE
 *
 * params relative_index: absolute index within os_tcbs array */
void _os_task_not_active_( tTCB *pTCB )
{


    /* */
    pTCB->pDin->state = osTskNOT_ACTIVE;
}

/* it starts one task and puts it in ready,
 * just task logic
 * NOT THREAD SAFE
 *
 * params pTCB: puntero a un TCB */
void _os_task_active( tTCB *pTCB )
{
    /* put the task in ready state */
    _os_task_change_state( pTCB ,  osTskREADY );

    /* events init */
    pTCB->pDin->events_waiting = 0;					/* events_waiting:  */
    pTCB->pDin->events_setted	= 0;					/* events_setted:  */

    /* the delay is zero */
    pTCB->pDin->delay = 0;

    /* init the mutex pointer */
    pTCB->pDin->pWainingMut = NULL;

    /* inicializo el stack */
    _os_tcb_init_stack( pTCB );
}

/* it starts one task and puts it in ready */
void osTaskStart( const tTCB *pTCB )
{
    //Verificar que la tarea no este arrancada
    //si no esta arrancada, hay que inicializar la estructura.
    //respecto de las prioridades: si no esta activa en el array os_sorted_Tcbs hay elementos al final (por lo menos 1)
    //en INVALID_TASK entonces, SIEMPRE se agrega la tarea nueva alli. Luego, se ordena.

    OS_DISABLE_ISR();

    /* it only starts the task if not active */
    if( pTCB->pDin->state == osTskNOT_ACTIVE )
    {
        /* initialize task data (without prio objects) */
        _os_task_active( pTCB );

        Sched.active_tasks++;

        /* manage the priority algoritms objects */
#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
        /* copy the defauult prio to the current_priority location */
        pTCB->pDin->current_priority = pTCB->def_priority;

        /* we add the new task to os_sorted_TCBs. If the task wasn't active, it wont be here
         * at the beginning. */
        os_sorted_Tcbs[TASK_COUNT-1] = pTCB ;

        /* we increment the task count for the priority */
        PRIORITIES_COUNT[pTCB->pDin->current_priority]++;

        /* se ordena el array de prioridades haciendo tracking de la tarea next */
        Sched.current_task = _os_pp_sort_prio_array_ti( Sched.current_task ) ;
#endif

        OS_ENABLE_ISR();

        _os_schedule();
    }
    else
    {
        OS_ENABLE_ISR();
    }
}

/* termina la tarea que esta corriendo*/
void osTaskEnd()
{
    OS_DISABLE_ISR();

    /*but the task in ready state */
    _os_task_change_state( OS_CURRENT_TASK_TCB_REF ,  osTskNOT_ACTIVE );

    Sched.active_tasks--;

    /* first we calculate the next task AS THIS TASK IS NOT DEAD */
    TASK_COUNT_TYPE next = _os_get_next( Sched.current_task );

    /* */

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* remove the task from the priority count array */
    PRIORITIES_COUNT[OS_CURRENT_TASK_TCB_REF->def_priority]--;

    /* kills the task from os_sorted_Tcbs array */
    os_sorted_Tcbs[Sched.current_task] = NULL ;

    /* se ordena el array de prioridades haciendo tracking de la tarea next */
    next = _os_pp_sort_prio_array_ti( next ) ;
#endif

    Sched.current_task = INVALID_TASK; /* esto va a reiniciar el RR en la misma prioridad de la tarea que se va. */
    Sched.next_task    = next;

    OS_ENABLE_ISR();

    /* triggers cc*/
    _os_trigger_cc_conditional( &Sched  );
    /* call the scheduler */
}



/*  */
void _osGetTaskId( tTCB *pTCB  )
{

}

/* cede el CPU a otra tarea de la misma prioridad o mayor. Si no hay otra, el schedule
 * volvera a darle el CPU a la tarea llamante. */
void osTaskYield()
{
    OS_DISABLE_ISR();

    /*but the task in ready state */
    _os_task_change_state( OS_CURRENT_TASK_TCB_REF ,  osTskREADY );
    OS_ENABLE_ISR();

    /* call the scheduler */
    _os_schedule();
}

/*
 *
 */
//no se usa.

