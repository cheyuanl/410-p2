/** @file thr_internals.h
 *
 *  @brief This file contains Macros, global variables and data structures
 *  that are internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

#include <cond_type.h>
#include <mutex_type.h>

/** @brief Round down the bits to page size */
#define PAGE_ALIGN_MASK ((unsigned int)~((unsigned int)(PAGE_SIZE - 1)))
/** @brief Round up the bits to page size */
#define PAGE_ROUNDUP(p) \
    (void *)(PAGE_ALIGN_MASK & (((unsigned int)(p)) + PAGE_SIZE - 1))
/** @brief Round down an address to page size and cast it to pointer type */
#define PAGE_ROUNDDN(p) (void *)(PAGE_ALIGN_MASK & ((unsigned int)(p)))

/** @brief The bounds of main thread's stack */
void *main_stk_lo;
void *main_stk_hi;

/** @brief The head of thread stack */
void *thr_stk_head;

/** @brief The current thread stack address to be allocated */
void *thr_stk_curr;

/** @brief Used for malloc lock */
mutex_t malloc_mp;

/** @brief The state of thread */
typedef enum thr_state {
    /* still installing the handler */
    THR_UNRUNNABLE,
    /* every thing is setup, good to go */
    THR_RUNNABLE,
    /* descheduled */
    THR_SLEEPING,
    /* not running and vanished */
    THR_EXITED

} thr_state_t;

/** @brief The structure of the head block of thread stack */
typedef struct thr_stk_t thr_stk_t;
struct thr_stk_t {
    void *ret_addr;     /* the return addr for wrapper function,
                           this should be invalid since it won't be called */
    void *func;         /* child thread's wrapper function */
    void *args;         /* args for the child function */
    thr_stk_t *cv_next; /* pointer to next thread in the cv chain */
    thr_stk_t *next;    /* pointer to next thread in thread list */
    thr_stk_t *prev;    /* pointer to prev thread in thread list */
    int utid;           /* thread id from user's perspective */
    int ktid;           /* thread id from kernel's perspective */
    thr_state_t state;  /* state of this thread */
    void *exit_status;  /* exit status place holder when thr_exit called */
    mutex_t mp;         /* mutex for this structure */
    cond_t cv;          /* conditional variable for this structure */
    int join_flag;      /* indicate if this thread is called by thr_join */
    int zero;           /* the value indicates the ebp of begin of stack */
};

/** @brief xchg instruction wrapper */
int xadd_wrapper(int *ticket);

/** @brief Get the pointer to this thread stack structure */
thr_stk_t *get_thr_stk();

/** @brief Install handler for multi-threaded program */
void install_handler(void);

int mutex_underlocked(mutex_t *mp);

#endif /* THR_INTERNALS_H */
