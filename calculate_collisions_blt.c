/* 
 * File:   calculate_collisions_blt.c
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
#define BYTE_SPACE 256      //dimension of a byte, i.e. 2^8 = 256

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
#define CALCULATE_COLLISIONS MAKE_FN_NAME(calculate_collisions_blt)
#define READ_DATA MAKE_FN_NAME(read_data)
#define GET_RELATIONS MAKE_FN_NAME(get_relations)
#define FIND_COLLISIONS MAKE_FN_NAME(find_collisions)


//struct to pass to threads as argument
typedef struct {
    DATA_TYPE** T;
    char* m;
    Relation** relations;
}THREAD_ARGS;


void READ_DATA (FILE* input, int M, int plaintextlen, char plaintexts[M][plaintextlen]);
void GET_RELATIONS (int M, char m[M][KEY_SIZE], Relation* relations[KEY_SIZE]);
void* FIND_COLLISIONS (void* args);


/*
 * Read data from input file and create the needed temp files.
 * Then call the method to calculate relations over them.
 */
void CALCULATE_COLLISIONS (FILE* infile, Relation* relations[KEY_SIZE]){
    
    char m[M][plaintextlen];
    
    char fcurr_name[50];
    char to_concat[10];    

    FILE* curr;
    mkdir("./iccpa_fopaes_blt_temp", 0700);
    for(int i=0 ; i<plaintextlen ; i++){
        for(int j=0 ; j<BYTE_SPACE ; j++){
            strcpy(fcurr_name, "./iccpa_fopaes_blt_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", j);
            strcat(fcurr_name, to_concat);
            curr = fopen(fcurr_name, "w+");
            fclose(curr);
        }
    }
    
    READ_DATA(infile, M, plaintextlen, m);
    
    fclose(infile);    
    
    GET_RELATIONS(M, m, relations);
    
}

/*
 * Correctly reads data from the file, basing on the data type
 */
void READ_DATA(FILE* input, int M, int plaintextlen, char plaintexts[M][plaintextlen]){
    DATA_TYPE temp[plaintextlen][l];
    FILE* fcurr;
    char fcurr_name[50];
    char to_concat[10];
    
    for(int j=0; j<N; j++){
        //read the first l*plaintextlen samples (corresponding to first round computation)
        for(int i=0; i<plaintextlen; i++){
            fread( temp[i], sizeof(DATA_TYPE), l, input);
        }
        //skip the rest of the computation, useless for the attack
        fseek(input, (nsamples-(l*plaintextlen))*sizeof(DATA_TYPE) , SEEK_CUR);
        //read the plaintext
        fread( plaintexts[j], sizeof(char), plaintextlen, input);
        
        //store the data read in the relative temp files
        for(int i=0; i<plaintextlen; i++){
            uint8_t byte_value = (uint8_t) plaintexts[j][i];
            strcpy(fcurr_name, "./iccpa_fopaes_blt_temp/");
            sprintf(to_concat, "%d", i+1);
            strcat(fcurr_name, to_concat);
            strcat(fcurr_name, "_");
            sprintf(to_concat, "%d", byte_value);
            strcat(fcurr_name, to_concat);
            fcurr = fopen(fcurr_name, "a");
            fwrite(temp[i], sizeof(DATA_TYPE), l, fcurr);
            fclose(fcurr);            
        }
    }
}




/*
 * Infer relations abut the key starting from the samples, storing them all in an array of linked lists
 * Spawn a thread for every trace
 */
void GET_RELATIONS(int M, char m[M][KEY_SIZE], Relation* relations[KEY_SIZE]){
    DATA_TYPE** T;
    DATA_TYPE temp[l];
    int i=0, threads_count=0;
    pthread_mutex_init(&mutex_relations, NULL);
    
    pthread_t running_threads[max_threads];
    void* ul[1];
    ul[0] = malloc(sizeof(int));
    
    FILE* fcurr;
    char fcurr_name[50];
    char to_concat[10];
    
    for(i=0; i<KEY_SIZE; i++){
        relations[i]=NULL;
    }
    while(i<M){
        while(i<M && threads_count<max_threads){
            T = malloc(sizeof(DATA_TYPE*)*n);
            THREAD_ARGS *args = malloc(sizeof(THREAD_ARGS));

            for(int j=0; j<n; j++){
                T[j] = malloc(sizeof(DATA_TYPE)*l*KEY_SIZE);
                for(int k=0; k<plaintextlen; k++){
                    uint8_t byte_value = (uint8_t) m[i][k];
                    strcpy(fcurr_name, "./iccpa_fopaes_blt_temp/");
                    sprintf(to_concat, "%d", k+1);
                    strcat(fcurr_name, to_concat);
                    strcat(fcurr_name, "_");
                    sprintf(to_concat, "%d", byte_value);
                    strcat(fcurr_name, to_concat);
                    fcurr = fopen(fcurr_name, "r");
                    fread(temp, sizeof(DATA_TYPE), l, fcurr);
                    fclose(fcurr);
                    for(int q=0; q<l; q++){
                        T[j][k*l+q] = temp[q];
                    }
                }
            }

            args->T = T;
            args->m = m[i];
            args->relations = relations;
            pthread_create(&(running_threads[threads_count]), NULL, FIND_COLLISIONS, (void* ) args);
            
            i++;
            threads_count++;
        }
        for(int j=0; j<threads_count; j++){
            pthread_join(running_threads[j], ul);
        }
        threads_count = 0;
    }
    free(ul[0]);
}

/*
 * Check collisions for a given power trace 
 */
void* FIND_COLLISIONS(void* args){
    THREAD_ARGS* args_casted = (THREAD_ARGS*) args;
    DATA_TYPE** T = args_casted->T;
    char* m = args_casted->m;
    Relation** relations = args_casted->relations;
    DATA_TYPE sum_array[KEY_SIZE*l];
    DATA_TYPE std_dev_array[KEY_SIZE*l];
    DATA_TYPE* max_correlation = malloc(sizeof(DATA_TYPE));
    
    COMPUTE_ARRAYS(n, l*KEY_SIZE, T, sum_array, std_dev_array);
    for(int j=0; j<KEY_SIZE; j++){
        for(int k=j+1; k<KEY_SIZE; k++){
            *max_correlation = -1;
            if(COLLISION(n, l*KEY_SIZE, T, j*l, k*l, sum_array, std_dev_array, max_correlation)){
                /* if a collision is detected, two new relations are created:
                 * from byte j to k and viceversa. This to achieve better 
                 * performance when searching for a relation involving a 
                 * specific byte
                 */
                pthread_mutex_lock(&mutex_relations); 
                
                Relation* new_relation = malloc(sizeof(Relation));
                char new_value = (m[j])^(m[k]);
                new_relation->in_relation_with = k;
                new_relation->value = new_value;
                new_relation->next = relations[j];
                relations[j] = new_relation;

                new_relation = malloc(sizeof(Relation));
                new_relation->in_relation_with = j;
                new_relation->value = new_value;
                new_relation->next = relations[k];
                relations[k] = new_relation;
                
                pthread_mutex_unlock(&mutex_relations); 
            }
        }      
    }
    
    //free memory allocated for thread argument
    for(int j=0; j<n; j++){
        free(T[j]);
    }
    free(T);
    free(args_casted);
    pthread_exit(NULL);
}