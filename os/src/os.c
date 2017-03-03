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

/* politica de scheduling:
 * en cada tick se hace cambio de contexto a la tarea siguiente.
 * es un roundrobin de 1 ms. */

void os_schedule()
{
    uint32_t i;
    uint32_t found = 0 ;

    if( Sched.current_task == INVALID_TASK )
    {
        /* busco la primera tarea en ready */
        for( i=0 ; i<TASK_COUNT ; i++ )
        {
            if( os_tcbs[i]->pDin->state == osTskREADY )
            {
                found = 1;
                break;
            }
        }

        if( found == 0 )
        {
            //no hay ninguna ready

            /* TENGO QUE EJECUTAR IDLE_TASK.... */
            Sched.next_task = OS_IDLE_TASK_INDEX;
        }
        else
        {
            // Ok! encontre una ready
            Sched.next_task = i;
        }
    }
    else
    {
        /* busco la siguiente tarea en ready */
        for( i = Sched.current_task + 1 ; i < TASK_COUNT ; i++ )
        {
            if( os_tcbs[i]->pDin->state == osTskREADY )
            {
                found = 1;
                break;
            }
        }

        if( i == TASK_COUNT )
        {
            //no encontro
            for( i = 0 ; i < Sched.current_task ; i++ )
            {
                if( os_tcbs[i]->pDin->state == osTskREADY )
                {
                    found = 1;
                    break;
                }
            }
        }

        if( found == 0 )
        {
            /* entre todas las tareas, que no son la que venia corriendo, no se encontro otra ready.
             * asique no se toca nada.
             * Sched.next_task quedaria en invalid */
        }
        else
        {
            //ok! encontre una ready
            Sched.next_task = i;
        }
    }

    if( Sched.next_task != Sched.current_task && Sched.next_task != INVALID_TASK )
    {
        os_trigger_cc();
    }
}

void os_trigger_cc()
{
    OS_DISABLE_ISR();
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __ISB(); //INSTRUCTION SYNCHRONIZATION BARRIER
    __DSB(); //DATA SYNCHRONIZATION BARRIER
    OS_ENABLE_ISR();
}


/* funcion llamada por pendsv para obtener el nuevo contexto.*/
uint32_t *os_get_next_context( uint32_t *actualcontext )
{
    uint32_t *rv;

    //primero veo si vengo del stackframe principal.

    if( Sched.current_task != INVALID_TASK )
    {
        if( Sched.next_task != INVALID_TASK )
        {
            os_tcbs[Sched.current_task]->pDin->state = osTskREADY;	/* change the state of the current task to ready */
            os_tcbs[Sched.current_task]->pDin->sp = actualcontext;

            Sched.current_task = Sched.next_task;
            Sched.next_task    = INVALID_TASK;

            os_tcbs[Sched.current_task]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */
            rv = os_tcbs[Sched.current_task]->pDin->sp;

            return rv;
        }
    }
    else
    {
        //caso en que se quiere cambiar contexto desde el stackframe principal
        //la primera ejecucion.
        if( Sched.next_task != INVALID_TASK )
        {
            Sched.current_task = Sched.next_task;

            Sched.next_task    = INVALID_TASK;

            os_tcbs[Sched.current_task]->pDin->state = osTskRUNNING;/* change the state of the NEW current task to running */

            rv = os_tcbs[Sched.current_task]->pDin->sp;

            return rv;
        }
    }

    return actualcontext;
}

/* implements a blocking delay for the current running task */
void osDelay( uint32_t delay_ms )
{
    uint32_t ticks = delay_ms; //TODO: AGREGAR FACTOR DE ESCALA.

    if( delay_ms!= 0 && os_tcbs[Sched.current_task]->pDin->delay== 0 ) //TODO: ESTA ULTIMA VALIDACION ES SOLO PARA ZAFAR. NO DEBERIA IR.
    {
        /* load the delay in the register  */
        os_tcbs[Sched.current_task]->pDin->delay = delay_ms;

        /* the task goes to blocking state */
        os_tcbs[Sched.current_task]->pDin->state  = osTskBLOCKED;

        /* the current task should be mark as invalid*/
        Sched.current_task = INVALID_TASK;

        /* call the scheduler */
        os_schedule();
    }
}

/* in a tick, it checkes the delays */
void os_check_timeouts()
{
    uint32_t i;

    for( i=0 ; i<TASK_COUNT ; i++ )
    {
        if( os_tcbs[i]->pDin->state == osTskBLOCKED && os_tcbs[i]->pDin->delay != 0 )
        {
            os_tcbs[i]->pDin->delay--;

            /* if the task delay counter went to 0 */
            if( os_tcbs[i]->pDin->delay == 0 )
            {
                /* the task goes to blocking state */
                os_tcbs[i]->pDin->state  = osTskREADY;
            }
        }
    }
}

void osStart()
{
    Board_Init();

    SystemCoreClockUpdate();

    /* habilito la interrupcion de pendable service */
    NVIC_SetPriority( PendSV_IRQn, ( 1 << __NVIC_PRIO_BITS ) - 1 );

    /**/
    Sched.current_task = INVALID_TASK;
    Sched.next_task    = INVALID_TASK;

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
        bzero( os_tcbs[i]->stackframe, os_tcbs[i]->stacksize );

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
