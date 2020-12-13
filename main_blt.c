/* 
 * File:   main_blt.c
 * Author: Emanuele Pisano
 *
 * Created on 19 agosto 2020, 10.51
 */

#include "iccpa.h"
#define TYPE_SUFFIX_f
#include "calculate_collisions_blt.c"
#undef TYPE_SUFFIX_f
#define TYPE_SUFFIX
#include "calculate_collisions_blt.c"
#undef TYPE_SUFFIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define TRUE 0xff
#define FALSE 0
#define BYTE_SPACE 256      //dimension of a byte, i.e. 2^8 = 256

#define KEY_SIZE 16     //key size in byte
#define KEY_SIZE_INT KEY_SIZE/(sizeof(int)/sizeof(char))    //key size in integers


pthread_mutex_t mutex_printf = PTHREAD_MUTEX_INITIALIZER;


typedef struct Subkey_element_s{
    char subkeys[BYTE_SPACE][KEY_SIZE];
    struct Subkey_element_s* next;
}Subkey_element;

typedef struct Printkey_thread_args_s{
    Relation** relations;
    uint8_t unkn_byte;
}Printkey_thread_args;


void help();
void guess_key(Relation* relations[KEY_SIZE]);
void* print_key(void* args);
void guess_key_optimized(Relation** relations);
Subkey_element* guess_subkey(int to_guess, Relation** relations, int* guessed);
void resolve_relations(int start, Relation** relations, uint8_t* new_guessed, uint8_t* xor_array);
void combine_subkeys(Subkey_element* subkeys_list, int* partial_key);


int main(int argc, char** argv) {
    
    char* file_name = NULL;
    
    //Set parameters to default value
    n = 30;     //number of power traces for each message
    l = 15;     //number of samples per clock
    threshold = 0.9;        //threshold beyond which collision is accepted
    max_threads = 50;       //maximum number of threads to spawn
    M = -1;     //number of different messages, each one being encrypted n times (set to -1 to check if user insert a different value)
    
    //Read parameters from command line argument
    opterr = 0;
    char c;
    while ((c = getopt (argc, argv, "f:n:l:m:t:x:h")) != -1)
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
        case 'x':
            max_threads = atoi(optarg);
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
   
    
    Relation* relations[KEY_SIZE];
    
    switch (sampletype){
        case 'f':
            calculate_collisions_blt_f(infile, relations);
          break;

        case 'd':
            calculate_collisions_blt(infile, relations);
          break;

        default:
            exit(-1);
    }
    
    guess_key(relations);
    
    //free memory allocated for relations lists
    Relation* pointer_relations_current;
    Relation* pointer_relations_next;
    for(int i=0; i<KEY_SIZE; i++){
        pointer_relations_current = relations[i];
        if(pointer_relations_current!=NULL){
            pointer_relations_next = pointer_relations_current->next;
            while(pointer_relations_next!=NULL){
                free(pointer_relations_current);
                pointer_relations_current = pointer_relations_next;
                pointer_relations_next = pointer_relations_current->next;
            }
            free(pointer_relations_current);
        }
    }
    
    return (EXIT_SUCCESS);
}

/*
 * Show a brief help
 */
void help(){
    printf("Improved Collision-Correlation Power Analysis on First Order Protected AES (for Blinded Lookup Table implementations)\n");
    printf("Usage: iccpa_fopaes_blt <file> <options>\n");
    printf("    <file> argument is required\n");
    printf("    Options:\n");
    printf("    -n      Specify number of power traces for each message (default: 30)\n");
    printf("    -l      Specify number of samples per clock (default: 15)\n");
    printf("    -m      Specify number of different messages, each one being encrypted n times (default: number of power traces in given file)\n");
    printf("    -t      Specify threshold beyond which collision is accepted, in range 0 to 1 (default: 0.9)\n");
    printf("    -x      Specify maximum number of threads to spawn during computations (default: 50)\n");
    printf("    -h      Show this help\n");
}

/*
 * Guess the key basing on the infered relations, starting from an unknown byte
 */
void guess_key(Relation** relations){
    pthread_t id;
    for(int i=0; i<BYTE_SPACE; i++){
        Printkey_thread_args* args = malloc(sizeof(Printkey_thread_args));
        args->relations = relations;
        args->unkn_byte = i;
        pthread_create(&id, NULL, print_key, args);
    }
    return;
}

/*
 * Given an unknown byte, compute all the other key bytes according to the relations 
 */
