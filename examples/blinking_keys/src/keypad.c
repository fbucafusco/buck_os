/*
 * teclado.c
 *
 *  Created on: 15/3/2017
 *      Author: franco.bucafusco
 */

#include "os.h"
#include "keypad.h"

#include "os_defs.h"

typedef struct
{
    int port;
    int bit;
} ciaaPin_t;

#ifdef edu_ciaa_nxp
ciaaPin_t inputs[4] = { {0,4},{0,8},{0,9},{1,9} };
#endif

typedef struct
{
    char estado;
} tKey;

#define KEY_ESTADO_UP	0
#define KEY_ESTADO_F	1
#define KEY_ESTADO_DOWN	2
#define KEY_ESTADO_R	3

tKey 		Keys[KEY_COUNT];
tTCB 		*keypad_task;
uint32_t 	key_state_backup;

void KeysInit()
{
    unsigned char i;

    keypad_task = osTaskGetHandler();

    for( i=0 ; i<KEY_COUNT ; i++ )
    {
        Keys[i].estado = KEY_ESTADO_UP;
    }

    uint32_t inputs = ( 1<<4 );
#if KEY_COUNT>1
    inputs |=  ( 1<<8 );
#endif

#if KEY_COUNT>2
    inputs |=  ( 1<<9 );
#endif

    Chip_GPIO_SetDir( LPC_GPIO_PORT, 0, inputs ,0 );	//as inputs


    Chip_SCU_PinMux( 1,0,MD_PUP|MD_EZI|MD_ZI,FUNC0 ); /* GPIO0[4], SW1 */
    Chip_SCU_GPIOIntPinSel( 0 , 0 , 4 );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH0 );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH0 );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT, PININTCH0 );

#if KEY_COUNT>1
    Chip_SCU_PinMux( 1,1,MD_PUP|MD_EZI|MD_ZI,FUNC0 ); /* GPIO0[8], SW2 */

    Chip_SCU_GPIOIntPinSel( 1,0,8 );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT,PININTCH1 );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT,PININTCH1 );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT,PININTCH1 );
#endif

#if KEY_COUNT>2
    Chip_SCU_PinMux( 1,2,MD_PUP|MD_EZI|MD_ZI,FUNC0 ); /* GPIO0[9], SW3 */

    Chip_SCU_GPIOIntPinSel( 2,0,9 );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT,PININTCH2 );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT,PININTCH2 );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT,PININTCH2 );
#endif

#if KEY_COUNT>3
    Chip_SCU_PinMux( 1,6,MD_PUP|MD_EZI|MD_ZI,FUNC0 ); /* GPIO1[9], SW4 */
    Chip_GPIO_SetDir( LPC_GPIO_PORT, 1,( 1<<9 ),0 );  //as inputs

    Chip_SCU_GPIOIntPinSel( 3,1,9 );
    Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT,PININTCH3 );
    Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT,PININTCH3 );
    Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT,PININTCH3 );
#endif

    key_state_backup = 0;
}

uint32_t KeyGetState()
{
    uint32_t rv  =0;
    uint32_t i;
    for( i=0 ; i<KEY_COUNT ; i++ )
    {
        if( !Chip_GPIO_ReadPortBit( LPC_GPIO_PORT,
                                    inputs[i].port,
                                    inputs[i].bit )  )
        {
            rv |= ( 1<<i );
        }
    }
    return rv;
}

/*espera una tecla, y devuelve las teclas pulsadas*/
uint32_t KeyWait( )
{
    char confirmada=0;

    uint32_t key_despues;

    while( 1 )
    {
        osClearEvents( 0x0001 );
        OS_EVENT_TYPE event = osWaitEvent( 0x0001 );

        // key_antes=0 ;
        key_despues=0 ;

        //key_antes = KeyGetState();
        osDelay( 20 ); //debounce
        key_despues = KeyGetState();

        if( key_state_backup == key_despues )
        {
            key_state_backup = 0 ;
            break;
        }
    }

    return key_despues;
}

void key_handler_0(  )
{
    key_state_backup = KeyGetState();

    osSetEvent_T( 0x0001 , keypad_task );

    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT,PININTCH0 );

#if KEY_COUNT>1
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT,PININTCH1 );
#endif
#if KEY_COUNT>2
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT,PININTCH2 );
#endif
#if KEY_COUNT>3
    Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT,PININTCH3 );
#endif

}
