#include <libsimics/simics.h>
#include <syscall.h>
#include <thread.h>
#include <assert.h>
#include <cond.h>
#include <mutex.h>

#define RUN_THR_NUM 3
#define TEST4
/* 4 threads are runnable, 1 thread is sleeping. */
/* test 1, utid 0 -> 3 would run in a roundrobin fashion */

/* test 2, thr_yield(-1), the root thread would run
 * less frequent. There should not be any visible print in console */

/* test 3, thr_yield(4), should return not runnable error. */

/* test 4, thr_yield(1), Only thread 1 is visible */

int done = 0;
mutex_t mp;
cond_t cv;


void* runnable(void* i) {
#ifdef TEST4
    int ret;
#endif
    /* make sure the runnable thread at least run once. */
    mutex_lock(&mp);
    lprintf("LOCK: Utid %d, Ktid %d \n", thr_getid(), gettid());
    done++;
    if(done == RUN_THR_NUM + 2)
        cond_signal(&cv);
    mutex_unlock(&mp);
    lprintf("UNLOCK: Utid %d, Ktid %d \n", thr_getid(), gettid());

    while(1){
//        continue;
#ifdef TEST4
        if(thr_getid() != 1){
            ret = thr_yield(1);
            if(ret != 0)
                panic("thr_yield return error.");
        }
#endif
        lprintf("Utid %d, Ktid %d \n", thr_getid(), gettid());
    }
    vanish();
}

void* insleep(void* i) {
    /* make sure the thread at least register before sleep */
    mutex_lock(&mp);
    lprintf("LOCK: Utid %d, Ktid %d \n", thr_getid(), gettid());
    done++;
    if(done == RUN_THR_NUM + 2)
        cond_signal(&cv);
    mutex_unlock(&mp);
    lprintf("UNLOCK: Utid %d, Ktid %d \n", thr_getid(), gettid());

    int rej = 0;
    deschedule(&rej);
    panic("This should never show up");
    vanish();
}

int main() {
    int ret;
    int i;
    thr_init(1024);
    mutex_init(&mp);
    cond_init(&cv);

    for (i=0 ; i<RUN_THR_NUM; i++){
        ret = thr_create(runnable, (void*)i);
        if(ret>0)
            lprintf("Create runnable thread %d, "
                    "The return value is %d \n", 
                    i+1, ret);
    }
    ret = thr_create(insleep, (void*)i);
        if(ret>0)
            lprintf("Create insleep thread %d," 
                    "The return value is %d \n", 
                    i+1, ret);
    /* wait for all the child threads start running */
    mutex_lock(&mp);
    done++;
    while(done != RUN_THR_NUM + 2)
        cond_wait(&cv, &mp);
    mutex_unlock(&mp);

    while(1){
#ifdef TEST2
        ret = thr_yield(-1);
        if(ret != 0)
            panic("thr_yield return error.");
#endif
#ifdef TEST3
        ret = thr_yield(4);
        if(ret != 0)
            panic("thr_yield return error.");
#endif
#ifdef TEST4
        ret = thr_yield(1);
        if(ret != 0)
            panic("thr_yield return error.");
#endif
       lprintf("Utid %d, Ktid %d \n", thr_getid(), gettid());
    }
    return 0;
}
