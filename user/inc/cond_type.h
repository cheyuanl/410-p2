/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H
#include <mutex_type.h>
#include <thr_internals.h>

typedef struct cond {
  /* Indicate whether the mutex is initialized or not. 1 is yes, 0 is 
   * no. If init is 0, it could also mean that the mutex has been
   * destroyed. */
  int init;
  /* The lock used in condition variable's world to make sure the queue
   * state changes are protected. */
  mutex_t mutex;
  /* The head pointer of the waiting thread queue. When deq an item 
   * from the queue, the head move to the next item using thr_t data
   * structure's "next" pointer. */
  thr_t *head;
  /* The tail pointer of the waiting thread queue. When enq an item 
   * to the queue, the current tail would link with the new item using 
   * thr_t data structure's "next" pointer. Then the tail pointer would
   * be moved to the new thread item. The new thread item's "next" 
   * pointer is NULL. */
  thr_t *tail;
} cond_t;

#endif /* _COND_TYPE_H */
