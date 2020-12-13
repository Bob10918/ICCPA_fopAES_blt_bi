/* 
 * File:   main_bi.c
 * Author: Emanuele Pisano
 *
 * Created on 19 agosto 2020, 10.51
 */

#include "iccpa.h"
#define TYPE_SUFFIX_f
#include "calculate_collisions_bi.c"
#undef TYPE_SUFFIX_f
#define TYPE_SUFFIX
#include "calculate_collisions_bi.c"
#undef TYPE_SUFFIX

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256

#define KEY_SIZE 16     //key size in byte

void help();
void print_guesses(int index, uint8_t** guesses);

int main(int argc, char** argv) {
    
    char* file_name = NULL;
    
    //Set parameters to default value
    n = 30;     //number of power traces for each message
    l = 15;     //number of samples per clock
    threshold = 0.7;        //threshold beyond which collision is accepted
    M = -1;     //number of different messages, each one being encrypted n times (set to -1 to check if user insert a different value)
    
    //Read parameters from command line argument
    opterr = 0;
    char c;
    while ((c = getopt (argc, argv, "f:n:l:m:t:h")) != -1)
        switch (c)
        {
        case 'f':
            file_name = optarg;
            break;
        case 'n':
            n = atoi(optarg);
            break;
        case 'l':
            l = atoi(optarg);
            break;
        case 'm':
            M = atoi(optarg);
            break;
        case 't':
            threshold = (float) strtof(optarg, NULL);
            break;
        case 'h':
            help();
            return 1;
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            return 1;
            abort ();
        }
      
    if(file_name==NULL){
        printf("Parameter -f is required\n");
        return 1;
    }  
    FILE* infile = fopen(file_name, "r");
    fread(&N, sizeof(uint32_t), 1, infile);
    if(M==-1) M = N;      //set to N if user hasn't inserted a custom value
    fread(&nsamples, sizeof(uint32_t), 1, infile);
    fread(&sampletype, sizeof(char), 1, infile);
    uint8_t plaintextlen_temp;
    fread(&plaintextlen_temp, sizeof(uint8_t), 1, infile);
    plaintextlen = (int) plaintextlen_temp;
    
    uint8_t* guesses[KEY_SIZE];
    
    switch (sampletype){
        case 'f':
            guesses = calculate_collisions_bi_f(infile);
          break;

        case 'd':
            guesses = calculate_collisions_bi(infile);
          break;

        default:
            exit(-1);
    }
    
    print_guesses(0, guesses);
    
    //free memory
    for(int i=0; i<KEY_SIZE; i++){
        if(guesses[i]!=NULL){
            free(guesses[i]);
        }
    }
    free(guesses);
    return (EXIT_SUCCESS);
}

/*
 * Show a brief help
 */
void help(){
    printf("Improved Collision-Correlation Power Analysis on First Order Protected AES (for Blinded Inversion implementations)\n");
    printf("Usage: iccpa_fopaes_bi <file> <options>\n");
    printf("    <file> argument is required\n");
    printf("    Options:\n");
    printf("    -n      Specify number of power traces for each message (default: 30)\n");
    printf("    -l      Specify number of samples per clock (default: 15)\n");
    printf("    -m      Specify number of different messages, each one being encrypted n times (default: number of power traces in given file)\n");
    printf("    -t      Specify threshold beyond which collision is accepted, in range 0 to 1 (default: 0.9)\n");
    printf("    -h      Show this help\n");
}

/*
 * Recursive functions that prints all the possible key starting from guesses
 */
void print_guesses(int index, uint8_t* guesses[KEY_SIZE]){
    if(index==KEY_SIZE){
        for(int i=0; i<KEY_SIZE; i++){
            if(guesses[i] == NULL){
                printf("XX");
            }
            else{
                printf("%02x", *(guesses[i])& 0xff);
            }
        }
        printf("\n");
        return;
    }
    else{
        if(guesses[index]==NULL){
            print_guesses(index+1, guesses);
            return;
        }
        else{
            print_guesses(index+1, guesses);
            
            *(guesses[index]) += 1;
            print_guesses(index+1, guesses);
            *(guesses[index]) -= 1;
            return;
        }
    }
}