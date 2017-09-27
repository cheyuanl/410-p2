#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>
#include <thr_internals.h>

int main(){
    int ret;
    int lock_available = 1;
    ret = xchg_wrapper(&lock_available, 0);
    printf("Expect 1, The return value is %d \n", ret);
    ret = xchg_wrapper(&lock_available, 1);
    printf("Expect 0, The return value is %d \n", ret);
    ret = xchg_wrapper(&lock_available, 1);
    printf("Expect 1, The return value is %d \n", ret);
    return 0;
}
