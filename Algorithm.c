/*
 * Algorithm.c
 *
 *  Created on: 6.11.2018
 *      Author: Joona Halkola
 */


#include "Algorithm.h"

float CalcMeanFloat(float *data){
	//Calculate mean for float values
	int i;
	float sum = 0;
	for (i = 0; i < 10; i++){
		sum += data[i];
	}
	return sum / 10;
}


double CalcMeanDouble(double data[]){
	//Calculate mean for double values
	int i;
	double sum = 0;
	for (i = 0; i < 10; i++){
		sum += data[i];
	}
	return sum / 10;
}


float CalcVar(float data[], float avg){
	//Calculate variance
	int i;
	float sum = 0;
	for (i = 0; i < 10; i++){
		sum += (data[i] - avg)*(data[i] - avg);
	}
	return sum / 9;
}


MovementState CalcState(float ax[], float ay[], float az[], double pres[], double previousPres[]){

	float xMean, yMean, zMean, zVar;
	double presMean, prevPresMean, difToLastPres;

	xMean = CalcMeanFloat(ax);
	yMean = CalcMeanFloat(ay);
	zMean = CalcMeanFloat(az);
	presMean = CalcMeanDouble(pres);
	prevPresMean = CalcMeanDouble(previousPres);
	zVar = CalcVar(az, zMean);

	//Calculate difToLastPresference between pres of previous values and new values
 	difToLastPres = presMean - prevPresMean;

	//Check which state user is in

	//Lift Up
	if ((0.0016 <= zVar && zVar <= 0.0043 & difToLastPres < 0) || (0.00025 <= zVar && zVar <= 0.0043 && difToLastPres < -0.07 && difToLastPres > -0.9))
	{
		return LiftUp;
	}
	//Lift Down
	else if ((0.0015 <= zVar && zVar <= 0.0043 && difToLastPres > 0) || (0.00025 <= zVar && zVar <= 0.0043 && 0.051 < difToLastPres && difToLastPres < 0.9))
	{
		return LiftDown;
	}
	//Stairs Up
	else if ((0.05 < zVar || (0.09 < xMean || 0.08 < yMean)) && difToLastPres < 0)
	{
		return StairsUp;
	}
	//Stairs Down
	else if ((0.05 < zVar || (0.09 < xMean || 0.08 < yMean)) && difToLastPres > 0)
	{
		return StairsDown;
	}
	//Idle
	else
	{
		return Idle;
	}
}

