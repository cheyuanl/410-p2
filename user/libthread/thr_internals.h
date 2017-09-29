/** @file thr_internals.h
 *
 *  @brief This file may be used to define things
 *         internal to the thread library.
 */



#ifndef THR_INTERNALS_H
#define THR_INTERNALS_H

typedef struct thr{
    int tid;
    struct thr *next;
}thr_t;


int xchg_wrapper(int *lock_available, int val);
thr_t *get_thr(void);

#endif /* THR_INTERNALS_H */
