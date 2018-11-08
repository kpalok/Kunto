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


uint8_t CalcState(float ax[], float ay[], float az[], double pres[], int count){
	float xMean, yMean, zMean, zVar, presMean, prevPres, dif;

	xMean = CalcMeanFloat(ax);
	yMean = CalcMeanFloat(ay);
	zMean = CalcMeanFloat(az);
	presMean = CalcMeanDouble(pres);
	zVar = CalcVar(az, zMean);

	//Check if it's first set of data
	if (count == 1){
		prevPres = presMean;
	}
	//Calculate difference between pres of previous values and new values
	dif = presMean - prevPres;
	prevPres = presMean;

	//Check which state user is in

	//Lift Up
	if ((0.0016 <= zVar && zVar <= 0.0043 & dif < 0) || (0.00025 <= zVar && zVar <= 0.0043 && dif < -0.07 && dif > -0.9))
	{
		return 3;
	}
	//Lift Down
	else if ((0.0015 <= zVar && zVar <= 0.0043 && dif > 0) || (0.00025 <= zVar && zVar <= 0.0043 && 0.051 < dif & dif < 0.9))
	{
		return 4;
	}
	//Stairs Up
	else if ((0.05 < zVar || (0.09 < xMean || 0.08 < yMean)) && dif < 0)
	{
		return 1;
	}
	//Stairs Down
	else if ((0.05 < zVar || (0.09 < xMean || 0.08 < yMean)) && dif > 0)
	{
		return 2;
	}
	//Idle
	else
	{
		return 0;
	}
}

