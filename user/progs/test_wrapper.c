#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
int main(){
    lprintf("Hello world \n");
    MAGIC_BREAK;
    set_status(0);
    MAGIC_BREAK;
    return 0;
}
