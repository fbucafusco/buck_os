	.syntax unified

	.extern sp1, sp2, state

	.text
	.global SysTick_Handler
	.thumb_func
SysTick_Handler:

	ldr r1, =state
	ldr r0,[r1]      /* r0 <= state */
	cmp r0, 0
	bne siguiente
	add r0, 1
	str r0,[r1]
	ldr r0, =sp1
	ldr r0, [r0]
	b salir
siguiente:
	cmp r0, 1
	bne siguiente2
	add r0, 1
	str r0, [r1]

	ldr r1, =sp1
	mrs r0, msp
	str r0, [r1]

	ldr r1, =sp2
	ldr r0, [r1]

	b salir

siguiente2:
	cmp r0, 2
	bne safe_exit
	mov r0, 1
	str r0, [r1]

	ldr r1, =sp2
	mrs r0, msp
	str r0, [r1]

	ldr r1, =sp1
	ldr r0, [r1]

salir:
	msr msp, r0   /* sp <= r0 */
safe_exit:
	bx lr


