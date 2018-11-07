/*
 * Algorithm.c
 *
 *  Created on: 6.11.2018
 *      Author: Joona Halkola
 */

#include "Algorithm.h"

float CalcAvg(float *data){
	int i = 0;
	float sum = 0;
	for (i = 0; i < 10; i++){
		sum += data[i];
	}
	return sum / 10;
}

float CalcVar(float *data, float avg){
	int i = 0;
	float sum = 0;
	for (i = 0; i < 10; i++){
		sum += (data[i] - avg)*(data[i] - avg);
	}
	return sum / 9;
}
