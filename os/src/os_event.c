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
extern tTCB 			*os_sorted_Tcbs[];


/* external os functions */
extern void _os_task_block( tTCB *pTCB );
extern void _os_schedule();

/* set an evento to a task.
 * if the task is set to ready, it return 1, 0 otherwise. */
uint32_t _os_set_event_t( OS_EVENT_TYPE events , void *task_ref )
{
    tTCB *pTCB = ( tTCB * ) task_ref;
    uint32_t rv;

    OS_DISABLE_ISR();

    /* sets setted flags */
    pTCB->pDin->events_setted |= events;

    /* if the task is waiting the event (if events_waiting != 0 the task MUST be in tskBLOCKED state ) */
    if( pTCB->pDin->events_waiting & events )
    {
        /* clear waiting events. */
        pTCB->pDin->events_waiting &= ~ events;

        /* task becomes ready */
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

/* it sets an event to every task */
void osSetEvent( OS_EVENT_TYPE events )
{
    TASK_COUNT_TYPE i;
    uint32_t count = 0;

    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        count += _os_set_event_t( events , ( tTCB * ) os_tcbs[i] );
    }

    if( count )
    {
        _os_schedule();
    }
}

/* @brief Blocks the running task until an event hapens OR the timeout expires
 * @param[in] events: Events to wait
 * @param[in] timeout_ms: Timeout value in ms. If zero, there will be no wainting.
 *                        If just  Delay in ms. If zero, the will be no delay.
 *                        If OS_INFINITE the waiting will be forever, until the events happen
 *                        (is the same as calling osWaitEvent( OS_EVENT_TYPE events ) )
 * @return    Returns the flags that represents the event that happened. If a timeout happened, the return value will be zero.
 * */
OS_EVENT_TYPE osWaitEventT( OS_EVENT_TYPE events , OS_DELAY_TYPE timeout_ms )
{
    //es una mezcla de wait event con delay.
    //creo que es igual a wait event pero al final hay que verificiar que el delay sea cero, o no.
    //tambien si los eventos antes y despues del wait, siguen siendo los mismos...
    //y hacer lo que haya uq ehacer.
    //ver como devolver algo que indique si es timeout o no.

    //por ahora copio la rutina waitevetn y modifico. luego optimizo

    //si se llama a esta funcion esta en running
    OS_EVENT_TYPE rv;

    /* start critical section */
    OS_DISABLE_ISR();

    if( OS_CURRENT_TASK_TCB_REF->pDin->events_setted &  events || timeout_ms==0 )
    {
        /* some event was already setted.
         * The Task wont wait, BUT we call the schedule anyway in order to let hight priority tasks to be run.  */

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = OS_CURRENT_TASK_TCB_REF->pDin->events_setted & events ;

        /* limpio setted */
        OS_CURRENT_TASK_TCB_REF->pDin->events_setted  &= ~ rv;

        /* end critical section */
        OS_ENABLE_ISR();

        /* calls the schedule in order to evaluate if there is a ready higher priority task*/
        _os_schedule();
    }
    else
    {
        if( timeout_ms != 0 && timeout_ms != OS_INFINITE )
        {
            /* load the timeout in the register  */
            OS_CURRENT_TASK_TCB_REF->pDin->delay = timeout_ms;
        }

        OS_CURRENT_TASK_TCB_REF->pDin->events_waiting = events;

        /* block the task and call schedluder()*/
        _os_task_block( OS_CURRENT_TASK_TCB_REF );

        /* at this point there are 2 schenarios.
         * the delay ended, OR someone setted an event.
         * So if the delay field is 0 the delay ended and events_waiting ^ events is zero
         * or if events_waiting ^ events is zero !=0 and de delay != 0 then, an event happened.
         *
         * so, for evaluate this situation, we just have to evaluate one condition */

        OS_DISABLE_ISR();

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = OS_CURRENT_TASK_TCB_REF->pDin->events_waiting ^ events;

        /* if the user requested a timeout*/
        if( timeout_ms != 0 && timeout_ms != OS_INFINITE )
        {
            if( rv == 0 )
            {
                /* a timeout happened */
                /* clear waiting events (it simulates the SetEvent that didn't happen ) */
                OS_CURRENT_TASK_TCB_REF->pDin->events_waiting &= ~ events;
            }
            else
            {
                /* an evento happened */
                /* reset delay  (it simulates the Timeout that didn't happen )  */
                OS_CURRENT_TASK_TCB_REF->pDin->delay = 0;
            }
        }

        /* limpio setted */
        OS_CURRENT_TASK_TCB_REF->pDin->events_setted  &= ~ rv;

        OS_ENABLE_ISR();
    }

    return rv;
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

    /* start critical section */
    OS_DISABLE_ISR();

    if( OS_CURRENT_TASK_TCB_REF->pDin->events_setted &  events )
    {
        /* some event was already setted.
         * The Task wont wait, BUT we call the schedule anyway in order to let hight priority tasks to be run.  */

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = OS_CURRENT_TASK_TCB_REF->pDin->events_setted & events ;

        /* limpio setted */
        OS_CURRENT_TASK_TCB_REF->pDin->events_setted  &= ~ rv;

        /* end critical section */
        OS_ENABLE_ISR();

        /* calls the schedule in order to evaluate if there is a ready higher priority task*/
        _os_schedule();
    }
    else
    {
        OS_CURRENT_TASK_TCB_REF->pDin->events_waiting = events;

        /* block the task and call schedluder()*/
        _os_task_block( OS_CURRENT_TASK_TCB_REF );

        OS_DISABLE_ISR();

        /* genero el valor de salida indicando que evento fue el que destrabo la tarea. */
        rv = OS_CURRENT_TASK_TCB_REF->pDin->events_waiting ^ events;

        /* limpio setted */
        OS_CURRENT_TASK_TCB_REF->pDin->events_setted  &= ~ rv;

        OS_ENABLE_ISR();
    }

    return rv;
}

/* It clears all events in the running task */
void osClearEvents( OS_EVENT_TYPE events )
{
    OS_DISABLE_ISR();
    OS_CURRENT_TASK_TCB_REF->pDin->events_setted &= ~events;
    OS_ENABLE_ISR();
}
