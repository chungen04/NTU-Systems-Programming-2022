#include "mylib.h"
#include <sys/mman.h>

extern movie_profile* movies[MAX_MOVIES];
extern unsigned int* num_of_keyword_matched_movies; // each req has 1 num_of_keyword_matched_movies
extern char*** keywords; // each req, each movie has one char* (title) 
extern double** scores; // each req, each movie has one score

extern unsigned int num_of_movies;
extern unsigned int num_of_reqs;

extern const unsigned int global_depth;
extern request* reqs[MAX_REQ];

extern pthread_mutex_t lock;

extern unsigned int num_of_matched_movies_pserver;

void* space;
char** movies_for_sorting;
double* scores_for_sorting;

double calculate_score(double* prof1, double* prof2){
    double dot_product = 0;
    for(int i=0; i<NUM_OF_GENRE; i++){
        dot_product += prof1[i]*prof2[i];
    }
    double norm_1 = 0;
    double norm_2 = 0;
    for(int i=0; i<NUM_OF_GENRE; i++){
        norm_1 += prof1[i]*prof1[i];
        norm_2 += prof2[i]*prof2[i];
    }
    norm_1 = sqrt(norm_1);
    norm_2 = sqrt(norm_2);
    if(norm_1*norm_2<__DBL_EPSILON__){
        return 0;
    }else{
        return dot_product/(norm_1*norm_2);
    }
}

void merge(int start, int end, int req_id){
    //split point is: 
    int split_point = (end-start)/2 + start + (end-start)%2;
    // ex start = 4, end = 7,
    // split = 6, first local is 4 5 th element, second is 6 th element
    int l_start = start;
    int r_start = split_point;
    int l_end = split_point;
    int r_end = end;
    if(end == start) return;
    char** keywords_temp = calloc(end-start, sizeof(char*));
    double* score_temp = calloc(end-start, sizeof(double));
    int cursor = 0;
    while(l_start != l_end || r_start != r_end){
        if(l_start == l_end){
            keywords_temp[cursor] = keywords[req_id][r_start];
            score_temp[cursor] = scores[req_id][r_start];
            r_start++;
            cursor++;
            continue;
        }
        if(r_start == r_end){
            keywords_temp[cursor] = keywords[req_id][l_start];
            score_temp[cursor] = scores[req_id][l_start];
            l_start++;
            cursor++;
            continue;
        }
        if(scores[req_id][l_start] == scores[req_id][r_start]){
            // consider the values are same
            if(strcmp(keywords[req_id][l_start], keywords[req_id][r_start])>0){
                keywords_temp[cursor] = keywords[req_id][r_start];
                score_temp[cursor] = scores[req_id][r_start];
                r_start++;
                cursor++;
            }else{
                keywords_temp[cursor] = keywords[req_id][l_start];
                score_temp[cursor] = scores[req_id][l_start];
                l_start++;
                cursor++;
            }
            continue;
        }
        if(scores[req_id][l_start]>scores[req_id][r_start]){
            keywords_temp[cursor] = keywords[req_id][l_start];
            score_temp[cursor] = scores[req_id][l_start];
            l_start++;
            cursor++;
        }else if(scores[req_id][l_start]<scores[req_id][r_start]){
            keywords_temp[cursor] = keywords[req_id][r_start];
            score_temp[cursor] = scores[req_id][r_start];
            r_start++;
            cursor++;
        }
    }
    //copy back
    for(int i=start; i<end; i++){
        keywords[req_id][i] = keywords_temp[i-start];
        scores[req_id][i] = score_temp[i-start];
    }
    free(keywords_temp);
    free(score_temp);
    return;
}

void* sort_caller(void* arg_sort_fn){
    char** keywords = ((args_sort*)arg_sort_fn)->keywords;
    double* scores = ((args_sort*)arg_sort_fn)->scores;
    int size = ((args_sort*)arg_sort_fn)->size;
    sort(keywords, scores, size);
    return NULL;
}

void* sort_caller_dummy(void* arg_sort_fn){
    char** keywords = ((args_sort*)arg_sort_fn)->keywords;
    double* scores = ((args_sort*)arg_sort_fn)->scores;
    int size = ((args_sort*)arg_sort_fn)->size;
    return NULL;
}

