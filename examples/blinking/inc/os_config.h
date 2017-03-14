/*
 * os_config.h
 *
 *  Created on: 27/2/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_CONFIG_H_
#define OS_CONFIG_H_

/*
 * this module defines fixed os behaviors choosen by the user.
 * */


/*
 * ELEGIR UNA DE POLITICA DE SCHEDULING DE tTaskSchedulePolicy
 */
#define OS_SCHEDULE_POLICY			osSchPolicyPRIORITY //osSchPolicyROUND_ROBIN


/*
 * OS_SCHEDULE_PRIORITY_COOP
 * 1: Tasks with the same priority MUST cooperate with rescheduling points
 * 0: Tasks with the same priority are scheduled as RR.
 */
#define OS_SCHEDULE_PRIORITY_COOP		0


//TODO: implementar time slicing.

/*
 * OS_FIXED_PIORITY
 *
 * 1: The tasks has fixed priority
 * 0: The tasks can change priority in runtime.
 */
#define OS_FIXED_PIORITY			1

/*
 * OS_IDLE_HOOK_STAKC_SIZE
 *
 * Defines the size of the stack for the idle hook (task that runs when no other is ready)
 * If the user do not need to redefine the idle_hook, delete this definition.
 * If the user will redefine the idle_hook, it will need to scale it properly.
 * */
#define OS_IDLE_HOOK_STACK_SIZE		256


#endif /* OS_CONFIG_H_ */
