/*
 * sched.c
 *
 *  Created on: 22/2/2017
 *      Author: franco.bucafusco
 */

#include "os.h"
#include "os_delay.h"
#include "chip.h"
#include <string.h>

/* external objects thart are defined by the user */
extern const tTCB *os_tcbs[];
extern unsigned short TASK_COUNT;
extern TASK_COUNT_TYPE PRIORITIES_COUNT[OS_PRI_COUNT];
extern tTCB * os_sorted_Tcbs[];

tSched Sched;

/* extern os functions */
extern void _os_task_start( TASK_COUNT_TYPE absolute_index );
extern void _os_task_active( tTCB *pTCB );
extern void _os_task_not_active_( tTCB *pTCB );

/* idle_hook
 * es llaado cuando el OS no tiene que ejecutar ninguna tarea. */
__attribute__( ( weak ) ) void idle_hook( void*arg )
{
    while( 1 )
    {

    };
}



/* increment the priority to the next high available priorirty */
OS_PRIORITY_TYPE _os_pp_get_next_prio( OS_PRIORITY_TYPE curr_prio )
{
    if( curr_prio==OS_PRI_HIGHEST )
    {
        return OS_PRI_HIGHEST;
    }

    return curr_prio+1;
}

/* Ordena el array os_sorted_Tcbs segun las prioridades de las tareas que éste referencia.
 *
 * Deja en os_sorted_Tcbs los indices de tcbs de las mas prioritarias a las menos.
 * Al final deja las INVALID_TASK
 *
 * : TODO: si se implementara un "generator" estas tareas no seria necesarias y se podria dejar en un array const */
void _os_pp_sort_prio_array()
{
    TASK_COUNT_TYPE i, j ;
    tTCB *swap;
    uint32_t interambiar;

    for( i=0; i< TASK_COUNT-1; i++ )
    {
        for( j=0; j< TASK_COUNT-i-1; j++ )
        {
            /* criteria for interchange elements */
            if( os_sorted_Tcbs[j+1]!= NULL )
            {
                if( os_sorted_Tcbs[j]==NULL )
                {
                    interambiar = 1;
                }
                else
                {
                    OS_PRIORITY_TYPE pri_j 		= os_sorted_Tcbs[j]->pDin->current_priority;
                    OS_PRIORITY_TYPE pri_j_1 	= os_sorted_Tcbs[j+1]->pDin->current_priority;

                    if( pri_j < pri_j_1 )
                    {
                        interambiar = 1;
                    }
                    else
                    {
                        interambiar = 0;
                    }
                }
            }
            else
            {
                interambiar = 0;
            }

            /* if elements has to be interchanged */
            if( interambiar )
            {
                swap       			= os_sorted_Tcbs[j+1];
                os_sorted_Tcbs[j+1] = os_sorted_Tcbs[j];
                os_sorted_Tcbs[j] 	= swap;
            }
        }
    }
}


/* hace lo mismo que antes, pero rastrea el tcb indicado.
 * es decir, ademas de ordenar el que llama dice "decime en donde estara el indice "index" luego de ordenar
 * index es el indice de os_sorted_Tcbs
 *
 *  * NO ES THREAD SAFE.*/
TASK_COUNT_TYPE _os_pp_sort_prio_array_ti( TASK_COUNT_TYPE index )
{
    TASK_COUNT_TYPE i, j ;
    tTCB *swap;
    uint32_t interambiar;

    for( i=0; i< TASK_COUNT-1; i++ )
    {
        for( j=0; j< TASK_COUNT-i-1; j++ )
        {
            /* criteria for interchange elements */
            if( os_sorted_Tcbs[j+1]!= NULL )
            {
                if( os_sorted_Tcbs[j]==NULL )
                {
                    interambiar = 1;
                }
                else
                {
                    OS_PRIORITY_TYPE pri_j 		= os_sorted_Tcbs[j]->pDin->current_priority;
                    OS_PRIORITY_TYPE pri_j_1 	= os_sorted_Tcbs[j+1]->pDin->current_priority;

                    if( pri_j < pri_j_1 )
                    {
                        interambiar = 1;
                    }
                    else
                    {
                        interambiar = 0;
                    }
                }
            }
            else
            {
                interambiar = 0;
            }

            /* if elements has to be interchanged */
            if( interambiar )
            {
                if( index == j )
                {
                    index = j+1;
                }
                else if( index == j+1 )
                {
                    index = j ;
                }

                swap       			= os_sorted_Tcbs[j+1];
                os_sorted_Tcbs[j+1] = os_sorted_Tcbs[j];
                os_sorted_Tcbs[j] 	= swap;
            }
        }
    }
    return index;
}

