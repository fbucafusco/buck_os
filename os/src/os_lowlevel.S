        	.syntax unified

	.text
	.global PendSV_Handler
	.global getNextContext
	.thumb_func

PendSV_Handler:
 	cpsid f	/* disable isr*/

	push {r4-r11,lr}

	mrs r0, msp

	bl os_get_next_context

	msr msp, r0   /* sp <= r0 */

	pop {r4-r11,lr}

 	cpsie f	/* enable isr */

safe_exit:
	bx lr

/*
* tarea: usar pendsv con la prioridad minima
*        crar funcion "switch context"
         n tareas
*
para usar pendsv

*/
