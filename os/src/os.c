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

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
extern TASK_COUNT_TYPE PRIORITIES_COUNT[OS_PRI_COUNT];
extern TASK_COUNT_TYPE PRIO_TASKS[];
#endif


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


/* prdena las tareas por pioridades.
 * deja en PRIO_TASKS los indices de tcbs de las mas prioritarias a las menos. */
void osPP_SortPrioArray()
{
    TASK_COUNT_TYPE i, j ;
    TASK_COUNT_TYPE swap;

    for( i=0; i< TASK_COUNT-1; i++ )
    {
        for( j=0; j< TASK_COUNT-i-1; j++ )
        {
            if( os_tcbs[PRIO_TASKS[j]]->priority < os_tcbs[PRIO_TASKS[j+1]]->priority )
            {
                swap       		= PRIO_TASKS[j+1];
                PRIO_TASKS[j+1] = PRIO_TASKS[j];
                PRIO_TASKS[j] 	= swap;
            }
        }
    }
}

/*
 *
 */
//no se usa.
void os_change_task_state( TASK_COUNT_TYPE index ,  tTaskState state )
{
#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* solo para el modo de scheduling, hay que */
    if( state == osTskREADY )
    {
        //agregar a la cola de ready
        //pq_Add( &prioq,  index, os_tcbs[index]->priority );
    }
    else
    {
        //quitar de la cola de ready
        //pq_Remove( &prioq, index );
    }
#endif

    os_tcbs[index]->pDin->state = state;
}


#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
/* politica de scheduling:
 * en cada tick se hace cambio de contexto a la tarea siguiente.
 * es un roundrobin de 1 ms.
 * */

/* busca en los TCBs desde el indice "from" hasta "to" (no inclusive) */
uint32_t osRRP_SearchReadyTask( uint32_t from, uint32_t to )
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
 * se busca la tarea en ready mas prioritaria
 * */


/* no se usa */
void osPP_RotateSubVector( TASK_COUNT_TYPE inicio, TASK_COUNT_TYPE fin )
{
    TASK_COUNT_TYPE i;
    TASK_COUNT_TYPE back;

    back = PRIO_TASKS[inicio];

    for( i=inicio ; i < fin-1 ; i++ )
    {
        PRIO_TASKS[i] = PRIO_TASKS[i+1];
    }

    PRIO_TASKS[i] = back;

}

/* busca, en PRIO_TASK la primera de mas prioridad en ready. */
uint32_t osPP_SearchReadyTask_Coop()
{
    TASK_COUNT_TYPE i;

    for( i = 0 ; i < TASK_COUNT ; i++ )
    {
        if( os_tcbs[ PRIO_TASKS[i] ]->pDin->state == osTskREADY )
        {
            return i ;
        }
    }

    return INVALID_TASK;
}

/* busca en los TCBs desde el indice 0 a TASK_COUNT (no inclusive)
 * las tareas ready mas prioritarias */
uint32_t osPP_SearchReadyTask(  )
{
    TASK_COUNT_TYPE i;
    TASK_COUNT_TYPE j;
    TASK_COUNT_TYPE i_max;
    TASK_COUNT_TYPE i_0 ;

    OS_PRIORITY_TYPE pri;

#if OS_SCHEDULE_PRIORITY_COOP==1

    /* For cooperative scheduling, we have to get the task from maximum priority to minimum.
     * PRIO_TASKS has the index for the tasks, ordered by priority */
    for( i = 0 ; i < TASK_COUNT ; i++ )
    {
        if( os_tcbs[ PRIO_TASKS[i] ]->pDin->state == osTskREADY )
        {
            return PRIO_TASKS[i] ;
        }
    }

    //no encontro,
    //retorno OS_IDLE_TASK_INDEX
    return OS_IDLE_TASK_INDEX;
#else

    /* busco next */
    j=0;

    for( pri = OS_PRI_HIGHEST ; pri != 0xFF ; pri -- )
    {
        if( PRIORITIES_COUNT[pri] )
        {
            /* la variable i es relativa al segmento de PRIO_TASKS para esta prioridad */
            i 		= 0;
            i_max 	= PRIORITIES_COUNT[pri];

            /* determino si la tarea actual esta en esta prioridad */
            if( Sched.current_task >= i+j && Sched.current_task < i_max+j )
            {
                /* si esta en esta prioridad, le tengo que dar el comportamiento de RR.
                 * o sea, que comienzo a "mirar" por tareas en ready desde la actual. */
                i =  Sched.current_task-j;
            }
            else
            {
                /* si la prioridad actual no esta en esta priodidad arranco desde la primera
                 * tarea que hay en la seccion del array */
                i = PRIORITIES_COUNT[pri]-1;
            }

            /* guardo el comienzo del loop, para que salga si llega al mismo lugar */
            i_0 = i;

            do
            {
                i++;
                i=i%PRIORITIES_COUNT[pri];

                if( os_tcbs[ PRIO_TASKS[i+j] ]->pDin->state == osTskREADY )
                {
                    return i+j ;
                }
            }
            while( i != i_0 );

            /* si sale del while, significa que no se encontro una tarea en ready en el segmento de PRIO_TASKS*/

        }

        j  +=  PRIORITIES_COUNT[pri] ;
    }


#endif

    return INVALID_TASK;
}



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
        next = osRRP_SearchReadyTask( 0 , TASK_COUNT );
