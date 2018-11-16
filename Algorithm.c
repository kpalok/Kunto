/*
 * Algorithm.c
 *
 *  Created on: 6.11.2018
 *      Author: Joona Halkola
 *      		Kalle Palokangas
 */


#include "Algorithm.h"

/*Calculate mean for float values*/
float CalcMeanFloat(float data[], int elementCount){

	int i;
	float sum = 0;

	for (i = 0; i < elementCount; i++){

		sum += data[i];
	}

	return sum / elementCount;
}

/*Calculate mean for double values*/
double CalcMeanDouble(double data[], int elementCount){

	int i;
	double sum = 0;

	for (i = 0; i < elementCount; i++){

		sum += data[i];
	}

	return sum / elementCount;
}

/*Calculate variance*/
float CalcVar(float data[], float avg, int elementCount){

	int i;
	float sum = 0;

	for (i = 0; i < elementCount; i++){

		sum += (data[i] - avg)*(data[i] - avg);
	}

	return sum / (elementCount - 1);
}

/* Returns movement state based on measured data set.
 * Assumes all arrays are of same length (defined by element count).*/
MovementState CalcState(float ax[], float ay[], float az[], double pres[], double previousPres[], int elementCount){

	float xMean, yMean, zMean, zVar;
	double presMean, prevPresMean, difToLastPres;

	xMean = CalcMeanFloat(ax, elementCount);
	yMean = CalcMeanFloat(ay, elementCount);
	zMean = CalcMeanFloat(az, elementCount);

	presMean = CalcMeanDouble(pres, elementCount);
	prevPresMean = CalcMeanDouble(previousPres, elementCount);

	zVar = CalcVar(az, zMean, elementCount);

	//Calculate difference to last measured pressure mean
 	difToLastPres = presMean - prevPresMean;

	//Lift Up
	if (0.00025 <= zVar && zVar <= 0.006 && difToLastPres < -0.047)
	{
		return LiftUp;
	}
	//Lift Down
	else if (0.00025 <= zVar && zVar <= 0.006 && difToLastPres > 0.047)
	{
		return LiftDown;
	}
	//Stairs Up
	else if (((0.07 < abs(xMean) && abs(xMean) < 0.35) || (abs(yMean) < 0.065 && abs(yMean) < 0.25)) && 0.085 < zVar && difToLastPres < -0.017)
	{ö
		return StairsUp;
	}
	//Stairs Down
	else if (((0.07 < abs(xMean) && abs(xMean) < 0.3) || (abs(yMean) < 0.065 && abs(yMean) < 0.25))  && 0.095 < zVar && difToLastPres > 0.018)
	{
		return StairsDown;
	}
	//Idle
	else
	{
		return Idle;
	}
}
