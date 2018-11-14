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
	for (i = 0; i < 20; i++){
		sum += data[i];
	}
	return sum / 20;
}


double CalcMeanDouble(double data[]){
	//Calculate mean for double values
	int i;
	double sum = 0;
	for (i = 0; i < 20; i++){
		sum += data[i];
	}
	return sum / 20;
}


float CalcVar(float data[], float avg){
	//Calculate variance
	int i;
	float sum = 0;
	for (i = 0; i < 20; i++){
		sum += (data[i] - avg)*(data[i] - avg);
	}
	return sum / 19;
}


MovementState CalcState(float ax[], float ay[], float az[], double pres[], double previousPres[]){

	float xMean, yMean, zMean, yVar, zVar;
	double presMean, prevPresMean, difToLastPres;

	xMean = CalcMeanFloat(ax);
	yMean = CalcMeanFloat(ay);
	zMean = CalcMeanFloat(az);
	presMean = CalcMeanDouble(pres);
	prevPresMean = CalcMeanDouble(previousPres);
	yVar = CalcVar(ay, yMean);
	zVar = CalcVar(az, zMean);

	//Calculate difToLastPresference between pres of previous values and new values
 	difToLastPres = presMean - prevPresMean;

	//Check which state user is in

	//Lift Up
	if (0.0006 <= zVar && zVar <= 0.0043 && difToLastPres < -0.08 && difToLastPres)
	{
		return LiftUp;
	}
	//Lift Down
	else if (0.00025 <= zVar && zVar <= 0.0045 && 0.06 < difToLastPres && difToLastPres)
	{
		return LiftDown;
	}
	//Stairs Up
	else if (0.07 < xMean && xMean < 0.4 && 0.07 < zVar && zMean < -0.58 && difToLastPres < -0.03)
	{
		return StairsUp;
	}
	//Stairs Down
	else if (((0.04 < yMean && yMean < 0.3) || (0.045 < xMean && xMean < 0.25)) && 0.07 < zVar && zMean < -0.58 && difToLastPres > 0.032)
	{
		return StairsDown;
	}
	//Idle
	else
	{
		return Idle;
	}
}
