#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>
#include <thread.h>
#include <thr_internals.h>
#include <mutex.h>
#include <cond.h>

/* Test both mutex, cv, thr_create. 
 * The main creates a child and wait until
 * child return. This require cond_wait.
 * There are two possibilities. 
 * First, after creation, the main's fake_thr_join gets
 * the lock, we will enqueue main thread
 * into CV's queue(Implemented using linked-list). 
 * Then wait to be woken
 * up by child. Then Child's fake_thr_exit
 * would get the lock and signal main thread,
 * dequeueing main thread from CV's queue. 
 * Child vanish eventually. Main would wake up
 * and print "end". 
 * Second case. After creation, child's fake_thr_exit
 * gets the lock and signals cv. But there is no thread
 * in the queue, so nothing would happen. Once main
 * enters fake_thr_join, it will check the 'done' signal
 * which is already '1', so it would not go to cond_wait.
 * Then main would directly print "end". 
 * From simics's test, it's always first case, but it's
 * totally possible to have second case under different
 * schedule policy. */
int done = 0;
mutex_t mp;
cond_t cv;

void fake_thr_exit(){
    lprintf("We are in exit. \n");
    mutex_lock(&mp);
    lprintf("Exit gets the lock. \n");
    done = 1;
    cond_signal(&cv);
    mutex_unlock(&mp);
    lprintf("Exit releases the lock. \n");
}

void* child(void* i) {
    lprintf("Hello from child %d", thr_getid());
    lprintf("Argument: %d", (int)i);
    fake_thr_exit();
    // while (1) {
    //     yield(-1);
    //     sleep(10);
    // }
    lprintf("Child is about to return \n");
    return 0;
}

void fake_thr_join(){
    lprintf("We are in join. \n");
    mutex_lock(&mp);
    lprintf("Join gets the lock. \n");
    while(!done)
        cond_wait(&cv, &mp);
    mutex_unlock(&mp);
    lprintf("Join releases the lock. \n");
}


int main(){
    thr_init(1024);
    mutex_init(&mp);
    cond_init(&cv);
    int ret = -2;
    int x = 10;
    ret = thr_create(child, (void*)x);
    lprintf("Create thread successfully! The return value is %d \n", 
            ret);
    fake_thr_join();
    cond_destroy(&cv);
    mutex_destroy(&mp);
    lprintf("parent: end \n");
    return 0;
}
