/*
 * sched.c
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#include "os.h"
#include "os_delay.h"
#include "chip.h"

/* external objects thart are defined by the user */
extern const tTCB *os_tcbs[];
extern unsigned short TASK_COUNT;
extern TASK_COUNT_TYPE PRIORITIES_COUNT[OS_PRI_COUNT];
extern TASK_COUNT_TYPE PRIO_TASKS[];

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
 * deja en PRIO_TASKS los indices de tcbs de las mas prioritarias a las menos.
 *
 * : TODO: si se implementara un "generator" estas tareas no seria necesarias y se podria dejar en un array const */
void _os_pp_sort_prio_array()
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
void _os_pp_rotate_subarray( TASK_COUNT_TYPE inicio, TASK_COUNT_TYPE fin )
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
uint32_t _os_pp_search_ready_task_coop()
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
uint32_t _os_pp_search_ready_task(  )
{
    TASK_COUNT_TYPE i;
    TASK_COUNT_TYPE j;
    TASK_COUNT_TYPE i_max;
    TASK_COUNT_TYPE i_0 ;

    OS_PRIORITY_TYPE pri;

#if OS_SCHEDULE_PRIORITY_COOP==1
#error REVISAR IMPLEMENTACION
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

    /*: TODO:  en la inicializacion de PRIORITIES_COUNT se podria dejar en ram una variable
     *         q tenga la prioridad mas alta usada asi arrancar este for con una prioridad en la
     *         que SI hay tareas y no barrerer prioridades sin tareas.
     *         Otra opcion es normalizar las prioridades y listo.... pero creo que esto podria hacerse con un "generador" */
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
void _os_schedule()
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
        next = _os_pp_search_ready_task_coop();
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
        next =  _os_pp_search_ready_task();
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
        _os_trigger_cc();
    }
}


void _os_trigger_cc()
{
    OS_DISABLE_ISR();
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __ISB(); //INSTRUCTION SYNCHRONIZATION BARRIER
    __DSB(); //DATA SYNCHRONIZATION BARRIER
    OS_ENABLE_ISR();
}


/* funcion llamada por pendsv para obtener el nuevo contexto.*/
uint32_t *_os_get_next_context( uint32_t *actualcontext )
{
    uint32_t *rv;

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
uint32_t _os_get_min_remain()
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


void _os_tcb_init_stack( tTCB *pTcb )
{
    uint32_t * stackframe_;
    uint32_t   stacksize_;

    /* inicializo el stack en cero */
    bzero( pTcb->stackframe , pTcb->stacksize );



    /* cargo en variables locales para lectura mas amena */
    stackframe_ = pTcb->stackframe;
    stacksize_  = pTcb->stacksize;

    /* armo el frame inicial */
    stackframe_[stacksize_/4-1] = 1<<24; 									/* xPSR.T = 1 */
    stackframe_[stacksize_/4-2] = ( uint32_t ) pTcb->entry_point; 	/* PC */
    stackframe_[stacksize_/4-3] = ( uint32_t ) return_hook; 			    /* LR */

    stackframe_[stacksize_/4-8] = ( uint32_t ) pTcb->arg; 			/* R0 <- arg */
    stackframe_[stacksize_/4-9] = 0xFFFFFFF9; 				    			/* como apila 1ro el LR */

    /* guardo el stackpointer en tcb*/
    pTcb->pDin->sp = stackframe_ + stacksize_/4 - 17; 		        /* sp inicial  el 17 es porque pusheo de r4 a r11 + lr   */

}

void osStart()
{
    TASK_COUNT_TYPE i;

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

        /* events init */
        os_tcbs[i]->pDin->events_waiting = 0;					/* events_waiting:  */
        os_tcbs[i]->pDin->events_setted	= 0;					/* events_setted:  */

        /* the delay is zero */
        os_tcbs[i]->pDin->delay = 0;

        /* inicializo el stack */
        _os_tcb_init_stack( os_tcbs[i] );

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY

#if OS_FIXED_PIORITY == 1


#else
        /* copy the defauult prio to the ram location */
        os_tcbs[i]->pDin->priority = os_tcbs[i]->priority;
#endif

        /* inicialmente las tareas estan desordenadas : TODO: si se implementara un "generator" estas tareas no seria necesarias y se podria dejar en un array const */
        PRIO_TASKS[i] = i ;
        PRIORITIES_COUNT[os_tcbs[i]->priority]++;
#endif
    }

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* se ordena el array de prioridades. */
    _os_pp_sort_prio_array();
#endif

    /* inicio el tick */
    SysTick_Config( SystemCoreClock/1000 );
}

/* handler de systick
 * aqui se decide por la tarea siguiente. */
void SysTick_Handler()
{
    _os_delay_update();
    _os_schedule();
}
