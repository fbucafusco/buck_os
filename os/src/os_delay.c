/*
 * os_delay.c
 *
 *  Created on: 10/3/2017
 *      Author: franco.bucafusco
 */

#include "os.h"
#include "os_tasks.h"
#include "os_delay.h"

/* external os objects */
extern tSched Sched;
extern const tTCB *os_tcbs[];
extern unsigned short TASK_COUNT;
extern tTCB * os_sorted_Tcbs[];

/* external os functions */
extern void _os_task_block( tTCB *pTCB );

/* in a tick, it checkes the delays */
void _os_delay_update()
{
    uint32_t i;

    OS_DISABLE_ISR();

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    uint32_t min_remain = 0xFFFFFFFF;

    /* en este caso, el campo delay de los tcbs, identificará el reminder posterior al timeout principal */

    if( Sched.main_delay_counter > 0 )
    {
        /* we reduce the counter */
        Sched.main_delay_counter--;

        if( Sched.main_delay_counter==0 )
        {
            //timeout: de uno o varios delays

            for( i=0 ; i<TASK_COUNT ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskBLOCKED  )
                {
                    if( os_tcbs[i]->pDin->delay == 0 )
                    {
                        //si el reminder es cero, entonces dio timeout esta tarea, y debe volver a ready.
                        os_tcbs[i]->pDin->state  = osTskREADY;
                    }
                    else
                    {
                        //busco el minimo de los remainders (os_tcbs[i]->pDin->delay)
                        if( os_tcbs[i]->pDin->delay < min_remain )
                        {
                            min_remain = os_tcbs[i]->pDin->delay;
                        }
                    }
                }
            }

            /* para todas las tareas de remain mayor a cero, recalculo el remain. */
            for( i=0 ; i<TASK_COUNT ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskBLOCKED && os_tcbs[i]->pDin->delay > 0 )
                {
                    os_tcbs[i]->pDin->delay -= min_remain ;
                }
            }

            Sched.main_delay_counter = min_remain;
        }

    }
#else
    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        if( os_tcbs[i]->pDin->state == osTskBLOCKED && os_tcbs[i]->pDin->delay > 0 )
        {
            os_tcbs[i]->pDin->delay--;

            /* if the task delay counter went to 0 */
            if( os_tcbs[i]->pDin->delay > 0  )
            {

            }
            else
            {
                /* the task goes to ready state */
                os_tcbs[i]->pDin->state  = osTskREADY;
            }
        }
    }
#endif

    OS_ENABLE_ISR();
}


/* @brief implements a blocking delay for the current running task
 * @param[in] delay_ms: Delay in ms. If zero, the will be no delay.
 * */
void osDelay( OS_DELAY_TYPE delay_ms )
{
    OS_DELAY_TYPE ticks = delay_ms; //TODO: AGREGAR FACTOR DE ESCALA.

    if( delay_ms!= 0 )
    {
        OS_DISABLE_ISR();

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1 )
        uint32_t min_remain;
        uint32_t i;

        if( delay_ms >= Sched.main_delay_counter )
        {
            /* load the delay in the register  */
            OS_CURRENT_TASK_TCB_REF->pDin->delay = delay_ms;

            min_remain = _os_get_min_remain();	//va a haber un minimo porque arriba ya se actualizo

            OS_CURRENT_TASK_TCB_REF->pDin->delay  -= min_remain;

            Sched.main_delay_counter = min_remain;
        }
        else
        {
            uint32_t dif = Sched.main_delay_counter - delay_ms;


            /* load the delay in the register  */
            OS_CURRENT_TASK_TCB_REF->pDin->delay = 0;

            /* a todos los otros remains, calculados con el tiempo parcial anterior se van a incrementar
             * en la dif */
            for( i=0 ; i<TASK_COUNT ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskBLOCKED  )
                {
                    if( i != OS_CURRENT_TASK_TCB_REF )
                    {
                        os_tcbs[i]->pDin->delay += dif;
                    }

                }
            }

            Sched.main_delay_counter = delay_ms;
        }

#else
        /* load the delay in the register  */
        OS_CURRENT_TASK_TCB_REF->pDin->delay = delay_ms;
#endif

        /* blockde task(it calls  the scheduler) */
        _os_task_block( OS_CURRENT_TASK_TCB_REF );
    }
}

