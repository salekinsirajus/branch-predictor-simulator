#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/

/* Struct containing the outcome
 *
 * */
struct branch {
    int counter;
};

unsigned long int get_index(unsigned long int hex, int m){
    //unsigned long int x = (hex << 2);
    //return x & 127;
   return (hex >> 2) & ((1 << m+1) -1);
   // return (((1 << stop) - 1) & (hex >> (stop - 1)));
}

    // Initializes a data structure that holds the predictions for each table
    //get low 6 (for this time) bits used for branch prediction
    //for now, we will just ignore the collissions

int demo(unsigned long int hex){

    unsigned long int idx = get_index(hex, 6);
    printf("%lx \n", hex);           // Print and test if file is read correctly
    printf("First 6 bits of this number: %lx", idx );
    printf("Index after dividing by 100: %lx", idx % 100);
    return 0;
}

int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file
    
    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name  = argv[1];
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }

    //Initialize data structure
    branch predtab[64];
    for (int i=0; i < 64; i ++){
        predtab[i].counter = 2;
    }
    
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    int hit=0;
    int miss=0;
    int count=0;;

    char str[2];
    int idx, pred;
    char prediction;
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {
        
        outcome = str[0];
        printf("=%d %lx  %c\n", count, addr, outcome);
        count++;
        idx = get_index(addr, 6) % 64;
        pred = predtab[idx].counter;
        printf("BP:  %d  %d\n", idx, pred);

        if (pred >= 2) {
            prediction = 't';
        }
        else {prediction = 'n';}
        
        if (outcome == 't'){
            if (predtab[idx].counter < 3) {
                predtab[idx].counter++;
            } 
            else {
                predtab[idx].counter = 3;
            }
        }
        else if (outcome == 'n'){
            if (predtab[idx].counter > 0) {
                predtab[idx].counter--;
            } 
            else {
                predtab[idx].counter = 0;
            }
        }


        pred = predtab[idx].counter;
        printf("BU:  %d  %d\n", idx, pred);

        if (outcome == prediction){
            hit++;
        } else {
            miss++;
        }
        /*************************************
            Add branch predictor code here
        **************************************/
    }

printf("number of predictions: %d\n", count);
printf("number of mispredictions: %d\n", miss);
printf("misprediction rate:      %f\n", (100.00 * miss/count));

    // printing the content
    for (int i=0; i < 64; i++){
        printf("%d %d\n", i, predtab[i].counter);
    }

    return 0;
}