/* it moves one element from one priority to another
 *  * NO ES THREAD SAFE.*/
void _os_pp_sort_prio_count_move( OS_PRIORITY_TYPE from, OS_PRIORITY_TYPE to )
{
    PRIORITIES_COUNT[from]--;
    PRIORITIES_COUNT[to]++;
}

/* Cambia la prioridad a una tarea
 *
 * index: indice absoluto del array de tcbs.
 * nueva prioridad.
 *
 * NO ES THREAD SAFE.
 * */
void _os_pp_change_task_priority( TASK_COUNT_TYPE index, OS_PRIORITY_TYPE newpriority )
{
    /* update the priority count array */
    _os_pp_sort_prio_count_move( os_tcbs[index]->pDin->current_priority , newpriority );

    /* set the new priority */
    os_tcbs[index]->pDin->current_priority = newpriority;

    /* sort the priority array and update the current_task relative index */
    Sched.current_task = _os_pp_sort_prio_array_ti( Sched.current_task );
}

void _os_pp_restore_task_priority( TASK_COUNT_TYPE index )
{
    OS_PRIORITY_TYPE original_prio;

    /* restores the original priority*/
#if OS_FIXED_PIORITY == 0
    original_prio = os_tcbs[index]->pDin->priority_back;
#else
    original_prio = os_tcbs[index]->def_priority;
#endif

    /* cambio la prioridad */
    _os_pp_change_task_priority( index , original_prio );
}



/*
 * PRIORITIES_COUNT[os_tcbs[i]->def_priority]++;
 * */


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


/* no se usa DEPRECATED */
/*
void _os_pp_rotate_subarray( TASK_COUNT_TYPE inicio, TASK_COUNT_TYPE fin )
{
    TASK_COUNT_TYPE i;
    TASK_COUNT_TYPE back;

    back = os_sorted_Tcbs[inicio];

    for( i=inicio ; i < fin-1 ; i++ )
    {
        os_sorted_Tcbs[i] = os_sorted_Tcbs[i+1];
    }

    os_sorted_Tcbs[i] = back;

}*/

/* busca, en os_sorted_Tcbs la primera de mas prioridad en ready. */
uint32_t _os_pp_search_ready_task_coop()
{
    TASK_COUNT_TYPE i;

    for( i = 0 ; i < Sched.active_tasks ; i++ )
    {
        if( os_sorted_Tcbs[i]->pDin->state == osTskREADY )
        {
            return i ;
        }
    }

    return INVALID_TASK;
}

/* busca en los os_sorted_Tcbs desde el indice 0 a TASK_COUNT (no inclusive)
 * las tareas ready mas prioritarias */
