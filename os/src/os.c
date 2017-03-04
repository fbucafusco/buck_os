/*
 * sched.c
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#include "os.h"
#include "chip.h"

/* external objects thart are defined by the user */
extern const tTCB *os_tcbs[];
extern unsigned short TASK_COUNT;

tSched Sched;

/* idle_hook
 * es llaado cuando el OS no tiene que ejecutar ninguna tarea. */
__attribute__( ( weak ) ) void idle_hook()
{
    while( 1 )
    {

    };
}

/* idle_hook
 * es llaado cuando el OS no tiene que ejecutar ninguna tarea. */
__attribute__( ( weak ) ) void return_hook()
{
    while( 1 )
    {

    };
}


#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
/* politica de scheduling:
 * en cada tick se hace cambio de contexto a la tarea siguiente.
 * es un roundrobin de 1 ms.
 * */

/* busca en los TCBs desde el indice "from" hasta "to" (no inclusive) */
uint32_t osRR_SearchReadyTask( uint32_t from, uint32_t to )
{
    char i;

    for( i = from ; i < to ; i++ )
    {
        if( os_tcbs[i]->pDin->state == osTskREADY )
        {
            return i ;
        }
    }

    return INVALID_TASK;
}



#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
/* politica de scheduling:
 * en cada tick se hace cambio de contexto a la tarea siguiente.
 * es un roundrobin de 1 ms.
 * */


#endif


/* os_schedule() SOLO debe definir quien es la siguiente tarea a ejecutarse (Sched.next_task)
* No debe modificar el estado de ninguna tarea.
* */
void os_schedule()
{
    uint32_t next;

    OS_DISABLE_ISR();

    if( Sched.current_task == INVALID_TASK || Sched.current_task == OS_IDLE_TASK_INDEX )
    {
#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
        /* busco la primera tarea en ready */
        next = osRR_SearchReadyTask( 0 , TASK_COUNT );
#endif
        if( next == INVALID_TASK )
        {
            //no hay ninguna ready

            /* TENGO QUE EJECUTAR IDLE_TASK.... */
            Sched.next_task = OS_IDLE_TASK_INDEX;
        }
        else
        {
            // Ok! encontre una ready
            Sched.next_task = next;
        }
    }
    else
    {

#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
        /* busco la siguiente tarea en ready */
        next = osRR_SearchReadyTask(  Sched.current_task + 1 , TASK_COUNT );

        if( next == INVALID_TASK )
        {
            //no encontro
            next = osRR_SearchReadyTask( 0 , Sched.current_task );
        }
#endif

        if( next == INVALID_TASK )
        {
            /* evaluo que paso con current task
               puede estar en running, por lo que la dejo asi.
               puede estar bloqueada: En este caso, la proxima tarea va a ser idle, porque la bloqueada y las otras
               no estan ready.  */

            if( os_tcbs[Sched.current_task]->pDin->state==osTskBLOCKED )
            {
                Sched.next_task = OS_IDLE_TASK_INDEX;
            }
        }
        else
        {
            //ok! encontre una ready
            Sched.next_task = next;
        }
    }

    uint32_t condicion_cc = ( Sched.next_task != Sched.current_task && Sched.next_task != INVALID_TASK );

    OS_ENABLE_ISR();

    if( condicion_cc )
    {
        os_trigger_cc();
    }
}

uint32_t dummy_pend = 0;

void os_trigger_cc()
{
    OS_DISABLE_ISR();
    dummy_pend=1;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __ISB(); //INSTRUCTION SYNCHRONIZATION BARRIER
    __DSB(); //DATA SYNCHRONIZATION BARRIER
    OS_ENABLE_ISR();
}


/* funcion llamada por pendsv para obtener el nuevo contexto.*/
uint32_t *os_get_next_context( uint32_t *actualcontext )
{
    uint32_t *rv;

    dummy_pend=0;

    //primero veo si vengo del stackframe principal.

    if( Sched.current_task == INVALID_TASK )
    {
        //caso en que se quiere cambiar contexto desde el stackframe principal
        //la primera ejecucion o viniendo de idle hook
        if( Sched.next_task != INVALID_TASK )
        {
            Sched.current_task = Sched.next_task;

            Sched.next_task    = INVALID_TASK;

            os_tcbs[Sched.current_task]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = os_tcbs[Sched.current_task]->pDin->sp;

            return rv;
        }
    }
    else
    {
        if( Sched.next_task != INVALID_TASK )
        {
            if( os_tcbs[Sched.current_task]->pDin->state==osTskRUNNING )
            {
                os_tcbs[Sched.current_task]->pDin->state = osTskREADY;	/* change the state of the current task to ready */
            }

            os_tcbs[Sched.current_task]->pDin->sp = actualcontext;

            Sched.current_task = Sched.next_task;
            Sched.next_task    = INVALID_TASK;

            os_tcbs[Sched.current_task]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */
            rv = os_tcbs[Sched.current_task]->pDin->sp;

            return rv;
        }
    }

    return actualcontext;
}

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
/* devuelve el minimo remain de los delays de las tareas. */
uint32_t os_get_min_remain()
{
    uint32_t i;
    uint32_t min_remain = 0xFFFFFFFF;

    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        if( os_tcbs[i]->pDin->state == osTskBLOCKED && os_tcbs[i]->pDin->delay > 0 )
        {
            if( os_tcbs[i]->pDin->delay < min_remain )
            {
                min_remain = os_tcbs[i]->pDin->delay;
            }
        }
    }

    if( Sched.main_delay_counter < min_remain && Sched.main_delay_counter!= 0 )
    {
        min_remain = Sched.main_delay_counter;
    }

    return min_remain;
}
#endif

