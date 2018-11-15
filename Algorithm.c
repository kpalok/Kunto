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
	if (0.0005 <= zVar & zVar <= 0.005 & difToLastPres < -0.06)
	{
		return LiftUp;
	}
	//Lift Down
	else if (0.00025 <= zVar & zVar <= 0.005 & 0.06 < difToLastPres)
	{
		return LiftDown;
	}
	//Stairs Up
	else if ((yMean < 0.25 | 0.025 < xMean & xMean < 0.35) & 0.08 < zVar & difToLastPres < -0.015)
	{
		return StairsUp;
	}
	//Stairs Down
	else if ((yMean < 0.25 | 0.025 < xMean & xMean < 0.35) & 0.08 < zVar & difToLastPres > 0.015)
	{
		return StairsDown;
	}
	//Idle
	else
	{
		return Idle;
	}
}
