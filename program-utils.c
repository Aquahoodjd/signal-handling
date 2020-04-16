#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include "program-utils.h"

void least_squares_method(char coordinates[LENGTH], char line[BLOCK])
{
    double X_sum = 0, 
		   Y_sum = 0, 
		   X_2_sum = 0, 
		   XY_sum = 0;
    char line_estimation[BLOCK];

    int X, Y;

    for (int i = 0 ; i < LENGTH ; i++)
    {
    	X = (int)((unsigned char) coordinates[i]);
    	Y = (int)((unsigned char) coordinates[++i]);

        X_sum += X;
        Y_sum += Y;
		XY_sum += X * Y;
        X_2_sum += (X * X);
    }
	double a = 0, b = 0;
    a = ((TOT_CRDNT * XY_sum) - (X_sum * Y_sum)) / ((TOT_CRDNT * X_2_sum) - (X_sum * Y_sum));
    b = (Y_sum - (a * X_sum)) / TOT_CRDNT;
    sprintf(line_estimation, " %.3lfx+%.3lf\n", a, b);
    strcat(line, line_estimation);
}


void set_metrics(char line[], 
				 double mean_absolute_err[], 
				 double mean_squared_err[], 
				 double root_mean_squared_err[], 
				 int index)
{
	char temp[BLOCK] = "";
	int count = 0;
	int x_coordinate[10];
    int y_coordinate[10];
	double mean_absolute_error = 0; 
	double mean_squared_error = 0;
	double root_mean_squared_error = 0;
	double a = 0, b = 0, nuy = 0;

	for(int i = 0; i < strlen(line); i++)
	{
		if(count >= LENGTH)
		{
		    if(line[i] != ' ')
    		{
    			if(line[i] == 'x' || i == strlen(line) - 1)
    			{
    				if(count % 2 == 0) a = atof(temp);
    				else b = atof(temp);
    				strcpy(temp, "");
    				count++;
    			}
    			else strncat(temp, &line[i], 1);
    		}
		}

		else if(line[i] != '(' && line[i] != ')' && line[i] != ' ')
		{
			if(line[i] == ',')
			{
				if(count % 2 == 0) x_coordinate[count/2] = atoi(temp);
				else y_coordinate[count/2] = atoi(temp);
				strcpy(temp, "");
				count++;
			}
			else strncat(temp, &line[i], 1);
		}
	}
	
	for(int i = 0 ; i < TOT_CRDNT; i++)
	{
	    nuy = (a * x_coordinate[i]) + b;
		mean_absolute_error += abs(nuy - y_coordinate[i]);
		mean_squared_error += ((nuy - y_coordinate[i]) * (nuy - y_coordinate[i]));
	}

	mean_absolute_error /= TOT_CRDNT;
	mean_squared_error /= TOT_CRDNT;
	root_mean_squared_error = sqrt(mean_squared_error);
	mean_absolute_err[index] = round(mean_absolute_error * 1000) / 1000;
	mean_squared_err[index] = round(mean_squared_error * 1000) / 1000;
	root_mean_squared_err[index] = round(root_mean_squared_error * 1000) / 1000;

	strcpy(temp, "");
	sprintf(temp, ", %.3lf, %.3lf, %.3lf\n", mean_absolute_error, 
											 mean_squared_error, 
											 root_mean_squared_error);
    strcat(line,temp);
}


double standard_deviation(double metric[], double mean, int count)
{
    double standard_deviation = 0;

    for (int i = 0; i < count; i++)
    {
        standard_deviation += ((metric[i] - mean) * (metric[i] - mean));
    }
    standard_deviation = sqrt(standard_deviation / count);
    standard_deviation = round(standard_deviation * 1000) / 1000;
    return standard_deviation;
}

double median_deviation(double metric[], double mean, int count)
{
    double total = 0;

    for(int i = 0; i < count; i++)
    {
        total = total + abs(mean - metric[i]);
    }

    total = total / count;
    total = round(total * 1000) / 1000;

    return total;
}

double mean(double metric[], int count)
{
	double total = 0;

	for(int i = 0; i < count; i++)
	{
		total += metric[i];
	}

	total /= count;
	total = round(total * 1000) / 1000;

	return total;
}