void* thr_fn(void* arg_thr_fn){
    int start = ((thr_fn_args*)arg_thr_fn)->start;
    int end = ((thr_fn_args*)arg_thr_fn)->end;
    int depth = ((thr_fn_args*)arg_thr_fn)->depth;
    int req_id = ((thr_fn_args*)arg_thr_fn)->req_id;
    if(depth == 0){
		for(int j=0; j< num_of_movies; j++){
			if(strstr(movies[j]->title, reqs[req_id]->keywords) || !strcmp(reqs[req_id]->keywords, "*")){
				keywords[req_id][num_of_keyword_matched_movies[req_id]] = movies[j]->title;
                scores[req_id][num_of_keyword_matched_movies[req_id]] = calculate_score(movies[j]->profile, reqs[req_id]->profile);
                num_of_keyword_matched_movies[req_id]++;
            }
		}
		// now keyword_match_movies saves the movies with matched keywords
		// next create movies array and corresponding score array
		//now start sorting
        end = num_of_keyword_matched_movies[req_id];
    }
    pthread_t tid1;
    bool tid1_created = false;
    
    if(depth < global_depth-1){
        thr_fn_args arg1;
        arg1.start = start;
        arg1.end = (end-start)/2 + start + (end-start)%2;
        arg1.depth = depth+1;
        arg1.req_id = req_id;
        pthread_create(&tid1, NULL, thr_fn, &arg1);
        thr_fn_args arg2;
        arg2.end = end;
        arg2.start = (end-start)/2 + start + (end-start)%2;
        arg2.depth = depth+1;
        arg2.req_id = req_id;
        thr_fn(&arg2);
        tid1_created = true;
    }else{
        // create threads for sorting
        int split_point = (end-start)/2 + start + (end-start)%2;
        args_sort args1;
        args_sort args2;
        if(start != split_point){
            sort(&keywords[req_id][start], &scores[req_id][start], split_point-start);
        }
        if(split_point != end){
            sort(&keywords[req_id][split_point], &scores[req_id][split_point], end-split_point); 
        } 
        tid1_created = false;
    }
    // wait
    if(tid1_created){
        pthread_join(tid1, NULL);
    }
    // merge
    merge(start, end, req_id);
    if(depth == 0){
        savefile_t(req_id);
    }
    return NULL;
}

void savefile_t(int i){
    char buf[30];
    sprintf(buf, "%dt.out", reqs[i]->id);
    FILE* fp = fopen(buf, "w");
    for(int j=0; j<num_of_keyword_matched_movies[i]; j++){
        fprintf(fp, "%s\n", keywords[i][j]);
    }
    fclose(fp);
    return;
}

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_SHARED | MAP_ANONYMOUS;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  return mmap(NULL, size, protection, visibility, -1, 0);
}

void shmem_sort_handler(int start, int end, int depth){
    //assert req_id = 0 only; only 1 request
    if(depth == 0){
        for(int i=0; i< num_of_movies; i++){
			if(strstr(movies[i]->title, reqs[0]->keywords) || !strcmp(reqs[0]->keywords, "*")){
				keywords[0][num_of_matched_movies_pserver] = movies[i]->title;
                scores[0][num_of_matched_movies_pserver] = calculate_score(movies[i]->profile, reqs[0]->profile);
                num_of_matched_movies_pserver++;
            }
		} // now create mmap space

        space = create_shared_memory(SHMEM_SIZE);

        // memory layout is: (from low addr)
        // double double ... (*num of matched movies)
        // char [MAX_LEN] char[MAX_LEN] char[MAX_LEN]... (*num of matched movies)
        
        // place in the data first
        memcpy(space, scores[0], num_of_matched_movies_pserver*sizeof(double));
        //copy the strings in

        // usage to use:
        // *(double*)(SHMEM_DBL_BASE(3))
        // (char*)(SHMEM_STR_BASE(3)
        
        for(int i=0; i<num_of_matched_movies_pserver; i++){
            memcpy(SHMEM_STR_BASE(i), keywords[0][i], sizeof(char)*MAX_LEN);
        }
        
        end = num_of_matched_movies_pserver;
        //init the movies_for_sorting and scores_for_sorting variables

        movies_for_sorting = calloc(num_of_matched_movies_pserver, sizeof(char*));
    }
    for(int i=0; i<num_of_matched_movies_pserver; i++){
        movies_for_sorting[i] = SHMEM_STR_BASE(i);
    }
    pid_t pid1;
    pid_t pid2;
    if(depth == global_depth){
        sort(&movies_for_sorting[start],SHMEM_DBL_BASE(start),end-start);
        //copy the strings back to shared memory!
        //now from movies_for_sorting start~end, the pointers point to the new strings.
        for(int i=start; i<end; i++){
            memcpy(SHMEM_STR_BASE(i), movies_for_sorting[i], MAX_LEN*sizeof(char));
        }
        exit(0);
    }else{
        if((pid1 = fork()) == 0){
            // call first child
            int start_arg = start;
            int end_arg = (end-start)/2 + start + (end-start)%2;
            int depth_arg = depth+1;
            if(end_arg == start_arg) exit(0);
            shmem_sort_handler(start_arg, end_arg, depth_arg);
            exit(0);
        }
        if((pid2 = fork()) == 0){
            // call second child
            int start_arg = (end-start)/2 + start + (end-start)%2;
            int end_arg = end;
            int depth_arg = depth+1;
            if(end_arg == start_arg) exit(0);
            shmem_sort_handler(start_arg, end_arg, depth_arg);
            exit(0);
        }

        wait(&pid1);
        wait(&pid2);
        //do merge
        merge_pserver(start, end);
        if(depth != 0){
            exit(0);
        }
    }
    // depth = 0
    savefile_p();
    return;
}

