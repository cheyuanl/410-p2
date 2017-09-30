/** @file mutex.c
 *  @brief Mutex library.
 *
 *  This file contains methods for mutex
 *
 *  @author Zhipeng zhao (zzhao1)
 *  @bug No known bugs.
 */
#include <stdio.h>
#include <mutex.h>
#include <libsimics/simics.h>
#include <thr_internals.h>

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
 *
 * @return 0 on success, negative number on error
 */
int mutex_init(mutex_t *mp)
{
    if(!mp){
        printf("The mutex input pointer is NULL \n");
        return -1;
    }
    else{
        mp->lock_available = 1;
        mp->init = 1;
    }
    return 0;
}


/** @brief mutex_destroy Destroy the mutex object.
 * 
 *  Deassert the init field, so the object become
 *  uninitialized/destroyed. 
 *  
 *  @return Void
 **/

void mutex_destroy(mutex_t *mp)
{
    if(!mp){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        printf("The mutex has not been initialized! \n");
        while(1);
    }
    /* Mutex is locked */
    else if(!mp->lock_available){
        printf("The mutex is locked! \n");
        while(1);
    }
    else{
        /* Clear init, so that the lock/unlock could not be directly
         * Used after destroy. */
        mp->init = 0;
        mp->lock_available = 0;
    }
}

/** @brief mutex_lock Acquire the lock. 
 * 
 *  If a thread calls this function and returned, that means that thread
 *  gets the lock. Otherwise, that thread will be blocked util it gets
 *  the lock. 
 *  
 *  @return Void
 **/

void mutex_lock(mutex_t *mp)
{
    if(!mp){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        printf("The mutex has not been initialized! \n");
        while(1);
    }
    /* Spin-wait to get the lock */
    else {
        while(!xchg_wrapper(&(mp->lock_available),0))
            continue;
    }
}

/** @brief mutex_unlock Release the lock. 
 * 
 *  The lock has to been acquired by someone before
 *  this function is invoked. 
 *  
 *  @return Void
 **/

void mutex_unlock(mutex_t *mp)
{
    if(!mp){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Mutex is uninitialized/destroyed */
    else if(!mp->init){
        printf("The mutex has not been initialized! \n");
        while(1);
    }
    else{
        /* expect 0 */
        if(xchg_wrapper(&(mp->lock_available),1)){
            printf("Should do unlock after lock! \n");
            while(1);
        }
    }
}


