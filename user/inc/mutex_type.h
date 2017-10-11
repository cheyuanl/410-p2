/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H


typedef struct mutex {
    /* Indicate whether the mutex is initialized or not. 1 is yes, 0 is 
     * no. If init is 0, it could also mean that the mutex has been
     * destroyed. */
    int init;
    /* The ticket value that to be assigned to each thread. The value will
     * be increased atomically when assigning to each thread. */
    int ticket;
    /* The global turn for waiting threads. Only when a thread's ticket
     * matches this global turn, the thread could get the lock. */
    int turn;
} mutex_t;
#endif /* _MUTEX_TYPE_H */
