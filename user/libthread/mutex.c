/** @file mutex.c
 *  @brief Mutex library.
 *
 *  This file contains methods for mutex. Mutex is used to prevent 
 *  simultaneous execution of critical section by multiple threads.
 *  If a thread acquires a mutex, it can safely modify some global
 *  information in the critical section. The other threads that try
 *  to acquire the mutex will do busy waiting. We assume the running
 *  environment will be multi-processor. In that case, it's ok to 
 *  let one processor spin, because the critical section is usually
 *  short, the thread that spinning would acquire the lock in a short
 *  period. The may save time to do the context-switch which could
 *  be expensive.  
 *
 *  The mutex is implemented using atomic instruction XADD. This
 *  is a ticket-lock. Each thread that tries to aquire the lock would
 *  be assigned a ticket number and at the same time the total ticket
 *  number would increase. There is another field that keeps track of
 *  the "turn", which will increase as a thread releases the acquired
 *  lock. Only when a thread's assigned ticket value matches the global
 *  "turn", this thread could acquire the lock. This ensures that all
 *  the threads that try to acquire the lock can eventually get the
 *  lock.  
 *
 *  @author Zhipeng Zhao (zzhao1)
 *  @bug If the number of mutex_lock() calls exceeds the TMAX, the
 *  behavior is undefined. 
 */
#include <assert.h>
#include <stdio.h>
#include <mutex.h>
#include <libsimics/simics.h>
#include <thr_internals.h>
#include <syscall.h>


/** @brief mutex_init Initialize the mutex object.
 *  
 *  This method should be called exact once before calling other methods.
 *  Or this method could be called after the mutex has been successfully
 *  destroyed. It's illegal to call this method multiple times after 
 *  initialization(before the mutex is destroyed). It's also illegal to 
 *  call other methods before initialization. Since this is the initialization
 *  function, we cannot trust the uninitialization value of the input
 *  struct members, thus here we only check the arugment pointer without
 *  checking the struct member values.
 *  
 *  @param mp The mutex object
 *  @return 0 on success, negative number on error
 */
int mutex_init(mutex_t *mp)
{
    if(!mp){
        printf("Mutex_init: The mutex input pointer is NULL \n");
        lprintf("Mutex_init: The mutex input pointer is NULL \n");
        return -1;
    }
    else{            
        mp->ticket = 0;
        mp->turn = 0;
        mp->init = 1;
    }
    return 0;
}


/** @brief mutex_destroy Destroy the mutex object.
 * 
 *  Deassert the init field, so the object become
 *  uninitialized/destroyed. 
 *  
 *  @param mp The mutex object
 *  @return Void
 **/
void mutex_destroy(mutex_t *mp)
{
    if(!mp){
        panic("Mutex_destroy: The input pointer is NULL");
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        panic("Mutex_destroy: The mutex has not been initialized!");
    }
    /* Mutex is locked */
    else if(mutex_underlocked(mp)){
        panic("Mutex_destroy: The mutex has not been unlocked!");
    }
    else{
        /* Clear init, so that the lock/unlock could not be directly
         * Used after destroy. */
        mp->init = 0;
        mp->ticket = 0;
        mp->turn = 0;
    }
}

/** @brief mutex_lock Acquire the lock. 
 * 
 *  If a thread calls this function and returned, that means that thread
 *  gets the lock. Otherwise, that thread will be blocked util it gets
 *  the lock. 
 *  
 *  @param mp The mutex object
 *  @return Void
 **/
void mutex_lock(mutex_t *mp)
{
    if(!mp){
        panic("Mutex_lock: The input pointer is NULL");
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        panic("Mutex_lock: The mutex has not been initialized!");
    }
    /* Spin-wait to get the lock */
    else {
        /* Get current ticket value and atomically increase the
         * ticket value. */
        int myturn = xadd_wrapper(&(mp->ticket));
        /* Wait until the ticket value in my hand matches the 
         * global turn */
        while(mp->turn != myturn)
            continue;
    }
}

/** @brief mutex_unlock Release the lock. 
 * 
 *  The lock has to been acquired by someone before
 *  this function is invoked. 
 *  
 *  @param mp The mutex object
 *  @return Void
 **/
void mutex_unlock(mutex_t *mp)
{
    if(!mp){
        panic("Mutex_unlock: The input pointer is NULL");
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        panic("Mutex_unlock: The mutex has not been initialized!");
    }
    else{
        /* increase the turn value, so the next thread could acquire
         * the lock */
        mp->turn = mp->turn + 1;
    }
}

/** @brief Check if the mutex is still locked or not
 * 
 *  
 *  @param mp The mutex object
 *  @return 1 if still locked 0 if not
 **/
int mutex_underlocked(mutex_t *mp)
{
    return (mp->turn != mp->ticket);
}


