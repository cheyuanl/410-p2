#include <thread.h>
#include <stdlib.h>
#include <simics.h>

#ifndef NULL
#define NULL 0
#endif

/* --- Define private field for thread lib --- */

/** @brief The next tid to be issued if new thread created */
static int global_tid = 0;

/** @brief The size of each thread stack */
static int stk_size;

/** @brief The structure of the head block of thread stack */
typedef struct{
    int zero;      /* the spectial value indicates the begining of stack */
    int tid;        /* the pid of this thread */
    void *args;    /* args for the entry function */
    void *ret_addr; /* the return addr for entry function */
} thr_headptr_t;

int thread_fork();

void* get_ebp();

void thread_entry(void* ebp, void* esp, void* *(*func)(void*)) {
}

/** @brief The doubly linked-list to keep tracking of all live threads */
typedef struct {
    void *thr_headptr;
    void *prev;
    void *next;
} thr_node_t;


/* --- Function definition --- */

void thr_destroy(){

    return;
}

int thr_init( unsigned int size ) {
    /* TODO: Harshly examiane the value */
    stk_size = size;
    return 0;
}

int thr_create( void *(*func)(void *), void *args ) {
    /* find a address to request a memory block */
    /* TODO: currently the thread stack lives on heap,
     *       since I don't konw how to find a chunck of
     *       memory on stack...
     */
    thr_headptr_t *thr_headptr = malloc(sizeof(thr_node_t) + stk_size);
    if (thr_headptr == NULL)
        return -1;

    /* setup the header block of thread stack */
    /* note: becareful about esp should be aligned */
    thr_headptr->zero = (int) NULL;
    thr_headptr->tid = global_tid++;
    thr_headptr->args = args;
    thr_headptr->ret_addr = thr_destroy; /* TODO: where? */


    /* a thread newly created by thread fork has no software exception
     * handler registered  */

    int ret = thread_fork();
    /* error. no thread created */;
    if (ret == -1) {
        free(thr_headptr);
        return -1;
    }
    /* child goes here */
    else if (ret == 0){
        thread_entry(&thr_headptr->zero, &thr_headptr->ret_addr, (void*)func);
    }

    return 0;
}

int gettid() {
    int* curr_ebp = get_ebp();
    while(*curr_ebp != (int)NULL) {
        curr_ebp = *(int**)(curr_ebp);
    }

    return ((thr_headptr_t*)curr_ebp)->tid;
}