#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
        next = osPP_SearchReadyTask_Coop();
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
#error REARMAR ESTA PARTE.
        /* busco la siguiente tarea en ready */
        next = osRRP_SearchReadyTask(  Sched.current_task + 1 , TASK_COUNT );

        if( next == INVALID_TASK )
        {
            //no encontro
            next = osRRP_SearchReadyTask( 0 , Sched.current_task );
        }
#endif


#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
        /* busco la primera tarea en ready con mayor prioridad */
        next =  osPP_SearchReadyTask();
#endif

        if( next == INVALID_TASK )
        {
            /* evaluo que paso con current task
               puede estar en running, por lo que la dejo asi.
               puede estar bloqueada: En este caso, la proxima tarea va a ser idle, porque la bloqueada y las otras
               no estan ready.  */

            if( os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state==osTskBLOCKED )
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

            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->sp;

            return rv;
        }
    }
    else
    {
        if( Sched.next_task != INVALID_TASK )
        {
            if( os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state==osTskRUNNING )
            {
                os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state = osTskREADY;	/* change the state of the current task to ready */
            }

            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->sp = actualcontext;

            Sched.current_task = Sched.next_task;
            Sched.next_task    = INVALID_TASK;

            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->sp;

            return rv;
        }
    }

    return actualcontext;
}

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
/*
 * devuelve el minimo remain de los delays de las tareas.
 * */
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
        os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->state  = osTskBLOCKED;

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
        uint32_t min_remain;
        uint32_t i;

        if( delay_ms >= Sched.main_delay_counter )
        {
            /* load the delay in the register  */
            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->delay = delay_ms;

            min_remain = os_get_min_remain();	//va a haber un minimo porque arriba ya se actualizo

            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->delay  -= min_remain;

            Sched.main_delay_counter = min_remain;
        }
        else
        {
            uint32_t dif = Sched.main_delay_counter - delay_ms;


            /* load the delay in the register  */
            os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->delay = 0;

            /* a todos los otros remains, calculados con el tiempo parcial anterior se van a incrementar
             * en la dif */
            for( i=0 ; i<TASK_COUNT ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskBLOCKED  )
                {
                    if( i != OS_CURRENT_TASK_TCB_INDEX )
                    {
                        os_tcbs[i]->pDin->delay += dif;
                    }

                }
            }

            Sched.main_delay_counter = delay_ms;
        }



#else
        /* load the delay in the register  */
        os_tcbs[OS_CURRENT_TASK_TCB_INDEX]->pDin->delay = delay_ms;
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

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* inicializo a cero el array de cuantas tareas por prioridad hay */
    memset( PRIORITIES_COUNT , 0, sizeof( PRIORITIES_COUNT ) );
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

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
            /* busco la primera tarea en ready con mayor prioridad */
            //   pq_Add( &prioq, i,os_tcbs[i]->priority );
#endif
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

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY

#if OS_FIXED_PIORITY == 1


#else
        /* copy the defauult prio to the ram location */
        os_tcbs[i]->pDin->priority = os_tcbs[i]->priority;
#endif


        /*  if( i!=OS_IDLE_TASK_INDEX )
          {*/
        /* inicialmente las tareas estan desordenadas */
        PRIO_TASKS[i] = i ;
        PRIORITIES_COUNT[os_tcbs[i]->priority]++;
        /* }*/
#endif
    }

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* se ordena el array de prioridades */
    osPP_SortPrioArray();
#endif

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
