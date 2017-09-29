/** @file thr_internals.c
 *  @brief Internal helper functions used for thread library
 *  Mutex and condition variable.
 *
 *
 *  @author Zhipeng zhao (zzhao1)
 *  @bug No known bugs.
 */
#include <stdio.h>
#include <libsimics/simics.h>
#include <thr_internals.h>
#include <malloc.h>

/** @brief mutex_init Initialize the mutex object.
 *  
*  
 *
 * @return 0 on success, negative number on error
 */
thr_t *get_thr()
{
    /* Should trace the stack of current stack to get the 
     * thr structure in the secret place. */
    /* Right now use malloc instead */
    thr_t *new_thr = (thr_t*)malloc(sizeof(thr_t));
    if(!new_thr)
        printf("Failed to allocate thread structure. \n");
    new_thr->tid = -1;
    new_thr->next = NULL;
    return new_thr;
}

