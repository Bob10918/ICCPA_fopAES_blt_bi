/* 
 * File:   calculations.c
 * Author: Emanuele Pisano
 *
 */

#include "iccpa.h"
#include <tgmath.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256      //dimension of a byte, i.e. 2^8 = 256

#define KEY_SIZE 16     //key size in byte
#define KEY_SIZE_INT KEY_SIZE/(sizeof(int)/sizeof(char))    //key size in integers



#define STANDARD_DEVIATION MAKE_FN_NAME(standard_deviation)
#define COVARIANCE MAKE_FN_NAME(covariance)
#define OPTIMIZED_PEARSON MAKE_FN_NAME(optimized_pearson)
#define COLLISION MAKE_FN_NAME(collision)
#define COMPUTE_ARRAYS MAKE_FN_NAME(compute_arrays)



int COLLISION (int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples], DATA_TYPE* max_correlation);
void COMPUTE_ARRAYS(int n, int samples, DATA_TYPE T[n][samples], DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples]);
DATA_TYPE OPTIMIZED_PEARSON(int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples]);
DATA_TYPE COVARIANCE(int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, int t, DATA_TYPE sum_array[samples]);
DATA_TYPE STANDARD_DEVIATION(int n, int samples, DATA_TYPE T[n][samples], int theta, int t);

/*
 * Decision function returning true or false depending on whether the same data was involved in two given instructions theta0 and theta1 
 * compare the value of a synthetic criterion with a practically determined threshold
 * as criterion we used the maximum Pearson correlation factor
 * theta0 and theta1 are time instant at which instruction starts (for performance reasons)
 */
int COLLISION (int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples], DATA_TYPE* max_correlation){
    
    DATA_TYPE correlation = OPTIMIZED_PEARSON(n, samples, T, theta0, theta1, sum_array, std_dev_array);
    //check if max is greater than threshold
    if(correlation>threshold && correlation>*max_correlation){
        *max_correlation = correlation;
        return TRUE;
    }
    else{
        return FALSE;
    }
}

/*
 * Efficiently compute in one step both sum and standard deviation
 */
void COMPUTE_ARRAYS(int n, int samples, DATA_TYPE T[n][samples], DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples]){
    DATA_TYPE sum;
    DATA_TYPE squared_sum;
    DATA_TYPE std_dev;
    for(int i=0; i<samples; i++){
        sum = 0;
        squared_sum = 0;
        for(int j=0; j<n; j++){
            sum += T[j][i];
            squared_sum += T[j][i]*T[j][i];
        }
        sum_array[i] = sum;
        std_dev = (n*squared_sum)-(sum*sum);
        // check for unreal values caused by floating point precision
        if(std_dev < 0){
            std_dev_array[i] = 0;
        }
        else{
            std_dev_array[i] = sqrt(std_dev);
        }
    }
}

/*
 * Efficiently compute pearson correlation factor
 */
DATA_TYPE OPTIMIZED_PEARSON(int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, DATA_TYPE sum_array[samples], DATA_TYPE std_dev_array[samples]){
    int i;
    DATA_TYPE correlation;
    DATA_TYPE max_correlation = 0;
    for(i=0; i<l; i++){
        // check for arithmetic overflow
        if(std_dev_array[theta0+i] != 0 && std_dev_array[theta1+i] != 0){
            correlation = COVARIANCE(n, samples, T, theta0, theta1, i, sum_array)  /  ( std_dev_array[theta0+i] * std_dev_array[theta1+i] );
        }
        else{
            max_correlation = 1;
            break;
        }
        if(correlation > max_correlation){
            max_correlation = correlation;
        }
    }
    return max_correlation;
}

/*
 * Compute covariance of two given time sequences of samples
 */
DATA_TYPE COVARIANCE(int n, int samples, DATA_TYPE T[n][samples], int theta0, int theta1, int t, DATA_TYPE sum_array[samples]){
    int i;
    DATA_TYPE first_sum=0;
    for(i=0; i<n; i++){
        first_sum += (T[i][theta0+t]*T[i][theta1+t]);
    }
    return (n*first_sum)-(sum_array[theta0+t]*sum_array[theta1+t]);
}

/*
 * Compute standard deviation of a given time sequence of samples
 */
DATA_TYPE STANDARD_DEVIATION(int n, int samples, DATA_TYPE T[n][samples], int theta, int t){
    int i;
    DATA_TYPE first_sum=0, second_sum=0;
    for(i=0; i<n; i++){
        first_sum += (T[i][theta+t])*(T[i][theta+t]);
        second_sum += T[i][theta+t];
    }
    return sqrt((n*first_sum)-(second_sum*second_sum));
}