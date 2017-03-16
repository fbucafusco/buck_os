#include "main.h"
#include "board.h"
#include "os.h"
#include <string.h>

typedef struct
{
	float 		value;
    float       init_value;
} tVariableFloat;




/* ************************
 * PARAMETERS FOR EACH TASK
 * ************************ */
tVariableFloat variables[] =
{
    [0] =
    {
        .init_value = 100,
    },

    [1] =
    {
        .init_value = 200,
    },

    [2] =
    {
        .init_value = 300,
    },
};

/* *******************
 * OS CONFIGURATION
 * ******************* */

/* DECLARACION DE TAREA 1 */
DECLARE_TASK_R( task1 , task_add_float , &variables[0] , MAX_STACK_SIZE , 2 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 2 */
DECLARE_TASK_R( task2 , task_add_float ,  &variables[1] , MAX_STACK_SIZE , 2 , TASK_AUTOSTART )

/* DECLARACION DE TAREA 3 */
DECLARE_TASK_R( task3 , task_add_float ,  &variables[2] , MAX_STACK_SIZE , 2 , TASK_AUTOSTART )


DECLARE_OS_START()		/* INICIO DE CONFIGURACION DE OS */
OS_ADD_TASK( task1 )
OS_ADD_TASK( task2 )
OS_ADD_TASK( task3 )
DECLARE_OS_END()		/* FIN DE CONFIGURACION DE OS */




/* sobrecargo la idle hook */
void idle_hook( void*arg )
{
    int j;
    while( 1 )
    {
    }
}

/* the task increments a float from a stating value of arg*/
void task_add_float( void* arg )
{

	tVariableFloat *pVar = (tVariableFloat*) arg;

	pVar->value = pVar->init_value;

    while( 1 )
    {
    	pVar->value++;
    }
}


int main( void )
{
    osStart();
}

/*==================[end of file]============================================*/

