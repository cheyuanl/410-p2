/** @file cond.c
 *  @brief Condition Variable library.
 *
 *  This file contains methods for condition variables
 *
 *  @author Zhipeng zhao (zzhao1)
 *  @bug No known bugs.
 */
#include <stdio.h>
#include <mutex.h>
#include <cond.h>
#include <libsimics/simics.h>
#include <thr_internals.h>
#include <syscall.h>


/* Internal helper functions */
int empty(cond_t *cv);
void enq(cond_t *cv, thr_t *thr);
int deq(cond_t *cv);

/** @brief cond_init Initialize the condition variable.
 *  
 *  This method should be called exact once before calling other methods.
 *  Or this method could be called after the conditional variable has 
 *  been successfully destroyed. It's illegal to call this method 
 *  multiple times after initialization(before the condition variable is
 *  destroyed). 
 *  
 *
 * @return 0 on success, negative number on error
 */
int cond_init(cond_t *cv)
{
    if(!cv){
        printf("The input pointer is NULL \n");
        return -1;
    }
    else{
        /* Initialize the mutex */
        if(mutex_init(&(cv->mutex)) < 0){
            printf("Failed to initialize condition variable mutex! \n");
            return -1;
        }
        else{
            /* Set the init flag, initialize head and tail pointer as
             * NULL. So the queue is empty. */
            cv->init = 1;
            cv->head = NULL;
            cv->tail = NULL;
        }
    }
    return 0;
}


/** @brief cond_destroy Destroy the cond variable.
 * 
 *  Deassert the init field, so the object become uninitialized/
 *  destroyed. It's illegal to call this function when the CV_Mutex
 *  is still locked or threads are still blocked. 
 *  
 *  @return Void
 **/

