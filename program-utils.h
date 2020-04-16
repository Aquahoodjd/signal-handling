#ifndef PROGRAM_UTILS_H
#define PROGRAM_UTILS_H

#define LENGTH 20
#define BLOCK 1024
#define TOT_CRDNT 10
#define OPTION_ERROR "Invalid option usage!\nEnter like that: ./programA -i inputPath -o outputPath\n"
#define TRUE 1

/* applies least squares method */
void least_squares_method(char*, char*);
/* calculates metrics to need to calculate */
void set_metrics(char*, double*, double*, double*, int);
/* calculates the standard deviation value*/ 
double standard_deviation(double*, double, int);
/* calculates the standard deviation value*/ 
double median_deviation(double*, double, int);
/* calculates mean value */
double mean(double*, int);
#endif