/* Copyright 2016, Pablo Ridolfi
 * All rights reserved.
 *
 * This file is part of Workspace.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

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
    while ( 1 )
    {
        Board_LED_Toggle( 0 );
        for ( i=0; i<0x3FFFFF; i++ );
    }
}

void task2( void )
{
    int j;
    while ( 1 )
    {
        Board_LED_Toggle( 2 );
        for ( j=0; j<0xFFFFF; j++ );
    }
}

void return_hook( void )
{

}

void taskSetup( void ( *entry_point )( void ), uint32_t * stack,
                uint32_t stack_size, uint32_t ** sp )
{
    bzero( stack, stack_size );

    stack[stack_size/4-3] = ( uint32_t )return_hook; /* LR */
    stack[stack_size/4-2] = ( uint32_t )entry_point; /* PC */
    stack[stack_size/4-1] = 1<<24; /* xPSR.T = 1 */

    *sp = stack+stack_size/4-8; /* sp inicial */
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
