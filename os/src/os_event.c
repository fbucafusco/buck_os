/*
 * osEvent.c
 *
 *  Created on: 10/3/2017
 *      Author: franco.bucafusco
 */


/*
 * La mecanica de eventos será la siguiente:
 *
 * Un evento se puede enviar a una tarea o a todas.
 *
 * Una tarea puede esperar varios eventos a la vez. Si alguno ya estaba seteado, LA TAREA SIGUE SU EJECUCION.
 * Cuando se sale del wait, el evento que genero el desbloqueo, se borra del registro de eventos.
 *
 *
 * */


#include "os_event.h"
#include "os_internal.h"

extern const tTCB 		*os_tcbs[];
extern unsigned short 	TASK_COUNT;
extern tSched 			Sched;
extern TASK_COUNT_TYPE	PRIO_TASKS[];

/* set an evento to a task.
 * if the task is set to ready, it return 1, 0 otherwise. */
uint32_t _os_set_event_t( OS_EVENT_TYPE events , void *task_ref )
{
    tTCB *pTCB = ( tTCB * ) task_ref;
    uint32_t rv;

    OS_DISABLE_ISR();
    pTCB->pDin->events_setted = events;

    //tiene que esta bloqueada la tarea. si no esta bloqueada, no puede tener events_waiting != 0
    if( pTCB->pDin->events_waiting & events )
    {
        pTCB->pDin->events_waiting &= ~ events;

        //si la tarea estaba esperando el evento, entonces se pone en ready.

        _os_task_change_state( pTCB ,  osTskREADY );

        rv= 1;
    }
    else
    {
        rv= 0;
    }

    OS_ENABLE_ISR();

    return rv;
}

/*
 * Envia un evento a una tarea especifica.
 * TASK_REF(TASK)
   Llamar osSetEvent_T( EV1|EV2 , OS_TASK_REF(TASKNAME) );

    TASKNAME: FANTASY NAME
*/
void osSetEvent_T( OS_EVENT_TYPE events , void *task_ref )
{
    tTCB *pTCB = ( tTCB * ) task_ref;

    if( _os_set_event_t( events  , pTCB ) )
    {
        _os_schedule();
    }
}

void osSetEvent( OS_EVENT_TYPE events )
{
    TASK_COUNT_TYPE i;
    uint32_t count =0;

    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        count  &= _os_set_event_t( events , ( tTCB * ) os_tcbs[i] );
    }

    if( count )
    {
        _os_schedule();
    }
}

/*
 * espera cualquiera de los eventos definidos en "events".
 * Si el evento ya habia ocurrido para la tarea, la funcion no espera.
 * Devuelve el evento que destrabo la tarea.
 * */
OS_EVENT_TYPE osWaitEvent( OS_EVENT_TYPE events )
{
    //si se llama a esta funcion esta en running
    OS_EVENT_TYPE rv;

    OS_DISABLE_ISR();

    if( os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_setted &  events )
    {
        //no espera nada.
        //le borro los flags de setted

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_setted & events ;

        /* limpio setted */
        os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_setted  &= ~ rv;

        OS_ENABLE_ISR();

        _os_schedule();
    }
    else
    {
        os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_waiting = events;

        /* block the task and call schedluder()*/
        _os_task_block( OS_CURRENT_TASK_TCB_INDEX );

        OS_DISABLE_ISR();

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_waiting ^ events;

        /* limpio setted */
        os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->events_setted  &= ~ rv;

        OS_ENABLE_ISR();
    }

    return rv;

}
