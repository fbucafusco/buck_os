/*
 * os_tasks.c
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */


#include "os_internal.h"

extern const tTCB *os_tcbs[];

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
void _os_task_block( TASK_COUNT_TYPE index )
{
    /* the task goes to blocking state */
    _os_task_change_state( os_tcbs[index]  , osTskBLOCKED );

    OS_ENABLE_ISR();

    /* call the scheduler */
    _os_schedule();
}

/*  */
void _osGetTaskId( tTCB *pTCB  )
{

}



/*
 *
 */
//no se usa.

