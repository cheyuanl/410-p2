/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <cond_type.h>

typedef struct rwlock {
    /* Indicate whether the rwlock is initialized or not. 
     * 1 is yes, 0 is no. If init is 0, it could also mean that 
     * the rwlock has been destroyed */
    int init;

    int num_reader;
    int num_wait_writer;

    /* The utid of writer thread */
    int writer_tid;
    /* Writer_tid is valid only when write_flag is 1 */
    int write_flag;


    cond_t writer_cv;
    cond_t reader_cv;

    mutex_t mutex;
    
    
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
