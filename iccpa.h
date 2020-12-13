/* 
 * File:   iccpa.h
 * Author: Emanuele Pisano
 *
 */

#ifndef ICCPA_H
#define ICCPA_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

int N;        //number of power traces captured from a device processing N encryptions of the same message
int n;     //number of encryption of the same message
int nsamples;   //total number of samples per power trace
int l;        //number of samples per instruction processing (clock)
char sampletype;    //type of samples, f=float  d=double
int plaintextlen;   //length of plaintext in bytes
int M;        //number of plaintext messages, each one being encrypted n times
float threshold;    //threshold for collision determination

int max_threads;    //max number of threads to run simultaneously
pthread_mutex_t mutex_relations;

typedef struct Relation_s{
    int in_relation_with;
    char value;
    struct Relation_s* next;
}Relation;


//void calculate_collisions_blt_f(FILE* infile, Relation** relations);
//void calculate_collisions_blt(FILE* infile, Relation** relations);
//
//uint8_t** calculate_collisions_bi_f(FILE* infile);
//uint8_t** calculate_collisions_bi(FILE* infile);

#endif

