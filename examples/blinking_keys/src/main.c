#include "main.h"
#include "board.h"
#include "os.h"
#include "keypad.h"
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
        .led_number  	= 3 ,
        .delay 			= 500,
    },

    [1] =
    {
        .led_number 	= 4 ,
        .delay 			= 1000,
    },

    [2] =
    {
        .led_number 	= 5 ,
        .delay 			= 50,
    },
    [3] =
    {
        .led_number 	= 0 ,
        .delay 			= 100,
    },
    [4] =
    {
        .led_number 	= 1 ,
        .delay 			= 300,
    },
    [5] =
    {
        .led_number 	= 2 ,
        .delay 			= 400,
    },
};

/* *******************
 * OS CONFIGURATION
 * ******************* */

/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , task_led , &leds[3] , MAX_STACK_SIZE , 2 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , task_led , &leds[4] , MAX_STACK_SIZE , 1  , TASK_AUTOSTART )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , task_keys , NULL , MAX_STACK_SIZE , 1  , TASK_AUTOSTART ) //


DECLARE_OS_START()		/* INICIO DE CONFIGURACION DE OS */
OS_ADD_TASK( task1 )
OS_ADD_TASK( task2 )
OS_ADD_TASK( task3 )
DECLARE_OS_END()		/* FIN DE CONFIGURACION DE OS */



/* isr vectors */
_I_OS_ISR_START

#if KEY_COUNT>0
_I_DECLARE_ISR_HANDLER_R_( PIN_INT0_IRQn , key_handler_0 )
#endif

#if KEY_COUNT>1
_I_DECLARE_ISR_HANDLER_R_( PIN_INT1_IRQn , key_handler_0 )
#endif


#if KEY_COUNT>2
_I_DECLARE_ISR_HANDLER_R_( PIN_INT2_IRQn , key_handler_0 )
#endif


#if KEY_COUNT>3
_I_DECLARE_ISR_HANDLER_R_( PIN_INT3_IRQn , key_handler_0 )
#endif

_I_OS_ISR_END


void task_led( void* arg )
{
    tLedBlick * led_config = ( tLedBlick * ) arg;

    while( 1 )
    {
        Board_LED_Toggle( led_config->led_number );
        osDelay( led_config->delay );
        //osSetEvent_T( 0x0001 , OS_TASK_REF( task3 ) );
    }
}


/* it blinks fast when receive an event.*/
void task_keys( void* arg )
{
    uint32_t keys;

    KeysInit();

    while( 1 )
    {
        keys =  KeyWait();

        if( keys & 0x0001 )
        {
            Board_LED_Set( leds[0].led_number , 1 );
        }
        else
        {
            Board_LED_Set( leds[0].led_number , 0 );
        }

        if( keys & 0x0002 )
        {
            Board_LED_Set( leds[1].led_number , 1 );
        }
        else
        {
            Board_LED_Set( leds[1].led_number , 0 );
        }

        if( keys & 0x0004 )
        {
            Board_LED_Set( leds[2].led_number , 1 );
        }
        else
        {
            Board_LED_Set( leds[2].led_number , 0 );
        }
    }
}



int main( void )
{
    osStart();
}

/*==================[end of file]============================================*/

