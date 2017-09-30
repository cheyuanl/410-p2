/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H


typedef struct mutex {
  /* Indicate whether a lock is available or not. 1 is available, and 0
   * is not.*/
  int lock_available;
  /* Indicate whether the mutex is initialized or not. 1 is yes, 0 is 
   * no. If init is 0, it could also mean that the mutex has been
   * destroyed. */
  int init;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