void* print_key(void* args){
    Printkey_thread_args* args_casted = (Printkey_thread_args*) args;
    Relation* relations[KEY_SIZE] = args_casted->relations;
    uint8_t unkn_byte = args_casted->unkn_byte;
    uint8_t guessed[KEY_SIZE];
    char key[KEY_SIZE];
    
    for(int j=1; j<KEY_SIZE; j++){
        guessed[j] = FALSE;
    }
    guessed[0] = TRUE;
    key[0] = unkn_byte;

    int one_guessed = TRUE;
    int total_guessed = 1;
    while(one_guessed && total_guessed<KEY_SIZE){
        one_guessed = FALSE;
        for(int j=0; j<KEY_SIZE; j++){
            if(guessed[j]==FALSE){
                Relation* curr = relations[j];
                while(curr!=NULL){
                    if(guessed[curr->in_relation_with]==TRUE){
                        key[j] = key[curr->in_relation_with] ^ curr->value;
                        guessed[j] = TRUE;
                        total_guessed++;
                        one_guessed = TRUE;
                        break;
                    }
                    curr = curr->next;
                }
            }
        }
    }
    pthread_mutex_lock(&mutex_printf); 
    for(int j=0; j<KEY_SIZE; j++){
        if(guessed[j]==TRUE){
            printf("%02x", key[j] & 0xff);
        }
        else{
            printf("XX");
        }
    }
    printf("\n");
    pthread_mutex_unlock(&mutex_printf);
    
    //free memory allocated
    free(args_casted);
    return NULL;
}


/*
 * Optimized but not working method to try to guess the key in a faster manner
 */
void guess_key_optimized(Relation** relations){
    int i=0;
    int guessed[KEY_SIZE];
    for(i=0; i<KEY_SIZE; i++){
        guessed[i]=FALSE;
    }
    Subkey_element* subkeys_list = NULL;
    Subkey_element* new_subkey;
    int initial_key[KEY_SIZE_INT];
    
    for(i=0; i<KEY_SIZE; i++){
        if(guessed[i]==FALSE){
            new_subkey = guess_subkey(i, relations, guessed);
            new_subkey->next = subkeys_list;
            subkeys_list = new_subkey;
        }
    }
    
    for(i=0; i<KEY_SIZE_INT; i++){
        initial_key[i] = 0;
    }
    combine_subkeys(subkeys_list, initial_key);
    
    //free memory allocated for subkeys
    Subkey_element* subkey_pointer_current = subkeys_list;
    Subkey_element* subkey_pointer_next = subkeys_list->next;
    while(subkey_pointer_next!=NULL){
        free(subkey_pointer_current);
        subkey_pointer_current = subkey_pointer_next;
        subkey_pointer_next = subkey_pointer_current->next;
    }
    free(subkey_pointer_current);
}

/*
 * Guess a subset of the key starting from the key bytes that are in relation
 */
Subkey_element* guess_subkey(int to_guess, Relation** relations, int* guessed){
    int i=0;
    uint8_t c=0;
    uint8_t xor_array[KEY_SIZE];
    uint8_t new_guessed[KEY_SIZE];
    for(i=0; i<KEY_SIZE; i++){
        new_guessed[i]=FALSE;
        xor_array[i] = 0x00;
    }
    new_guessed[to_guess] = TRUE;
    uint8_t random_value[KEY_SIZE];
    Subkey_element* new_subkeys = malloc(sizeof(Subkey_element));
    
    if(relations[to_guess]!=NULL){
        resolve_relations(to_guess, relations, new_guessed, xor_array);
    }
    for(c=0; c<BYTE_SPACE; c++){
        for(i=0; i<KEY_SIZE; i++){
            random_value[i]=c;
        }
        for(i=0; i<KEY_SIZE_INT; i++){
            ((int*) ((new_subkeys->subkeys)[c]))[i] = (((int*) random_value)[i] ^ ((int*) xor_array)[i]) & ((int*) new_guessed)[i];     //int optimization
        }
    }
    for(i=0; i<KEY_SIZE; i++){
        guessed[i] = guessed[i] | new_guessed[i];
    }
    return new_subkeys;
}

/*
 * Simplify the relation to use them faster when guessing the key
 */
void resolve_relations(int start, Relation** relations, uint8_t* new_guessed, uint8_t* xor_array){        
    new_guessed[start]=TRUE;
    Relation* pointer = relations[start];
    while(pointer!=NULL){
        if(new_guessed[pointer->in_relation_with]==FALSE){
            xor_array[pointer->in_relation_with] = xor_array[start] ^ (pointer->value);
            resolve_relations(pointer->in_relation_with, relations, new_guessed, xor_array);
        }            
        pointer = pointer->next;
    }
}

/*
 * Combine the subkeys in every possible combination to obtain all the possible final keys
 */
void combine_subkeys(Subkey_element* subkeys_list, int* partial_key){
    Subkey_element* list_pointer = subkeys_list;
    int** subkey_pointer = (int**) list_pointer->subkeys;
    char key[KEY_SIZE]; 
    int i=0, j=0;
    
    if(list_pointer->next == NULL){
        for(i=0; i<BYTE_SPACE; i++){
            for(j=0; j<KEY_SIZE_INT; j++){
                ((int*) key)[j] = partial_key[j] ^ subkey_pointer[i][j];
            }
            for(j=0; j<KEY_SIZE; j++){
                printf("%x", key[j]);
            }
            printf("\n");
        }
    }
    else{
        for(i=0; i<BYTE_SPACE; i++){
            for(j=0; j<KEY_SIZE_INT; j++){
                ((int*) key)[j] = partial_key[j] ^ subkey_pointer[i][j];
            }
            combine_subkeys(subkeys_list->next, (int*) key);
        }
    }
}