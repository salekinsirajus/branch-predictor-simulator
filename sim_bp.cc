#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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


void print_contents(branch table[], int SIZE, char* name){
    printf("FINAL %s CONTENTS\n", name);
    for (int i=0; i < SIZE; i++){
        printf("%d %d\n", i, table[i].counter);
    }
}

unsigned long int left_x_bits(unsigned long int number, int x){
    return number & (((1<<x)-1) << x);

}

unsigned long int get_gshare_idx(unsigned long int shift_register, unsigned long int pc, int m, int n){

    unsigned long int m_bits_of_pc = get_index(pc, m); 
    //n bits of shift register (from msb) (from left)
    //the following is unused as it seems
    //unsigned long int shift_register_valid_bits = (left_x_bits(shift_register, n)) << (m-n); 

    return m_bits_of_pc ^ (shift_register << (m-n));  
}

unsigned long int get_index(unsigned long int hex, int m){
   //The following version was working for Bimodal
   //return (hex >> 2) & ((1 << m+1) -1);
   return (hex >> 2) & ((1 << (m+1)) -1);
}

int get_new_prediction_from_outcome(int old_pred, char actual){
    /* 0 and 3 are the saturation points */
    /* Update the table based on the prediction */
    if (actual == 't'){
        if (old_pred < 3) {
            return old_pred + 1;
        } 
        else {
            return 3;
        }
    }
    else if (actual == 'n'){
        if (old_pred > 0) {
            return old_pred - 1;
        } 
        else {
            return 0 ;
        }
    }

}

int get_new_pred_chooser(int old_pred, int gshare_pred, int bimodal_pred, int actual){
    if ((gshare_pred == actual) && (bimodal_pred != actual)){
        if (old_pred < 3) {
            return old_pred + 1;
        } 
        else {
            return 3;
        }
    } 
    else if ((gshare_pred != actual) && (bimodal_pred == actual)){
        if (old_pred > 0) {
            return old_pred - 1;
        } 
        else {
            return 0 ;
        }
    }

    else {
        return old_pred;
    }
}

char itoc_pred(int pred_int){
    /* Turns the prediction from int to a char. Example: '0'->'n', or '2'->'t' */
    if (pred_int >= 2) {
        return 't';
    }

    return 'n';
}


unsigned long int update_bhr(unsigned long int bhr, char outcome, int n){
    //Update shift register after updating the counter. (We also probably need a
    //2-D array of counters for each branch
    //
    //Right shift
    //Deposit in the most significant bit
    bhr =  bhr >> 1;
    //n=32;

    if (outcome == 't'){
        bhr |= (1 << (n - 1));
        bhr &= ((1 << n) - 1);
        //
        //setting the MSB(to 1).
        // #FIXME: is this gonna work on all the machines?
        // #FIXME: what's the max value of M gonna be?
        //bhr |= (1 << 32); 
        //bhr |= (1UL << 32);
    } 
    return bhr;
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
    int g_pred, h_pred, b_pred, g_idx, h_idx, b_idx;
    char prediction;
    int m = params.M1;
    int n = params.N; 
    int k = params.K;

    unsigned long int bhr = 0; // shift register

    //TODO: dynamically allocate these arrays
    int CTABSIZE = pow(2, k);
    branch choosertab[CTABSIZE];
    for (int i=0; i <CTABSIZE; i++){choosertab[i].counter=1;}

    int BIMODALTABSIZE = 32;
    branch bimodaltab[BIMODALTABSIZE];
    for (int i=0; i <BIMODALTABSIZE ; i ++){bimodaltab[i].counter = 2;}

    int GSHARETABSIZE = 1024;
    branch gsharetab[GSHARETABSIZE];
    for (int i=0; i <GSHARETABSIZE; i ++){gsharetab[i].counter = 2;}


    while(fscanf(FP, "%lx %s", &addr, str) != EOF) {
        
        outcome = str[0];

        /*
        printf("=%d %lx  %c\n", count, addr, outcome);
        */
        count++;

        /*=============== Determine Index ============= */
        b_idx = get_index(addr, m) % BIMODALTABSIZE;
        g_idx = get_gshare_idx(bhr,addr,m,n) % GSHARETABSIZE;
        //TODO: reexamine this
        h_idx = get_index(addr, k) % CTABSIZE; 

        /*=============== Make Prediction ============= */
        b_pred = bimodaltab[b_idx].counter; 
        g_pred = gsharetab[g_idx].counter;
        h_pred = choosertab[h_idx].counter;

        if (h_pred >= 2){
            /*============== Update Table ================= */
            gsharetab[g_idx].counter = get_new_prediction_from_outcome(gsharetab[g_idx].counter, outcome);
            /* Use this as the final prediction */
            prediction = itoc_pred(g_pred);

        } else {
            /*============== Update Table ================= */
            bimodaltab[b_idx].counter = get_new_prediction_from_outcome(bimodaltab[b_idx].counter, outcome);
            /* Use this as the final prediction */
            prediction = itoc_pred(b_pred);

        } 
        /*Update chooser table */
        //and 'n'
        choosertab[h_idx].counter = get_new_pred_chooser(choosertab[h_idx].counter, itoc_pred(g_pred), itoc_pred(b_pred), outcome);

        /*
        printf("GP:  %d  %d\n", idx, pred);
        */

        /*
        pred = predtab[idx].counter;
        printf("GU:  %d  %d\n", idx, pred);
        */
        
        /*========== Update Global History ============ */
        bhr = update_bhr(bhr, outcome, n);

        /*============== Update Stats ================= */
        if (outcome == prediction){
            hit++;
        } else {
            miss++;
        }
    }

    // print overall stats
    printf("number of predictions: %d\n", count);
    printf("number of mispredictions: %d\n", miss);
    printf("misprediction rate:      %f\n", (100.00 * miss/count));

    // printing the content
    print_contents(choosertab, CTABSIZE, "CHOOSER");
    print_contents(gsharetab, GSHARETABSIZE, "GSHARE");
    print_contents(bimodaltab, BIMODALTABSIZE, "BIMODAL");

    return 0;
}
