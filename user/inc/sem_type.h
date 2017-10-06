/** @file sem_type.h
*  @brief This file defines the type for semaphores.
*/

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <cond_type.h>
#include <mutex_type.h>

typedef struct sem {
    /* Indicate whether the semaphore is initialized or not. 1 is yes, 0 is
     * no. If init is 0, it could also mean that the semaphore has been
     * destroyed. */
    int init;

    /* The number of threads are premited to decrease the semaphore */
    int count;

    mutex_t mutex;

    cond_t cv;

} sem_t;

#endif /* _SEM_TYPE_H */