TASK_COUNT_TYPE _os_pp_search_ready_task( TASK_COUNT_TYPE current_task )
{
    TASK_COUNT_TYPE i;
    TASK_COUNT_TYPE j;
    TASK_COUNT_TYPE i_max;
    TASK_COUNT_TYPE i_0 ;

    OS_PRIORITY_TYPE pri;

#if OS_SCHEDULE_PRIORITY_COOP==1
#error REVISAR IMPLEMENTACION
    /* For cooperative scheduling, we have to get the task from maximum priority to minimum.
     * os_sorted_Tcbs has the index for the tasks, ordered by priority */
    for( i = 0 ; i < TASK_COUNT ; i++ )
    {
        if( os_tcbs[ os_sorted_Tcbs[i] ]->pDin->state == osTskREADY )
        {
            return os_sorted_Tcbs[i] ;
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
            /* la variable i es relativa al segmento de os_sorted_Tcbs para esta prioridad */
            i 		= 0;
            i_max 	= PRIORITIES_COUNT[pri];

            /* determino si la tarea actual esta en esta prioridad */
            if( current_task >= i+j && current_task < i_max+j )
            {
                /* si esta en esta prioridad, le tengo que dar el comportamiento de RR.
                 * o sea, que comienzo a "mirar" por tareas en ready desde la actual. */
                i =  current_task - j ;
            }
            else
            {
                /* si la prioridad actual no esta en esta priodidad, arranco desde la primera
                 * tarea que hay en la seccion del array */
                i = PRIORITIES_COUNT[pri]-1;
            }

            /* guardo el comienzo del loop, para que salga si llega al mismo lugar */
            i_0 = i;

            do
            {
                i++;
                i=i%PRIORITIES_COUNT[pri];

                if( os_sorted_Tcbs[i+j]->pDin->state == osTskREADY )
                {
                    return i+j ;
                }
            }
            while( i != i_0 );

            /* si sale del while, significa que no se encontro una tarea en ready en el segmento de os_sorted_Tcbs*/
        }

        j  +=  PRIORITIES_COUNT[pri] ;
    }

#endif

    return INVALID_TASK;
}

#endif

void _os_trigger_cc()
{
    OS_DISABLE_ISR();
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __ISB(); //INSTRUCTION SYNCHRONIZATION BARRIER
    __DSB(); //DATA SYNCHRONIZATION BARRIER
    OS_ENABLE_ISR();
}


/*
 * decide virtualmente que tarea será la proxima
 * basado en el current task.
 * NOT THREAD SAFE
 * */
TASK_COUNT_TYPE _os_get_next( TASK_COUNT_TYPE current_task )
{
    TASK_COUNT_TYPE rv;

    if( current_task == INVALID_TASK || current_task == OS_IDLE_TASK_INDEX )
    {
#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
        /* busco la primera tarea en ready */
        next = osRRP_SearchReadyTask( 0 , TASK_COUNT );
#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
        rv = _os_pp_search_ready_task_coop( Sched.current_task );
#endif

        if( rv == INVALID_TASK )
        {
            //no hay ninguna ready

            /* TENGO QUE EJECUTAR IDLE_TASK.... */
            rv = OS_IDLE_TASK_INDEX;
        }
        else
        {
            // Ok! encontre una ready
            //Sched.next_task = next;
        }
    }
    else
    {
#if OS_SCHEDULE_POLICY==osSchPolicyROUND_ROBIN
#error REARMAR ESTA PARTE.
        /* busco la siguiente tarea en ready */
        rv = osRRP_SearchReadyTask(  current_task + 1 , TASK_COUNT );

        if( rv == INVALID_TASK )
        {
            //no encontro
            rv = osRRP_SearchReadyTask( 0 , current_task );
        }
#endif


#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
        /* busco la primera tarea en ready con mayor prioridad */
        rv =  _os_pp_search_ready_task( current_task );
#endif

        if( rv == INVALID_TASK )
        {
            /* evaluo que paso con current task
               puede estar en running, por lo que la dejo asi.
               puede estar bloqueada: En este caso, la proxima tarea va a ser idle, porque la bloqueada y las otras
               no estan ready.  */

            if( OS_TASK_TCB_REF_( current_task )->pDin->state==osTskBLOCKED )
            {
                rv = OS_IDLE_TASK_INDEX;
            }
        }
        else
        {
            //ok! encontre una ready
            //Sched.next_task = next;
        }
    }

    return rv;
}

