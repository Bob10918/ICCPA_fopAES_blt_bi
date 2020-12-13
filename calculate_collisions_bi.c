/* 
 * File:   calculate_collisions_bi.c
 * Author: Emanuele Pisano
 *
 */
#include "iccpa.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256

#define KEY_SIZE 16     //key size in byte

//functions names definition (generate function name basing on data type passed)
#ifdef TYPE_SUFFIX_f
#define MAKE_FN_NAME(name) name ## _ ## f
#define DATA_TYPE float
#else
#define MAKE_FN_NAME(name) name
#define DATA_TYPE double
#endif
#include "calculations.c"
#define THREAD_ARGS MAKE_FN_NAME(Thread_args)
#define CALCULATE_COLLISIONS MAKE_FN_NAME(calculate_collisions_bi)
#define READ_TRACES MAKE_FN_NAME(read_traces)
#define ANALYZE_TRACES MAKE_FN_NAME(analyze_traces)
#define FIND_COLLISIONS MAKE_FN_NAME(find_collisions)

typedef struct {
    DATA_TYPE*** T;
}THREAD_ARGS;


void READ_TRACES (FILE* input, int M, int plaintextlen, uint8_t plaintexts[M][plaintextlen]);
uint8_t** ANALYZE_TRACES();
void* FIND_COLLISIONS (void* args);


/*
 * Calculate collisions starting from the power traces
 */
uint8_t** CALCULATE_COLLISIONS (FILE* infile){
    
    uint8_t m[M][plaintextlen];
    
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    mkdir("./iccpa_fopaes_bi_temp", 0700);
    
    for(int i=0 ; i<plaintextlen ; i++){
        for(int j=0 ; j<BYTE_SPACE ; j++){
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", j);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "w+");
            fclose(fcurr);
        }
    }
    
    READ_TRACES(infile, m);
    
    fclose(infile);    
    
    return ANALYZE_TRACES();
    
}

/*
 * Read power traces and create the needed temp files
 */
void READ_TRACES (FILE* input, int M, int plaintextlen, uint8_t plaintexts[M][plaintextlen]){
    DATA_TYPE temp1[plaintextlen][l];
    DATA_TYPE temp2[plaintextlen][l];
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    for(int j=0; j<N; j++){
        for(int i=0; i<plaintextlen; i++){
            //read the first l samples (corresponding to the loading of the plaintext byte) 
            fread( temp1[i], sizeof(DATA_TYPE), l, input);
            //skip the next l samples (corresponding to the processing of the byte) 
            fseek(input, l*sizeof(DATA_TYPE) , SEEK_CUR);
            //read the next l samples (corresponding to the storing of the cyphertext byte) 
            fread( temp2[i], sizeof(DATA_TYPE), l, input);
        }
        fseek(input, (nsamples-(l*plaintextlen*3))*sizeof(DATA_TYPE) , SEEK_CUR);
        fread( plaintexts[j], sizeof(char), plaintextlen, input);
        
        for(int i=0; i<plaintextlen; i++){
            uint8_t byte_value = (uint8_t) plaintexts[j][i];
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", byte_value);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "a");
            fwrite(temp1[i], sizeof(DATA_TYPE), l, fcurr);
            fwrite(temp2[i], sizeof(DATA_TYPE), l, fcurr);
            fclose(fcurr);
        }
    }
}

/*
 * Analyze power traces using the temp files created, spawning a thread for every
 * key byte to guess
 */
uint8_t** ANALYZE_TRACES(){
    //T structure: T[BYTE_SPACE/2][n][l*2]
    DATA_TYPE*** T;
    DATA_TYPE temp[l];
    int byte_value;
    
    pthread_t running_threads[KEY_SIZE];
    
    char fcurr_name[50];
    char to_concat[10];   
    FILE* fcurr;
    
    uint8_t** guesses = malloc(sizeof(uint8_t*)*KEY_SIZE);
    
    for(int i=0; i<KEY_SIZE; i++){
        T = malloc(sizeof(DATA_TYPE**)*BYTE_SPACE/2);
        THREAD_ARGS* args = malloc(sizeof(THREAD_ARGS));
        
        for(byte_value=0; byte_value<(BYTE_SPACE/2); byte_value++){
            T[byte_value] = malloc(sizeof(DATA_TYPE*)*n);
            strcpy(fcurr_name, "./iccpa_fopaes_bi_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", byte_value*2);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "r");
            for(int j=0; j<n; j++){
                T[byte_value][j] = malloc(sizeof(DATA_TYPE)*l*2);
                for(int k=0; k<2; k++){ 
                    fread(temp, sizeof(DATA_TYPE), l, fcurr);                   
                    for(int q=0; q<l; q++){
                        T[byte_value][j][k*l+q] = temp[q];
                    }
                }
            }
            fclose(fcurr);
        }
        args->T = T;
        pthread_create(&(running_threads[i]), NULL, FIND_COLLISIONS, (void* ) args);
    }
    void** rv = malloc(sizeof(void*));
    for(int j=0; j<KEY_SIZE; j++){
        pthread_join(running_threads[j], rv);
        if(rv == NULL){
            guesses[j]=NULL;
        }
        else{
            guesses[j] = (uint8_t*) *rv;
        }
    }
    return guesses;
}

/*
 * Check collisions inside a given key byte
 */
void* FIND_COLLISIONS(void* args){
    THREAD_ARGS* args_casted = (THREAD_ARGS*) args;
    DATA_TYPE T[BYTE_SPACE/2][n][l*2] = args_casted->T;
    DATA_TYPE sum_array[2*l];
    DATA_TYPE std_dev_array[2*l];
    DATA_TYPE* max_correlation = malloc(sizeof(DATA_TYPE));
    *max_correlation = 0;
    int found = FALSE;
    uint8_t* guessed_byte = malloc(sizeof(uint8_t));
    int i;
   
    for(i=0; i<(BYTE_SPACE/2); i++){    
        COMPUTE_ARRAYS(n, l*2, T[i], sum_array, std_dev_array);
        if(COLLISION(n, l*2, T[i], 0, l, sum_array, std_dev_array, max_correlation)){
            // if a collision is detected, the corresponding byte value is returned
            found = TRUE;
            *guessed_byte = i*2;
        }
    }
    
    //free memory
    free(max_correlation);
    for(i=0; i<(BYTE_SPACE/2); i++){
        for(int j=0; j<n; j++){
            free(T[i][j]);
        }
        free(T[i]);
    }
    free(T);
    free(args);
    
    if(found){
        pthread_exit(guessed_byte);
    }
    else{
        pthread_exit(NULL);
    }
}
