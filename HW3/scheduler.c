#include "threadtools.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/ioctl.h>

/*
 * Print out the signal you received.
 * If SIGALRM is received, reset the alarm here.
 * This function should not return. Instead, call longjmp(sched_buf, 1).
 */
// sighandler should only be triggered via thread_yield.
void sighandler(int signo) {
    // TODO
    if(signo == SIGTSTP){
        fprintf(stdout, "caught SIGTSTP\n");
    }else if(signo == SIGALRM){
        alarm(timeslice);
        fprintf(stdout, "caught SIGALRM\n");
    }
    longjmp(sched_buf, 1);
}

void put_to_ready_queue(int pos){
    struct tcb* block = waiting_queue[pos];
    for(int i = pos; i<wq_size-1; i++){
        waiting_queue[i] = waiting_queue[i+1];
    }
    wq_size--;
    ready_queue[rq_size] = block;
    rq_size++;
}

void move_to_waiting_queue(){
    struct tcb* block = RUNNING;
    if(rq_current != rq_size-1){
        RUNNING = ready_queue[rq_size-1];
    }else{
        rq_current = 0;
    }
    rq_size--;
    waiting_queue[wq_size] = block;
    wq_size++;
}

void remove_from_ready_queue(){
    if(rq_current != rq_size-1){
        RUNNING = ready_queue[rq_size-1];
    }else{
        rq_current = 0;
    }
    rq_size--;
}

void wait_until_ready(){
    while(1){
        for(int i=0; i<wq_size; i++){
            ioctl(waiting_queue[i]->fd, FIONREAD, &waiting_queue[i]->sz);
            if(waiting_queue[i]->sz>0){
                put_to_ready_queue(i);
                return;
            }
        }
    }
}

/*
 * Prior to calling this function, both SIGTSTP and SIGALRM should be blocked.
 */
void scheduler() {
    // TODO
    /*
    CASE 1. called by main:
    execute the first created thread
    */

    int stat;
    if((stat = setjmp(sched_buf)) == 0){
        rq_current = 0;
        longjmp(RUNNING->environment, 1);
    }
    while(1){
        // always put maximumsubarray instance in waiting queue only
        for(int i=0; i<wq_size; i++){
            ioctl(waiting_queue[i]->fd, FIONREAD, &waiting_queue[i]->sz);
            if(waiting_queue[i]->sz > 0){\
                put_to_ready_queue(i);
                if(i != wq_size-1){
                    i--;
                }// next iter start with index i
            }
        } 
        if(stat == 1){
            // sighandler triggered by thread_yield
            rq_current++;
            if(rq_current>=rq_size){
                rq_current = 0;
            }
            if((stat = setjmp(sched_buf)) == 0){
                longjmp(RUNNING->environment, 1);
            }
            continue;
        }
        if(stat == 3){
            // thread_exit
            // thread filled is the current first thread
            remove_from_ready_queue();
            if(rq_size > 0){
                if((stat = setjmp(sched_buf)) == 0){
                    longjmp(RUNNING->environment, 1);
                }
            }else{
                if(wq_size == 0){
                    return;
                }else{
                    wait_until_ready();
                    if((stat = setjmp(sched_buf)) == 0){
                        longjmp(ready_queue[0]->environment, 1);
                        rq_current = 0;
                    }
                }
            }
            continue;
        }
        if(stat == 2){
            // async_read
            move_to_waiting_queue();
            if(rq_size > 0){
                if((stat = setjmp(sched_buf)) == 0){
                    longjmp(RUNNING->environment, 1);
                }
            }else{
                if(wq_size == 0){
                    return;
                }else{
                    wait_until_ready();
                    if((stat = setjmp(sched_buf)) == 0){
                        longjmp(ready_queue[0]->environment, 1);
                        rq_current = 0;
                    }
                }
            }
            continue;
        }
        
    }
    /*
    other cases:
    1. longjmp(sched_buf, 1) from sighandler triggered by thread_yield
    2. longjmp(sched_buf, 2) from async_read
    3. longjmp(sched_buf, 3) from thread_exit
     once triggered, iterates through the waiting queue and
     see any data is ready. then bring all avaiable thread to the ready queue (by relative order)
     holes in waiting queue should be filled in original relative order
     
     to leave the queue: 1. thread has finished executing (thread_exit); 
     thread wants to read from a file (move to waiting queue(async_read))

     if thread is removed -> take thread from end of ready queue to fill

     switch to next thread: 
     thread_yield -> execute next thread in queue
     async_read and thread_exit -> execute the thread that filled up the hole
     if the thread is the last thread in queue -> execute first thread
    
     ready queue is empty:
     1. waiting queue is empty: waut until one is ready.
     2. waiting queue is also empty: return to main.
    */
    
}

// running thread is not always the zeroth thread, but if a thread is kicked out, it is replaced by the last thread. (so it will cluster) 