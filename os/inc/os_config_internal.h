/*
 * os_config.h
 *
 *  Created on: 27/2/2017
 *      Author: franco.bucafusco
 */

#ifndef OS_CONFIG_INTERNAL_H_
#define OS_CONFIG_INTERNAL_H_

/*
 * this module defines fixed os behaviors fixed
 *
 * the modification of anything here, NEEDS more knowledge from the programmer.
 * */


/*
 * OS_INTERNAL_DELAY_WITH_MAIN_COUNTER
 *
 * 1: El os consume algo mas de memoria porque gestiona los delays a partir de un contador principal.
 *    que se va decrementando, y al dar timeout, se actualizan los individuales por tarea.
 *    Tiene la ventaja de que el proceso de delays en el tick del sistema es mucho mas agil.
 * 0: El tick gestiona los delays directamente con el registro de delay de la tarea.
 * */
#define OS_INTERNAL_DELAY_WITH_MAIN_COUNTER	1

#endif /* OS_CONFIG_INTERNAL_H_ */
