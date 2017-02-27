#include "main.h"
#include "board.h"
#include <string.h>

#define STACK_SIZE 512

/* stack frames */
uint8_t stack1[STACK_SIZE];
uint8_t stack2[STACK_SIZE];

/* stack pointers */
uint32_t * sp1;
uint32_t * sp2;

/* system state */
uint32_t state;

void task1( void )
{
    int i;

    while( 1 )
    {
        Board_LED_Toggle( 0 );
        for ( i=0; i<0x3FFFFF; i++ );
    }
}

void task2( void )
{
    int j;

    while( 1 )
    {
        Board_LED_Toggle( 2 );
        for ( j=0; j<0xFFFFF; j++ );
    }
}


void taskSetup( void ( *entry_point )( void ), uint32_t * stack, uint32_t stack_size, uint32_t ** sp )
{
    bzero( stack, stack_size );

    stack[stack_size/4-3] = ( uint32_t ) return_hook; 	/* LR */
    stack[stack_size/4-2] = ( uint32_t ) entry_point; 	/* PC */
    stack[stack_size/4-1] = 1<<24; 						/* xPSR.T = 1 */

    *sp = stack+stack_size/4 - 16; 						/* sp inicial */
}

int main( void )
{
    Board_Init();
    SystemCoreClockUpdate();

    taskSetup( task1, ( uint32_t * )stack1, STACK_SIZE, &sp1 );

    taskSetup( task2, ( uint32_t * )stack2, STACK_SIZE, &sp2 );

    SysTick_Config( SystemCoreClock/1000 );

    while ( 1 )
    {
    }
}

/*==================[end of file]============================================*/
