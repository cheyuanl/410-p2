#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>

int main(){
    int tid;
    tid = gettid();
    lprintf("Expect to see tid \"%d\" in console",tid);
    
    return 0;
}
