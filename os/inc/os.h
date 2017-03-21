/*
 * os_.h
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_H
#define OS_H

#include "os_internal.h"


/* ********** USER PUBLIC INTERFACES ********** */

/*
It declares a task Object. This is intended to be used for FCNNAME reuse.
TASKNAME: Fantasy Name. It could be the same name as the function name that represents the task behavior.
FCNNAME:  Function that represents the task behavior defined as

          void FCNNAME( void* arg )
          {
          }

ARG: A pointer to an object that will be passed to FCNNAME at the beginning of its execution
STACKSIZE: Stack size for the task. Must be a number grater than MIN_STACK_SIZE
PRI:       Priority for the task. The high number the higher priority
CONFIG:     Configuration Flags ORED: FLAG1 | FLAG2 ...
            TASK_AUTOSTART : If configured, the task will be ready just after calling startOS.
*/
#define DECLARE_TASK_R( TASKNAME , FCNNAME , ARG , STACKSIZE, PRI, CONFIG )     _I_DECLARE_TASK_R( TASKNAME , FCNNAME , ARG , STACKSIZE, PRI, CONFIG )

/*
 Same as DECLARE_TASK_R but it uses the function name as the TASKNAME.
*/
#define DECLARE_TASK( FCNNAME , ARG , STACKSIZE, PRI, CONFIG)                   DECLARE_TASK_R( FCNNAME , FCNNAME , ARG , STACKSIZE, PRI, CONFIG )

/* Starts the declaration for the OS object */
#define DECLARE_OS_START()                                                      _I_OS_TASKS_START()

/* Ends the declaration for the OS object */
#define DECLARE_OS_END()                                                        _I_OS_TASKS_END()

/* Adds a task to the OS
TASKNAME: Fantasy Name. The same name used in DECLARE_TASK_R
*/
#define OS_ADD_TASK(TASKNAME)                                                   _I_OS_ADD_TASK(TASKNAME)

/*
Gets a reference for a task.
TASKNAME: Fantasy Name. The same name used in DECLARE_TASK_R
*/
#define OS_TASK_REF(TASKNAME)                                                   _I_TASK_REF(TASKNAME)


/* os methods */
void osTaskYield();

#endif /* OS_H */