void dump_shared_memory(){
    FILE* fp = fopen("log.txt", "w");
    for(int i=0; i<num_of_matched_movies_pserver; i++){
        fprintf(fp, "%s\n", (char*)(SHMEM_STR_BASE(i)));
    }
    fclose(fp);
}

void savefile_p(){
    char buf[30];
    sprintf(buf, "%dp.out", reqs[0]->id);
    FILE* fp = fopen(buf, "w");
    for(int i=0; i<num_of_matched_movies_pserver; i++){
        fprintf(fp, "%s\n", (char*)(SHMEM_STR_BASE(i)));
    }
    fclose(fp);
}

void merge_pserver(int start, int end){
    int split_point = (end-start)/2 + start + (end-start)%2;
    int l_start = start;
    int r_start = split_point;
    int l_end = split_point;
    int r_end = end;
    if(end == start) return;
    char** keywords_temp = calloc(end-start, sizeof(char*));
    for(int i=0; i<end-start; i++){
        keywords_temp[i] = calloc(MAX_LEN, sizeof(char));
    }
    double* score_temp = calloc(end-start, sizeof(double));
    int cursor = 0;
    while(l_start != l_end || r_start != r_end){
        if(l_start == l_end){
            memcpy(keywords_temp[cursor], SHMEM_STR_BASE(r_start), MAX_LEN*sizeof(char));
            memcpy(&score_temp[cursor], SHMEM_DBL_BASE(r_start), sizeof(double));
            r_start++;
            cursor++;
            continue;
        }
        if(r_start == r_end){
            memcpy(keywords_temp[cursor], SHMEM_STR_BASE(l_start), MAX_LEN*sizeof(char));
            memcpy(&score_temp[cursor], SHMEM_DBL_BASE(l_start), sizeof(double));
            l_start++;
            cursor++;
            continue;
        }
        if(*(double*)(SHMEM_DBL_BASE(l_start)) == *(double*)(SHMEM_DBL_BASE(r_start))){
            // consider the values are same
            if(strcmp(SHMEM_STR_BASE(l_start), SHMEM_STR_BASE(r_start))>0){
                memcpy(keywords_temp[cursor], SHMEM_STR_BASE(r_start), MAX_LEN*sizeof(char));
                memcpy(&score_temp[cursor], SHMEM_DBL_BASE(r_start), sizeof(double));
                r_start++;
                cursor++;
            }else{
                memcpy(keywords_temp[cursor], SHMEM_STR_BASE(l_start), MAX_LEN*sizeof(char));
                memcpy(&score_temp[cursor], SHMEM_DBL_BASE(l_start), sizeof(double));
                l_start++;
                cursor++;
            }
            continue;
        }
        if(*(double*)(SHMEM_DBL_BASE(l_start))>*(double*)(SHMEM_DBL_BASE(r_start))){
            memcpy(keywords_temp[cursor], SHMEM_STR_BASE(l_start), MAX_LEN*sizeof(char));
            memcpy(&score_temp[cursor], SHMEM_DBL_BASE(l_start), sizeof(double));
            l_start++;
            cursor++;
        }else if(*(double*)(SHMEM_DBL_BASE(l_start))<*(double*)(SHMEM_DBL_BASE(r_start))){
            memcpy(keywords_temp[cursor], SHMEM_STR_BASE(r_start), MAX_LEN*sizeof(char));
            memcpy(&score_temp[cursor], SHMEM_DBL_BASE(r_start), sizeof(double));
            r_start++;
            cursor++;
        }
    }
    //copy back
    for(int i=start; i<end; i++){
        memcpy(SHMEM_STR_BASE(i), keywords_temp[i-start], MAX_LEN*sizeof(char));
        memcpy(SHMEM_DBL_BASE(i), &score_temp[i-start], sizeof(double));
    }
    free(keywords_temp);
    free(score_temp);
    return;
}