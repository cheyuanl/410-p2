#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>
#include <thread.h>
#include <thr_internals.h>
#include <mutex.h>
#include <cond.h>

#define THREAD_NUM 2
int done = 0;
mutex_t mp;
cond_t cv;

void fake_thr_exit(){
    lprintf("Utid %d, Ktid %d is in exit. \n",
             thr_getid(), gettid());
    mutex_lock(&mp);
    lprintf("Utid %d, Ktid %d Exit gets the lock. \n",
             thr_getid(), gettid());
    done++;
    if(done == THREAD_NUM)
       cond_signal(&cv);
    mutex_unlock(&mp);
    lprintf("Utid %d, Ktid %d Exit releases the lock. \n",
             thr_getid(), gettid());
}

void* child(void* i) {
    lprintf("Utid %d, Ktid %d Hello", 
             thr_getid(), gettid());
    lprintf("Argument: %d", (int)i);
    MAGIC_BREAK;
    int *ptr;
    ptr = (int *)malloc(sizeof(int));
    if(!ptr)
        lprintf("Utid %d, Ktid %d Memory not allocated", 
                 thr_getid(), gettid());
    else
        free(ptr);
    MAGIC_BREAK;
    fake_thr_exit();
    // while (1) {
    //     yield(-1);
    //     sleep(10);
    // }
    lprintf("Utid %d, Ktid %d is about to return. \n", 
             thr_getid(), gettid());
    return 0;
}

void fake_thr_join(){
    lprintf("Utid %d, Ktid %d in join. \n",
             thr_getid(), gettid());
    mutex_lock(&mp);
    lprintf("Utid %d, Ktid %d Join gets the lock. \n",
             thr_getid(), gettid());
    while(done != THREAD_NUM)
        cond_wait(&cv, &mp);
    mutex_unlock(&mp);
    lprintf("Utid %d, Ktid %d Join releases the lock. \n",
             thr_getid(), gettid());
}


int main(){
    thr_init(1024);
    mutex_init(&mp);
    cond_init(&cv);
    int ret = -2;
    int i;
    for(i=0; i<THREAD_NUM; i++){
        ret = thr_create(child, (void*)i);
        lprintf("Create thread successfully! The return value is %d \n", 
                ret);
    }
    fake_thr_join();
    mutex_destroy(&malloc_mp);
    cond_destroy(&cv);
    mutex_destroy(&mp);
    lprintf("parent: end \n");
    return 0;
}
