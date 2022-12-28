#ifndef _MYLIB_H_
#define _MYLIB_H_

#include <stdbool.h>
#include <stdlib.h>
#include "header.h"
#include <math.h>

#define SHMEM_STR_BASE(i) space+num_of_matched_movies_pserver*sizeof(double)+i*sizeof(char)*MAX_LEN
#define SHMEM_DBL_BASE(i) space+i*sizeof(double)
#define SHMEM_SIZE sizeof(double)*num_of_matched_movies_pserver+sizeof(char)*num_of_matched_movies_pserver*MAX_LEN

typedef struct args_sort{
    char** keywords;
    double* scores;
    int size;
} args_sort;

typedef struct thr_fn_args{
    int start;
    int end;
    int depth;
    int req_id;
} thr_fn_args;

double calculate_score(double* prof1, double* prof2);
void* thr_fn(void* thr_fn_args);
void merge(int start, int end, int req_id);
void savefile_t();
movie_profile* deep_copy(movie_profile* movie);
void* create_shared_memory(size_t size);
void shmem_sort_handler(int start, int end, int depth);
void dump_shared_memory();
void merge_pserver(int start, int end);
void savefile_p();

#endif