/* implements a blocking delay for the current running task */
void osDelay( uint32_t delay_ms )
{
    uint32_t ticks = delay_ms; //TODO: AGREGAR FACTOR DE ESCALA.

    OS_DISABLE_ISR();

    if( delay_ms!= 0 )
    {
        /* the task goes to blocking state */
        os_tcbs[Sched.current_task]->pDin->state  = osTskBLOCKED;

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
        uint32_t min_remain;
        uint32_t i;

        if( delay_ms >= Sched.main_delay_counter )
        {
            /* load the delay in the register  */
            os_tcbs[Sched.current_task]->pDin->delay = delay_ms;

            min_remain = os_get_min_remain();	//va a haber un minimo porque arriba ya se actualizo

            os_tcbs[Sched.current_task]->pDin->delay  -= min_remain;

            /*      for( i=0 ; i<TASK_COUNT ; i++ )
                  {
                   //resto a todas los remains, el valor del minimo
                      if( os_tcbs[i]->pDin->state == osTskBLOCKED   )
                      {
                          if( os_tcbs[i]->pDin->delay > 0 )
                          {
                              os_tcbs[i]->pDin->delay -= min_remain ;
                          }
                      }
                  }*/

            Sched.main_delay_counter = min_remain;
        }
        else
        {
            uint32_t dif = Sched.main_delay_counter - delay_ms;


            /* load the delay in the register  */
            os_tcbs[Sched.current_task]->pDin->delay = 0;

            /* a todos los otros remains, calculados con el tiempo parcial anterior se van a incrementar
             * en la dif */
            for( i=0 ; i<TASK_COUNT ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskBLOCKED  )
                {
                    if( i != Sched.current_task )
                    {
                        os_tcbs[i]->pDin->delay += dif;
                    }

                }
            }

            Sched.main_delay_counter = delay_ms;
        }



#else
        /* load the delay in the register  */
        os_tcbs[Sched.current_task]->pDin->delay = delay_ms;

#endif

        /* call the scheduler */
        os_schedule();
    }
    OS_ENABLE_ISR();
}



/* in a tick, it checkes the delays */
void os_check_timeouts()
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
                os_tcbs[i]->pDin->delay  = 0; //redundante
            }
        }
    }
#endif
    OS_ENABLE_ISR();
}

void osStart()
{
    Board_Init();

    SystemCoreClockUpdate();

    /* habilito la interrupcion de pendable service */
    NVIC_SetPriority( PendSV_IRQn, ( 1 << __NVIC_PRIO_BITS ) - 1 );

    /**/
    Sched.current_task 			= INVALID_TASK;
    Sched.next_task    			= INVALID_TASK;

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    Sched.main_delay_counter 	= 0;
#endif

    TASK_COUNT_TYPE i;
    uint32_t * stackframe_;
    uint32_t   stacksize_;

    /* for each task, initialize the stack */
    for( i=0 ; i<TASK_COUNT_WIH ; i++ )
    {
        /* inicializo el estado de la tarea */
        if( os_tcbs[i]->config & TASK_AUTOSTART )
        {
            os_tcbs[i]->pDin->state = osTskREADY;
        }
        else
        {
            os_tcbs[i]->pDin->state = osTskNOT_ACTIVE;
        }

        /* the delay is zero */
        os_tcbs[i]->pDin->delay = 0;

        /* inicializo el stack en cero */
        bzero( os_tcbs[i]->stackframe , os_tcbs[i]->stacksize );

        /* cargo en variables locales para lectura mas amena */
        stackframe_ = os_tcbs[i]->stackframe;
        stacksize_  = os_tcbs[i]->stacksize;

        /* armo el frame inicial */
        stackframe_[stacksize_/4-1] = 1<<24; 									/* xPSR.T = 1 */
        stackframe_[stacksize_/4-2] = ( uint32_t ) os_tcbs[i]->entry_point; 	/* PC */
        stackframe_[stacksize_/4-3] = ( uint32_t ) return_hook; 			    /* LR */

        stackframe_[stacksize_/4-8] = ( uint32_t ) os_tcbs[i]->arg; 			/* R0 <- arg */
        stackframe_[stacksize_/4-9] = 0xFFFFFFF9; 				    			/* como apila 1ro el LR */

        /* guardo el stackpointer en tcb*/
        os_tcbs[i]->pDin->sp = stackframe_ + stacksize_/4 - 17; 		        /* sp inicial  el 17 es porque pusheo de r4 a r11 + lr   */
    }

    /*inicio el tick*/
    SysTick_Config( SystemCoreClock/1000 );

    while ( 1 )
    {

    }
}

/* handler de systick
 * aqui se decide por la tarea siguiente. */
void SysTick_Handler()
{
    os_check_timeouts();
    os_schedule();


}
