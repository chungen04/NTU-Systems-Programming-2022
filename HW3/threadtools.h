#ifndef THREADTOOL
#define THREADTOOL
#include <setjmp.h>
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define THREAD_MAX 16  // maximum number of threads created
#define BUF_SIZE 512

struct tcb {
    //storage of per-thread metadata
    int id;  // the thread id
    jmp_buf environment;  // where the scheduler should jump to
    int arg;  // argument to the function
    int fd;  // file descriptor for the thread
    char buf[BUF_SIZE];  // buffer for the thread
    char* own_pipe; // owned pipe name
    int i, x, y, result, now;  // declare the variables you wish to keep between switches
    int num, sz;
};

extern int timeslice;
extern jmp_buf sched_buf;
extern struct tcb *ready_queue[THREAD_MAX], *waiting_queue[THREAD_MAX];

/*
 * rq_size: size of the ready queue
 * rq_current: current thread in the ready queue
 * wq_size: size of the waiting queue
 */
extern int rq_size, rq_current, wq_size;
/*
* base_mask: blocks both SIGTSTP and SIGALRM
* tstp_mask: blocks only SIGTSTP
* alrm_mask: blocks only SIGALRM
*/
extern sigset_t base_mask, tstp_mask, alrm_mask;
/*
 * Use this to access the running thread.
 */
#define RUNNING (ready_queue[rq_current])

/*
    thread life cycle: 
    created by calling thread_create in main
    call thread setup to initialize
    when small portion calc. is done, call thread_yield to check context switch
    call async_read when need to read from file
    call thread_exit to clean up when done  
*/
void sighandler(int signo);
void scheduler();

#define thread_create(func, id, arg) {\
    func(id, arg);\
}

// initialize the tcb and append to ready queue, create and open a named pipe
// call setjmp for longjmp (jump to scheduler.)
//open named pipe in nonblocking mode

#define thread_setup(id, arg) {\
    struct tcb* new_tcb = calloc(1, sizeof(struct tcb));\
    new_tcb->id = id;\
    new_tcb->arg = arg;\
    char* buf = calloc(20, sizeof(char));\
    sprintf(buf, "%d_%s", id, __func__);\
    mkfifo(buf, 0700);\
    new_tcb->fd = open(buf, O_NONBLOCK, O_RDONLY);\
    new_tcb->own_pipe = buf;\
    /*fprintf(stderr, "created new tcb: %d %s rq:%d\n", new_tcb->id,new_tcb->own_pipe, rq_size);*/\
    ready_queue[rq_size] = new_tcb;\
    rq_size++;\
    new_tcb->i = 0;\
    new_tcb->x = 0;\
    new_tcb->y = 0;\
    new_tcb->result = 0;\
    new_tcb->now = 0;\
    new_tcb->sz = 0;\
    if(setjmp(new_tcb->environment) == 0){\
        return;\
    }\
}

// remove name pipe owned by the thread_exit
// jump back to scheduler by longjmp(sched_buf, 3)
#define thread_exit() {\
    remove(RUNNING->own_pipe);\
    free(RUNNING->own_pipe);\
    longjmp(sched_buf, 3);\
}

// a thread should use thread_yield to check if a context switch is needed.
// should save execution content(??? not implemented), unblock SIGTSTP, SIGALRM
// if any is pending, sighandler takes over and run scheduler (which trigger context switch and longjmp to scheduler)
// if no signal pending, block the signals again and continue execute current thread
#define thread_yield() {\
    if(setjmp(RUNNING->environment) == 0){\
        /*save running states... for maximum subarray*/\
        sigprocmask(SIG_SETMASK, &alrm_mask, NULL);\
        sigprocmask(SIG_SETMASK, &tstp_mask, NULL);\
    }\
    sigprocmask(SIG_SETMASK, &base_mask, NULL);\
}

// save the execution content
// jump to scheduler via longjmp(sched_buf, 2)
// after thread restart, call read to read from named pipe
#define async_read(count) ({\
    /*check if pipe has data. if there is, no need to jump*/\
    if(setjmp(RUNNING->environment) == 0){\
        longjmp(sched_buf, 2);\
    }\
    read(RUNNING->fd, RUNNING->buf, count);\
    RUNNING->num = atoi(RUNNING->buf);\
})

#endif // THREADTOOL
