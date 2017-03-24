/*
 * os_mutex.c
 *
 *  Created on: 16/3/2017
 *      Author: franco.bucafusco
 */
//#include "os_mutex.h"

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
extern void _os_pp_change_task_priority( TASK_COUNT_TYPE index, OS_PRIORITY_TYPE newpriority );

/*
 * el mutex lo tiene que tomar y liberar la misma tarea.
 * */

void osMutexInit( OS_MUTEX *pM )
{
    pM->counter 		= 0;
    pM->owner_task 		= INVALID_TASK;
}


/*
 * Espera que se liberere el mutex. Cuando se libera, lo toma. Si ya estaba tomado, la tarea
 * se bloquea.
 * */
void osMutexWait( OS_MUTEX *pM )
{
    OS_DISABLE_ISR();

    if( pM->counter == 0 || ( pM->owner_task == Sched.current_task ) )
    {
        //no esta tomado.
    }
    else
    {
        /* esta tomado:
         * Entonces como quiero esperarlo, hago dos cosas:
         * - Si el owner es de mas baja prioridad que el qu quiere tomar la tarea,
         *   se debe elevar la prioridad de la tarea owner, para que lo largue lo antes posible. */
        OS_PRIORITY_TYPE running_task_prio	= OS_CURRENT_TASK_TCB_REF->pDin->current_priority;
        OS_PRIORITY_TYPE owner_task_prio	= os_tcbs[pM->owner_task]->pDin->current_priority;

        if( owner_task_prio < running_task_prio )
        {
            /* calculate new priority */
            OS_PRIORITY_TYPE new_prio = _os_pp_get_next_prio( OS_CURRENT_TASK_TCB_REF->pDin->current_priority ) ;

            _os_pp_change_task_priority( pM->owner_task , new_prio );
        }

        //bloqueo a running task
        _os_task_block( OS_CURRENT_TASK_TCB_REF );

        /* en este punto, la tarea se desbloqueo porque alguien hizo release del mutex.
         * entonces, ya estoy seguro de que el mutex NO esta tomado por otra.
         * entonces se sigue el flujo obligatorio como si se hubiera entrado a la funcion nuevamente,
         * con el primer if en true. */

        OS_DISABLE_ISR();
    }

    //no esta tomado
    pM->counter++;
    pM->owner_task = Sched.current_task;

    /* init the mutex pointer */
    os_tcbs[pM->owner_task]->pDin->pWainingMut = pM;

    OS_ENABLE_ISR();

    /* call the scheduler */
    _os_schedule();
}

void osMutexRelease( OS_MUTEX *pM )
{
    TASK_COUNT_TYPE i;

    if( pM->owner_task == Sched.current_task )
    {
        pM->counter--;

        if( pM->counter==0 )
        {
            /* init the mutex pointer */
            os_tcbs[pM->owner_task]->pDin->pWainingMut = NULL;

            /* restores the original priority*/
            _os_pp_restore_task_priority( pM->owner_task );

            pM->owner_task = INVALID_TASK;

            /* busco alguna otra tarea que este esperando el mutex.
             * Si encuentro, entonces, pongo en ready la tarea.
             * Se busca de mayor a menor prioridad */

            for( i=0; i<TASK_COUNT; i++ )
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
