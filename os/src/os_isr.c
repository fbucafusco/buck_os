/*
 * os_isr.c
 *
 *  Created on: 29/3/2017
 *      Author: franco.bucafusco
 */

#include "chip.h"
#include "os_defs.h"
#include <stdint.h>

extern const void ( * os_User_Isr_Handlers[] )() ;
extern const uint16_t USER_ISR_COUNT;

extern void _os_force_schedule();

/* it return the index of the isr vector running.
 * it return a number between 1-68
 * or BASE_LEVEL_ISR_NRO if the execution is not a ISR level. */
uint32_t _os_get_running_isr_handler( void )
{
    uint32_t nro =  __get_IPSR();

    /* check if in background level */
    if( nro==0 )
    {
        return BASE_LEVEL_ISR_NRO;
    }

    /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/ch02s03s02.html */
    /* verify the number of the ISR */
    if( nro<69 + 0x10 )	//69 is platform dependant //0x10 is because the Core offsets the number in 16
    {
        return nro - 0x10;
    }
    else
    {
        return BASE_LEVEL_ISR_NRO;
    }
}

/* dummy definitions in case the user do not needs ISRs */
__attribute__ ( ( weak ) ) const void ( * os_User_Isr_Handlers[0] )();
__attribute__ ( ( weak ) ) const uint16_t USER_ISR_COUNT = 0;

/* wrapper of ANY user handler */
void _os_generic_isr_handler()
{
    //obtener el numero de isr del registro
    uint32_t interrupt_nro = _os_get_running_isr_handler();

    /* if the ISR is within USER handlers... */
    if( interrupt_nro < USER_ISR_COUNT )
    {
        /*... and the handler is defined */
        if( os_User_Isr_Handlers[interrupt_nro] != NULL )
        {
            NVIC_ClearPendingIRQ( interrupt_nro );

            /* execute the user handler */
            os_User_Isr_Handlers[interrupt_nro]( );

            /* we call the schedule*/
            _os_force_schedule();

            return;
        }
    }

    /* the user has enabled anohter isr, without define a handler for it */
    /* the os will disable the handler */

    NVIC_ClearPendingIRQ( interrupt_nro );
    NVIC_DisableIRQ( interrupt_nro );
}

void ResetISR( void );
void NMI_Handler( void );
void HardFault_Handler( void );
void MemManage_Handler( void );
void BusFault_Handler( void );
void UsageFault_Handler( void );
void SVC_Handler( void );
void DebugMon_Handler( void );
void PendSV_Handler( void );
void SysTick_Handler( void );
void IntDefaultHandler( void );
extern void _vStackTop( void );

//extern void ( * const g_pfnVectors[] )( void );

__attribute__ ( ( used,section( ".isr_vector" ) ) )

void ( * const g_pfnVectors[] )( void ) =
{
    // Core Level - CM4
    &_vStackTop,                    // The initial stack pointer
    ResetISR,                       // The reset handler
    NMI_Handler,                    // The NMI handler
    HardFault_Handler,              // The hard fault handler
    MemManage_Handler,              // The MPU fault handler
    BusFault_Handler,               // The bus fault handler
    UsageFault_Handler,             // The usage fault handler
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    SVC_Handler,                    // SVCall handler
    DebugMon_Handler,               // Debug monitor handler
    0,                              // Reserved
    PendSV_Handler,                 // The PendSV handler
    SysTick_Handler,                // The SysTick handler

    // Chip Level - LPC43 (M4)
    _os_generic_isr_handler,        // 16
    _os_generic_isr_handler,        // 17
    _os_generic_isr_handler,        // 18
    0,                              // 19
    _os_generic_isr_handler,        // 20 ORed flash Bank A, flash Bank B, EEPROM interrupts
    _os_generic_isr_handler,        // 21
    _os_generic_isr_handler,        // 22
    _os_generic_isr_handler,        // 23
    _os_generic_isr_handler,        // 24
    _os_generic_isr_handler,        // 25
    _os_generic_isr_handler,        // 26
    _os_generic_isr_handler,        // 27
    _os_generic_isr_handler,        // 28
    _os_generic_isr_handler,        // 29
    _os_generic_isr_handler,        // 30
    _os_generic_isr_handler,        // 31
    _os_generic_isr_handler,        // 32
    _os_generic_isr_handler,        // 33
    _os_generic_isr_handler,        // 34
    _os_generic_isr_handler,        // 35
    _os_generic_isr_handler,        // 36
    _os_generic_isr_handler,        // 37
    _os_generic_isr_handler,        // 38
    _os_generic_isr_handler,        // 39
    _os_generic_isr_handler,         // 40
    _os_generic_isr_handler,         // 41
    _os_generic_isr_handler,         // 42
    _os_generic_isr_handler,         // 43
    _os_generic_isr_handler,          // 44
    _os_generic_isr_handler,          // 45
    _os_generic_isr_handler,         // 46
    _os_generic_isr_handler,         // 47
    _os_generic_isr_handler,         // 48
    _os_generic_isr_handler,         // 49
    _os_generic_isr_handler,         // 50
    _os_generic_isr_handler,         // 51
    _os_generic_isr_handler,         // 52
    _os_generic_isr_handler,         // 53
    _os_generic_isr_handler,         // 54
    _os_generic_isr_handler,         // 55
    _os_generic_isr_handler,         // 56
    _os_generic_isr_handler,         // 57
    _os_generic_isr_handler,       	 // 58
    _os_generic_isr_handler,         // 59
    _os_generic_isr_handler,         // 60
    _os_generic_isr_handler,         // 61
    _os_generic_isr_handler,         // 62
    _os_generic_isr_handler,         // 63
    _os_generic_isr_handler,         // 64
    _os_generic_isr_handler,         // 65
    _os_generic_isr_handler,         // 66
    _os_generic_isr_handler,         // 67
    _os_generic_isr_handler,         // 68
};
