#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
int main(){
    int status = 4;
    lprintf("You can expect to see the process exit with status: %d\n", status);
    return status;
}
