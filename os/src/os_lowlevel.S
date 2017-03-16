        	.syntax unified

	.text
	.global PendSV_Handler
	.global getNextContext
	.thumb_func

PendSV_Handler:
 	cpsid f	/* disable isr */

	tst lr, #0x00000010 /* testeo el bit EXC_RETURN[4]. Si es 1*/
	it eq				/* si el resultado es 0 entonces, se ejecuta la siguiente instruccion, sino la ignora */
	vpusheq {s16-s31}

	push {r4-r11,lr} 	/* salvo el resto del contexto. El lr es porque estoy estackeando el EXC_RETURN */

	mrs r0, msp			/* guardo el main stack pointer en r0 (argumento de _os_get_next_context) */

	bl _os_get_next_context	/* llamo a la funcion en C que devuelve el puntero al siguiente contexto. */

	msr msp, r0  		/* recupero el main stack pointer de r0 (return value de _os_get_next_context) */

	pop {r4-r11,lr}		/* reestablezco el contexto. El lr es el EXC_RETURN */

	tst lr, #0x00000010 /* testeo el bit EXC_RETURN[4]. Si es 1*/
	it eq				/* si el resultado es 1 entonces, se ejecuta la siguiente instruccion, sino la ignora */
	vpopeq {s16-s31}

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