void _os_trigger_cc_conditional( tSched *pSched  )
{
    OS_DISABLE_ISR();
    uint32_t condicion_cc = ( pSched->next_task != pSched->current_task && pSched->next_task != INVALID_TASK );
    OS_ENABLE_ISR();

    if( condicion_cc )
    {
        _os_trigger_cc();
    }
}

/* os_schedule() SOLO debe definir quien es la siguiente tarea a ejecutarse (Sched.next_task)
* No debe modificar el estado de ninguna tarea.
* */
void _os_schedule()
{
    TASK_COUNT_TYPE next;

    OS_DISABLE_ISR();

    Sched.next_task = _os_get_next( Sched.current_task );

    uint32_t condicion_cc = ( Sched.next_task != Sched.current_task && Sched.next_task != INVALID_TASK );

    OS_ENABLE_ISR();

    if( condicion_cc )
    {
        _os_trigger_cc();
    }
}


/* funcion llamada por pendsv para obtener el nuevo contexto.
 *
 * it relies in that Sched.current_task & Sched.next_task are VALID
 * os_sorted_Tcbs elements (or not if INVALID_TASK) */
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

            OS_CURRENT_TASK_TCB_REF->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = OS_CURRENT_TASK_TCB_REF->pDin->sp;

            return rv;
        }
    }
    else
    {
        if( Sched.next_task != INVALID_TASK )
        {
            if( OS_CURRENT_TASK_TCB_REF->pDin->state==osTskRUNNING )
            {
                OS_CURRENT_TASK_TCB_REF->pDin->state = osTskREADY;	/* change the state of the current task to ready */
            }

            OS_CURRENT_TASK_TCB_REF->pDin->sp = actualcontext;

            Sched.current_task = Sched.next_task;
            Sched.next_task    = INVALID_TASK;

            OS_CURRENT_TASK_TCB_REF->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = OS_CURRENT_TASK_TCB_REF->pDin->sp;

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
    Sched.active_tasks			= 0;

#if( OS_INTERNAL_DELAY_WITH_MAIN_COUNTER== 1)
    Sched.main_delay_counter 	= 0;
#endif

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* inicializo a cero el array de cuantas tareas por prioridad hay */
    memset( PRIORITIES_COUNT , 0, sizeof( PRIORITIES_COUNT ) );
#endif

    /* for each task, initialize the stack */
    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        /* inicializo el estado de la tarea */
        if( os_tcbs[i]->config & TASK_AUTOSTART )
        {
            /* init the task structure and objects regarding priority */
            _os_task_active( ( tTCB * ) os_tcbs[i] );

            /* manage the priority algoritms objects */
#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
            /* copy the defauult prio to the current_priority location */
            os_tcbs[i]->pDin->current_priority = os_tcbs[i]->def_priority;

            /* inicialmente las tareas estan desordenadas : TODO: si se implementara un "generator" estas tareas no seria necesarias y se podria dejar en un array const */
            os_sorted_Tcbs[i] = ( tTCB * ) os_tcbs[i] ;

            PRIORITIES_COUNT[os_tcbs[i]->pDin->current_priority]++;
#endif

            /* increment the active task count */
            Sched.active_tasks++;
        }
        else
        {
            _os_task_not_active_( ( tTCB * ) os_tcbs[i] );

            /* manage the priority algoritms objects */
#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
            os_sorted_Tcbs[i] = NULL ;
#endif
        }
    }

#if OS_SCHEDULE_POLICY==osSchPolicyPRIORITY
    /* se ordena el array de prioridades. */
    _os_pp_sort_prio_array();
#endif

    /* idle hook : special treatments */
    _os_task_active( ( tTCB * ) os_tcbs[OS_IDLE_TASK_INDEX] );

    /* initiates one element in the os_sorted_array */
    os_sorted_Tcbs[OS_IDLE_TASK_INDEX] =  ( tTCB * ) os_tcbs[OS_IDLE_TASK_INDEX] ;

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


//TODO: implementar time slice. Ahora el slice es de 1 tick.
//TODO: implementar mutex wait con timeout
//TODO: implementar cola
//TODO:

