#include "main.h"
#include "board.h"
#include "os.h"
#include <string.h>

/*
 * Test 0
 *
 * 3 tareas de igual prioridad se planifican con round robin.
 * Pero cada tarea, antes del 1er tick de cambio de contexto, se ceden el CPU una a la otra.
 * El round Robin se activa en la planificacion, y las tareas se serializan.
 *
 * El test es correcto si el LED verde comienza a titilar.
 *
 * */
uint32_t SEQUENCE_INVALID	= 0x80000000;
uint32_t SequenceCounter	= 0;

char N= 5;

void Sequence
(
    uint32_t seq
)
{
    if ( ( seq ) == 0 )
    {
        SequenceCounter = 0;
    }
    else if ( ( SequenceCounter+1 ) == ( seq ) )
    {
        SequenceCounter++;
    }
    else
    {
        SequenceCounter |= SEQUENCE_INVALID;
    }
}

void SequenceEnd()
{
    uint32_t last_correct_sequence = SequenceCounter&~SEQUENCE_INVALID;
    uint32_t secuence_invalid = SequenceCounter&SEQUENCE_INVALID;
    int j;
    while( 1 )
    {
        if( !secuence_invalid )
        {
            Board_LED_Toggle( 1 );
        }
        else
        {
            Board_LED_Toggle( 2 );
        }

        for ( j=0 ; j< 0xFFFFF ; j++ )
        {
        }
    }
}


/* sobrecargo la idle hook */
void idle_hook()
{
    while( 1 )
    {
    }
}

void generic_task_con_yield( void* arg )
{
    uint32_t task_nro = ( uint32_t ) arg;

    char counter = 0;
    while( 1 )
    {
        Sequence( arg + 3*counter );
        counter++;

        osTaskYield();

        if( arg == 1 && counter==N )
        {
            SequenceEnd();
        }
    }
}


/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , generic_task_con_yield , 1 , MAX_STACK_SIZE , 1 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , generic_task_con_yield , 2 , MAX_STACK_SIZE , 1  , TASK_AUTOSTART )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , generic_task_con_yield , 3 , MAX_STACK_SIZE , 1  , TASK_AUTOSTART )

/* INICIO DE CONFIGURACION DE OS */
DECLARE_OS_START()
OS_ADD_TASK( task1 )
OS_ADD_TASK( task2 )
OS_ADD_TASK( task3 )
DECLARE_OS_END()

/* FIN DE CONFIGURACION DE OS */
int main( void )
{
    osStart();
}

/*==================[end of file]============================================*/

