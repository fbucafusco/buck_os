/*
 * oc_tasks.h
 *
 *  Created on: 11/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_TASKS_H
#define OS_TASKS_H
/*
#include "stdint.h"
*/

//#include "os_internal.h"
#include "os_defs.h"


void _os_task_change_state( tTCB *pTCB , tTaskState state );
void _os_task_block( TASK_COUNT_TYPE index );

#endif /* OS_TASKS_H */
