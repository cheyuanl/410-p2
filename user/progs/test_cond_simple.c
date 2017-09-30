#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>
#include <thr_internals.h>
#include <mutex.h>
#include <cond.h>

#define TEST1


int main(){
    printf("The expected value is in ()\n");
    /* Case 1, init and destroy */
#ifdef TEST1
    cond_t cv;
    int ret;

    ret = cond_init(&cv);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, cv.mutex.lock_available, cv.init);
    cond_signal(&cv);

    cond_destroy(&cv);
    printf("lock_available = %d(0); init = %d(0). \n", 
           cv.mutex.lock_available, cv.init);

#endif
    /* Case 2, wait */
#ifdef TEST2
    cond_t cv;
    mutex_t mp;
    int ret;
    mutex_init(&mp);
    ret = cond_init(&cv);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, cv.mutex.lock_available, cv.init);
    mutex_lock(&mp);
    cond_wait(&cv,&mp);
    printf("You are not supposed to see this line! \n");
#endif
    /* Case 3, lock before destroy */
#ifdef TEST3
    cond_t cv;
    mutex_t mp;
    int ret;
    mutex_init(&mp);
    ret = cond_init(&cv);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, cv.mutex.lock_available, cv.init);
    mutex_lock(&(cv.mutex));
    cond_destroy(&cv);
#endif
      return 0;
}
