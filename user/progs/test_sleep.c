#include <stdio.h>
#include <stdlib.h>
#include <libsimics/simics.h>
#include <syscall.h>
#include <string.h>

#define SLEEP_TICKS 2000

int main(){
    unsigned int start_tick;
    unsigned int end_tick;
    start_tick = get_ticks();
    if(sleep(SLEEP_TICKS) < 0)
        lprintf("SLEEP_TICKS is negative, %d \n", SLEEP_TICKS);

    end_tick = get_ticks();

    lprintf("Expect to sleep %d ticks \n", SLEEP_TICKS);
    lprintf("The start_tick is %d; The end_tick is %d \n", 
            start_tick, end_tick);
    lprintf("The actual sleep is end-start = %d ticks \n", 
            end_tick - start_tick);
    
    return 0;
}
