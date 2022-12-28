#include "threadtools.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

// SIGTSTP and SIGALRM should be blocked when executing
void fibonacci(int id, int arg) {
    thread_setup(id, arg);

    for (RUNNING->i = 1; ; RUNNING->i++) {
        // fprintf(stderr, "fib: %d", RUNNING->i);
        if (RUNNING->i <= 2)
            RUNNING->x = RUNNING->y = 1;
        else {
            /* We don't need to save tmp, so it's safe to declare it here. */
            int tmp = RUNNING->y;  
            RUNNING->y = RUNNING->x + RUNNING->y;
            RUNNING->x = tmp;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);

        if (RUNNING->i == RUNNING->arg) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}

void collatz(int id, int arg) {
    // TODO
    thread_setup(id, arg);
    RUNNING->i = RUNNING->arg;
    while (RUNNING->i != 1){
        if(RUNNING->i%2 == 0){
            RUNNING->i = RUNNING->i / 2;
        }else{
            RUNNING->i = RUNNING->i*3+1;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->i);
        sleep(1);
        if (RUNNING->i == 1) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}
void max_subarray(int id, int arg) {
    // TODO
    thread_setup(id, arg);
    RUNNING->i = RUNNING->arg;
    for(; RUNNING->i > 0; RUNNING->i--) {
        // read in number
        async_read(5);
        if(RUNNING->i == RUNNING->arg){
            RUNNING->result = RUNNING->num>0? RUNNING->num:0;
            RUNNING->now = RUNNING->num>0? RUNNING->num:0;  
        }else{
            if(RUNNING->now > 0){
                RUNNING->now += RUNNING->num;
            }else{
                RUNNING->now = RUNNING->num>0? RUNNING->num:0;
            }
            if(RUNNING->now > RUNNING->result){
                RUNNING->result = RUNNING->now;
            }
        }
        printf("%d %d\n", RUNNING->id, RUNNING->result);
        sleep(1);
        if (RUNNING->i == 1) {
            thread_exit();
        }
        else {
            thread_yield();
        }
    }
}
