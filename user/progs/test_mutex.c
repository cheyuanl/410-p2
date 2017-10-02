#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>
#include <thr_internals.h>
#include <mutex.h>

#define TEST5

int main(){
    printf("The expected value is in ()\n");
    /* Case 1, correct order. */
#ifdef TEST1
    mutex_t mp;
    int ret;
    printf("!!!Undefined: lock_available = %d(\?); init = %d(\?) \n", 
           mp.lock_available, mp.init);

    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init);
    mutex_lock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_unlock(&mp);
    printf("lock_available = %d(1); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_destroy(&mp);
    printf("lock_available = %d(0); init = %d(0). \n", 
           mp.lock_available, mp.init);

    /* Next round */
    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init);
    mutex_lock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_unlock(&mp);
    printf("lock_available = %d(1); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_destroy(&mp);
    printf("lock_available = %d(0); init = %d(0). \n", 
           mp.lock_available, mp.init);
#endif

    /* Case 2, NULL pointer */
#ifdef TEST2
    mutex_t *mp=NULL;//Null pointer
    int ret;
    ret = mutex_init(mp);
    printf("Ret = %d(-1) \n", ret);    
#endif

    /* Case 3, lock without init */
#ifdef TEST3
    mutex_t mp;
    printf("!!!Undefined: lock_available = %d(\?); init = %d(\?) \n", 
           mp.lock_available, mp.init);

    mutex_unlock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
#endif

    /* Case 4, unlock before lock*/
#ifdef TEST4
    mutex_t mp;
    int ret;
    printf("!!!Undefined: lock_available = %d(\?); init = %d(\?) \n", 
           mp.lock_available, mp.init);

    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init); 
    mutex_unlock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_lock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
#endif

    /* Case 5, destroy before unlock*/
#ifdef TEST5
    mutex_t mp;
    int ret;
    printf("!!!Undefined: lock_available = %d(\?); init = %d(\?) \n", 
           mp.lock_available, mp.init);

    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init); 
    mutex_lock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
    mutex_destroy(&mp);
    printf("lock_available = %d(0); init = %d(0). \n", 
           mp.lock_available, mp.init);
    mutex_unlock(&mp);
    printf("lock_available = %d(1); init = %d(1). \n", 
           mp.lock_available, mp.init);
#endif

    /* Case 6, Init after lock. This is illegal and will lead to
     * undefined behavior. */
#ifdef TEST6
    mutex_t mp;
    int ret;
    printf("!!!Undefined: lock_available = %d(\?); init = %d(\?) \n", 
           mp.lock_available, mp.init);

    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init); 
    mutex_lock(&mp);
    printf("lock_available = %d(0); init = %d(1). \n", 
           mp.lock_available, mp.init);
    ret = mutex_init(&mp);
    printf("Ret = %d(0); lock_available = %d(1); init = %d(1). \n", 
           ret, mp.lock_available, mp.init); 
    mutex_destroy(&mp);
    printf("lock_available = %d(0); init = %d(0). \n", 
           mp.lock_available, mp.init);
    mutex_unlock(&mp);
    printf("lock_available = %d(1); init = %d(1). \n", 
           mp.lock_available, mp.init);
#endif

    return 0;
}
