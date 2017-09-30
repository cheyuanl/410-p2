/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */

#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

/** @brief The bounds of main thread's stack */
void *main_stk_lo;
void *main_stk_hi;
/** @brief The head of thread stack */
void *thr_stk_head;
/** @brief The current thread stack address to be allocated */
void *thr_stk_curr;

/** @brief The structure of the head block of thread stack */
typedef struct thr_stk_t thr_stk_t;
struct thr_stk_t {
    void *ret_addr;     /* the return addr for entry function */
    void *args;         /* args for the entry function */
    thr_stk_t *cv_next; /* pointer to next conditional variable thread */
    thr_stk_t *next;    /* pointer to next thread */
    thr_stk_t *prev;    /* pointer to prev thread */
    int utid;          /* the user thread id */
    int ktid;          /* the kernel thread id */
    int zero;           /* the value indicates the begining of stack */
};

int xchg_wrapper(int *lock_available, int val);

thr_t *get_thr(void);

void *get_ebp();

int get_eax();

/** @brief Allocate chunck of memory from (hi-size) to hi */
void *stk_alloc(void *hi, int nbyte);

/** @brief Perform thread fork and move chlld thread to the new stack */
int thr_create_asm(void *ebp, void *esp, void *func);

/** @brief Get the pointer to this thread stack structure */
thr_stk_t *get_thr_stk();

#endif /* THR_INTERNALS_H */
