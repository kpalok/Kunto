/*
 * Algorithm.h
 *
 *  Created on: 6.11.2018
 *      Author: Joona Halkola
 *      		Kalle Palokangas
 */

#ifndef ALGORITHM_H_
#define ALGORITHM_H_

#include <inttypes.h>

typedef enum movementState{
	Idle = 0,
	StairsUp = 1,
	StairsDown = 2,
	LiftUp = 3,
	LiftDown = 4
} MovementState;

MovementState CalcState(float ax[], float ay[], float az[], double pres[], double previousPres[], int elementCount);

#endif /* ALGORITHM_H_ */
