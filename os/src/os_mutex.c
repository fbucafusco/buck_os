/*
 * os_mutex.c
 *
 *  Created on: 16/3/2017
 *      Author: franco.bucafusco
 */

#include "os_defs.h"
#include "os_internal.h"

/* external os objects */
extern const tTCB 		*os_tcbs[];
extern unsigned short 	TASK_COUNT;
extern tSched 			Sched;
extern tTCB *	os_sorted_Tcbs[];

/* external os functions */
extern void _os_task_block( tTCB *pTCB );
extern void _os_schedule();
extern OS_PRIORITY_TYPE _os_pp_get_next_prio( OS_PRIORITY_TYPE curr_prio );
extern void _os_pp_change_task_priority( tTCB *pTCB , OS_PRIORITY_TYPE newpriority );
extern void _os_pp_restore_task_priority( tTCB *pTCB );


/*
 * el mutex lo tiene que tomar y liberar la misma tarea.
 * */
void osMutexInit( OS_MUTEX *pM )
{
    pM->counter 		= 0;
    pM->owner_task 		= NULL;
}


void _os_mutex_block_if_taken( OS_MUTEX *pM )
{
    if( pM->counter == 0 || ( pM->owner_task == OS_TASK_TCB_REF_( Sched.current_task ) ) )
    {
        /* not taken or taken by the running task */
    }
    else
    {
        /* The task will wait the mutex.
         * If the owner has less priority than the running task, the owner task priority is risen above the running task priority (or equal).
        */
        OS_PRIORITY_TYPE running_task_prio	= OS_TASK_TCB_REF_( Sched.current_task )->pDin->current_priority;
        OS_PRIORITY_TYPE owner_task_prio	= ( ( tTCB * )pM->owner_task )->pDin->current_priority;

        if( owner_task_prio < running_task_prio )
        {
            /* calculate new priority */
            OS_PRIORITY_TYPE new_prio = _os_pp_get_next_prio( OS_CURRENT_TASK_TCB_REF->pDin->current_priority ) ;

            _os_pp_change_task_priority( pM->owner_task , new_prio );
        }

        /* init the mutex pointer (this task is going to take the mutex when is released) */
        OS_CURRENT_TASK_TCB_REF->pDin->pWainingMut = pM;

        /* block the running task */
        _os_task_block( OS_CURRENT_TASK_TCB_REF );

        /* en este punto, la tarea se desbloqueo porque alguien hizo release del mutex.
         * entonces, ya estoy seguro de que el mutex NO esta tomado por otra.
         * entonces se sigue el flujo obligatorio como si se hubiera entrado a la funcion nuevamente,
         * con el primer if en true. */

        OS_DISABLE_ISR();
    }
}


void _os_mutex_take( OS_MUTEX *pM )
{
    pM->counter++;
    pM->owner_task = OS_TASK_TCB_REF_( Sched.current_task );

    /* init the mutex pointer */
    ( ( tTCB * )pM->owner_task )->pDin->pWainingMut = pM;

    OS_ENABLE_ISR();

    /* call the scheduler */
    _os_schedule();
}


/*
 * Espera que se liberere el mutex. Cuando se libera, lo toma. Si ya estaba tomado, la tarea
 * se bloquea.
 * */
void osMutexWait( OS_MUTEX *pM )
{
    OS_DISABLE_ISR();

    _os_mutex_block_if_taken( pM );

    _os_mutex_take( pM );
}

void osMutexRelease( OS_MUTEX *pM )
{
    TASK_COUNT_TYPE i;

    if( pM->owner_task == OS_TASK_TCB_REF_( Sched.current_task )  )
    {
        pM->counter--;

        if( pM->counter==0 )
        {
            /* init the mutex pointer */
            ( ( tTCB * )pM->owner_task )->pDin->pWainingMut = NULL;

            /* restores the original priority */
            _os_pp_restore_task_priority( pM->owner_task );

            pM->owner_task = NULL;

            /* busco alguna otra tarea que este esperando el mutex.
             * Si encuentro, entonces, pongo en ready la tarea.
             * Se busca de mayor a menor prioridad */

            for( i=0; i<Sched.active_tasks; i++ )
            {
                if( OS_TASK_TCB_REF_( i )->pDin->pWainingMut == pM )
                {
                    /* */
                    OS_TASK_TCB_REF_( i )->pDin->state = osTskREADY;
                    break;
                }
            }

            _os_schedule();
        }
    }
    else
    {

    }
}
