#include "main.h"
#include "board.h"
#include "os.h"
#include <string.h>

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
        .delay = 0x1FFFFF,
    },

    [1] =
    {
        .led_number = 4 ,
        .delay = 0x1FFFFF*2,
    },
    [2] =
    {
        .led_number = 5 ,
        .delay = 0x1FFFFF*4,
    },
};

/* sobrecargo la idle hook */
void idle_hook()
{
    int j;
    while( 1 )
    {
        Board_LED_Toggle( 1 );
        for ( j=0 ; j< 0xFFFFF ; j++ )
        {
        }
    }
}




void generic_task_con_delay( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;
    int j;
    int dummy_i;

    while( 1 )
    {
        Board_LED_Toggle( led_config->led_number );
        osDelay( 100 );
    }
}


void generic_task( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;
    int j;
    int dummy_i;

    while( 1 )
    {
        Board_LED_Toggle( led_config->led_number );
        for ( j=0 ; j<led_config->delay ; j++ )
        {
        }
    }
}

/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , generic_task_con_delay , &leds[0] , MAX_STACK_SIZE/2 , 1 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , generic_task , &leds[1] , MAX_STACK_SIZE , 1  , TASK_AUTOSTART )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , generic_task , &leds[2] , MAX_STACK_SIZE , 1  , TASK_AUTOSTART ) //

/* INICIO DE CONFIGURACION DE OS */
OS_TASKS_START()
OS_ADD_TASK( task1 )
OS_ADD_TASK( task2 )
OS_ADD_TASK( task3 )
OS_TASKS_END()

/* FIN DE CONFIGURACION DE OS */
int main( void )
{
    osStart();
}

/*==================[end of file]============================================*/

