#include "main.h"
#include "board.h"
#include "os.h"
#include <string.h>

typedef struct
{
    int 		led_number;
    uint32_t 	delay;
} tLedBlick;


/* ************************
 * PARAMETERS FOR EACH TASK
 * ************************ */
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
        .delay = 1000,
    },

    [2] =
    {
        .led_number = 5 ,
        .delay = 50,
    },
};

/* *******************
 * OS CONFIGURATION
 * ******************* */

/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , generic_task_con_delay , &leds[0] , MAX_STACK_SIZE , 2 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , generic_task_con_delay , &leds[1] , MAX_STACK_SIZE , 1  , TASK_AUTOSTART )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , task_wait , &leds[2] , MAX_STACK_SIZE , 1  , TASK_AUTOSTART ) //


DECLARE_OS_START()		/* INICIO DE CONFIGURACION DE OS */
OS_ADD_TASK( task1 )
OS_ADD_TASK( task2 )
OS_ADD_TASK( task3 )
DECLARE_OS_END()		/* FIN DE CONFIGURACION DE OS */

void generic_task_con_delay( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;

    while( 1 )
    {
        Board_LED_Toggle( led_config->led_number );
        osDelay( led_config->delay );
        osSetEvent_T( 0x0001 , OS_TASK_REF( task3 ) );
    }
}


/* it blinks fast when receive an event.*/
void task_wait( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;
    float a=0;

    while( 1 )
    {
        osWaitEvent( 0x0001 );
        Board_LED_Toggle( led_config->led_number );
        osDelay( led_config->delay );
        Board_LED_Toggle( led_config->led_number );
    }
}


int main( void )
{
    osStart();
}

/*==================[end of file]============================================*/

