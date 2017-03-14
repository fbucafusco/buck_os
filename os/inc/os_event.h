/*
 * os_Event.h
 *
 *  Created on: 10/3/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_EVENT_H_
#define OS_EVENT_H_

#include "os_defs.h"

OS_EVENT_TYPE osWaitEvent( OS_EVENT_TYPE events );
void osSetEvent( OS_EVENT_TYPE events );
void osSetEvent_T( OS_EVENT_TYPE events , void *task_ref );


#endif /* OS_EVENT_H_ */
