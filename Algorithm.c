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
	for (i = 0; i < 15; i++){
		sum += data[i];
	}
	return sum / 15;
}


double CalcMeanDouble(double data[]){
	//Calculate mean for double values
	int i;
	double sum = 0;
	for (i = 0; i < 15; i++){
		sum += data[i];
	}
	return sum / 15;
}


float CalcVar(float data[], float avg){
	//Calculate variance
	int i;
	float sum = 0;
	for (i = 0; i < 15; i++){
		sum += (data[i] - avg)*(data[i] - avg);
	}
	return sum / 14;
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
	if (0.00016 <= zVar && zVar <= 0.0043 && zMean < -0.07 && difToLastPres < -0.055 && difToLastPres > -0.9)
	{
		return LiftUp;
	}
	//Lift Down
	else if (0.00025 <= zVar && zVar <= 0.0045 && zMean < -0.07 && 0.055 < difToLastPres && difToLastPres < 0.9)
	{
		return LiftDown;
	}
	//Stairs Up
	else if (((0.01 < yVar && yVar < 0.12) || (0.06 < yMean && yMean < 0.3) || (0.07 < xMean && xMean < 0.3)) && 0.06 < zVar && zMean < -0.45 && difToLastPres < -0.008)
	{
		return StairsUp;
	}
	//Stairs Down
	else if (((0.01 < yVar && yVar < 0.12)|| (0.06 < yMean && yMean < 0.3) || (0.07 < xMean && xMean < 0.3)) && 0.06 < zVar && zMean < -0.45 && difToLastPres > 0.008)
	{
		return StairsDown;
	}
	//Idle
	else
	{
		return Idle;
	}
}