void cond_destroy(cond_t *cv)
{
    if(!cv){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Condition variable is uninitialized/destroyed */
    else if(!cv->init){
        printf("The condition variable has not been initialized! \n");
        while(1);
    }
    /* The CV_Mutex is locked */
    else if(!cv->mutex.lock_available){
        printf("Some threads are still locked! \n");
        while(1);
    }
    /* Some threads are still in sleep */
    else if(cv->head != NULL){
        printf("Some threads are still blocked! \n");
        while(1);
    }
    else{
        /* Clear init, so that the lock/unlock could not be directly
         * used after destroy. */
        cv->init = 0;
        mutex_destroy(&(cv->mutex));
    }
}

/** @brief cond_wait Atomically realse the lock and block the calling
 *  thread. Will re-acquire the lock before leaving this function.
 * 
 *  @return Void
 **/

void cond_wait(cond_t *cv, mutex_t *mp)
{
    if(!cv || !mp){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Condition variable is uninitialized/destroyed */
    else if(!cv->init){
        printf("The condition variable has not been initialized! \n");
        while(1);
    }
    /* We are ready to release the lock and block the thread */
    else {
        /* Get cv_mutex before changing the cv's queue states */
        mutex_lock(&(cv->mutex));
        int reject = 0;
        /* Enq this calling thread into CV's queue */
        enq(cv, get_thr());
        /* Release the outside mutex, so other thread can acquire this
         * lock.*/
        mutex_unlock(mp);
        /* Release the cv_mutex, so that other thread can use 
         * cond_signal or cond_broadcast to change cv's queue state */
        mutex_unlock(&(cv->mutex));
        /* System call to let the calling thread go to sleep. This 
         * syscall itself is atomic with respect to the make_runnable()*/
        deschedule(&reject);
        /* Reacquire the outside lock, so when we exit, we're in
         * critical section again */
        mutex_lock(mp);
    }
}

/** @brief cond_signal Signal the thread under cond_wait.
 * 
 *  This method will only wake up the very first thread in the cv's 
 *  queue. If there is no sleeping thread in the queue, we do nothing.
 *  
 *  @return Void
 **/

void cond_signal(cond_t *cv)
{
    if(!cv){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Condition variable is uninitialized/destroyed */
    else if(!cv->init){
        printf("The condition variable has not been initialized! \n");
        while(1);
    }
    else{
        /* Get cv_mutex before changing the cv's queue states */
        mutex_lock(&(cv->mutex));
        int tid;
        /* Act only when the cv's queue is not empty */
        if(!empty(cv)){
            /* Get the first sleeping thread's id in the queue */
            tid = deq(cv);
            /* Spin on the waking up process. make_runnable would return
             * 0 only when tid exists and thread-tid is in sleep. Since 
             * tid comes from the thread queue, we are sure tid exists, 
             * and thread-tid is going to sleep. But we are not sure if
             * thread-tid is already in sleep or not, because this
             * thread might execute before thread-tid really gets into
             * sleep. So we keep trying make_runnable, until thread-tid
             * goes to sleep and make_runnable wakes it up successfully*/
            while(make_runnable(tid) < 0)
                continue;
        }
        /* Unlock the cv_mutex so that other threads can change the 
         * cv's states */
        mutex_unlock(&(cv->mutex));
    }
}

/** @brief cond_broadcast Wake up all the waiting threads
 * 
 *  If there is no sleeping thread in the queue, we do nothing. The
 *  threads that are woken up by this method would not go to sleep before
 *  the end of this method, because right now we hold the cv_mutex. The
 *  thread that want to go to sleep again has to acquire the cv_mutex
 *  first. Since we have the cv_mutex now, the thread would not go to
 *  sleep during the execution of waking up.
 *  
 *  @return Void
 **/
void cond_broadcast(cond_t *cv)
{
    if(!cv){
        printf("The input pointer is NULL \n");
        while(1);
    }
    /* Condition variable is uninitialized/destroyed */
    else if(!cv->init){
        printf("The condition variable has not been initialized! \n");
        while(1);
    }
    else{
        /* Get cv_mutex before changing the cv's queue states */
        mutex_lock(&(cv->mutex));
        int tid;
        /* If the cv's queue is not empty, we keep dequeueing */
        while(!empty(cv)){
            /* Dequeue the queue to get the thread id of a sleeping
             * thread. */
            tid = deq(cv);
            /* Spin on the condition check of make_runnable to make sure
             * the thread-tid is actually in sleep and then we wake it
             * up successfully */
            while(make_runnable(tid) < 0)
                continue;
        }
        /* Unlock the cv_mutex so that other threads can change the 
         * cv's states */
        mutex_unlock(&(cv->mutex));
    }
}

/* Internal helper functions */


/** @brief empty Check if cv's queue is empty or not.
 *  
 *  @return 1 if the queue is empty, 0 if not.
 **/

int empty(cond_t *cv){
    return (cv->head == NULL);
}

/** @brief enq Put a thread into cv's waiting thread queue. 
 *  
 *  The queue is implemented using linked list. CV holds the head and 
 *  tail pointer of this FIFO queue. When enqueue, we put the new item
 *  in tail. When dequeue, we remove an item from the head. The thr_t
 *  data structure has the "next" pointer, which will point to the next
 *  item(thead) in the queue. 
 *
 *  @return Void
 **/
void enq(cond_t *cv, thr_t *thr){
    /* If the queue is empty initially, we intialize the head and tail
     * pointer to this very first thread item. */
    if(empty(cv)){
        cv->head = thr;
        cv->tail = thr;
    }
    else{
        /* link current tail thread's next pointer to new thread */
        cv->tail->next = thr;
        /* Set the next pointer of this new thread as NULL */
        thr->next = NULL;
        /* Move the tail pointer to the new thread */
        cv->tail = thr;
    }
}


/** @brief deq Remove a thread from cv's waiting thread queue. 
 *  
 *  We remove a thread from the head of the queue. If this is the last
 *  thread item, we set the head and tail pointers as NULL.
 *
 *  @return thread id of the first thread item in the queue on success,
 *  otherwise return -1.
 **/
int deq(cond_t *cv){
    thr_t *thr;
    /* No element in queue. This should not happen, since the caller
     * is responsible to check if the queue is empty or not before even
     * call this method. */
    if(empty(cv)){
        printf("no elements in queue! \n");
        return -1;
    }
    else{
        /* Return the thread item pointed by head pointer */
        thr = cv->head;
        /* This is the last element, clear head and tail pointers */
        if(cv->head == cv->tail){
            cv->head = NULL;
            cv->tail = NULL;
        }
        else{
            /* Move head pointer forward */
            cv->head = cv->head->next;
        }
    }
    /* We only return the tid field of this thread. */
    return thr->tid;
}
