#include "main.h"
#include "board.h"
#include "os.h"
#include <string.h>

/*
 * Test 1
 *
 * 3 tareas de igual prioridad se planifican con round robin.
 *
 * 1 tarea se encarga e iniciar otras 2 con osAtartTask. De forma periodica
 *
 * Las 2 creadas encienden 1 led, y se auto terminan.
 *
 * El test es correcto si el LED verde comienza a titilar.
 *
 * */

uint32_t SEQUENCE_INVALID	= 0x80000000;
uint32_t SequenceCounter	= 0;

char N= 5;

void Sequence( uint32_t seq )
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

typedef struct
{
    int 		led_number;
    uint32_t 	delay;
} tLedBlick;

tLedBlick leds[] =
{
    [0] =
    {
        .led_number  = 3 ,
        .delay = 500,
    },
    [1] =
    {
        .led_number = 4 ,
        .delay = 200,
    },
    [2] =
    {
        .led_number = 5 ,
        .delay = 400,
    },
};


/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , master_task , &leds[0] , MAX_STACK_SIZE , 1 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , led_task2 , &leds[1] , MAX_STACK_SIZE , 1  , TASK_NOCONFIG )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , led_task3 , &leds[2] , MAX_STACK_SIZE , 1  , TASK_NOCONFIG )

/* sobrecargo la idle hook */
void idle_hook( void*arg )
{
    while( 1 )
    {
    }
}

void led_task2( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;

    Sequence( 2 );

    Board_LED_Toggle( led_config->led_number );

    osDelay( led_config->delay );

    Sequence( 6 );

    Board_LED_Toggle( led_config->led_number );

    osTaskEnd();
}


void led_task3( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;

    Sequence( 4 );

    Board_LED_Toggle( led_config->led_number );

    osDelay( led_config->delay );

    Sequence( 7 );

    Board_LED_Toggle( led_config->led_number );

    osTaskEnd();
}


void master_task( void* arg )
{
    char counter = 0;

    Sequence( 1 );

    counter++;

    /* we start the task2 and as it has the same priority, it comes next in the scheduling */
    osTaskStart( task2 );

    Sequence( 3 );

    osTaskStart( task3 );

    Sequence( 5 );

    osDelay( 1000 );


    SequenceEnd();
}

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

