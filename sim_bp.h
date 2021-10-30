#ifndef SIM_BP_H
#define SIM_BP_H

typedef struct bp_params{
    unsigned long int K;
    unsigned long int M1;
    unsigned long int M2;
    unsigned long int N;
    char*             bp_name;
}bp_params;

// Put additional data structures here as per your requirement
unsigned long int left_x_bits(unsigned long int number, int x);
unsigned long int get_gshare_idx(unsigned long int shift_register, unsigned long int pc, int m, int n);
unsigned long int get_index(unsigned long int hex, int m);
unsigned long int update_bhr(unsigned long int bhr, char outcome);

#endif
