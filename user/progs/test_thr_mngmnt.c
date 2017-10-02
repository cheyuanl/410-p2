#include <thread.h>
#include <thr_internals.h>
#include <simics.h>


void* func(void* i) {
    //lprintf("Hello from child %d", thr_getid());
    //lprintf("Argument: %d", (int)i);
    // while (1) {
    //     yield(-1);
    //     sleep(10);
    // }
    return 0;
}

int main(int argc, char *argv[]) {

    thr_init(1024);

    int i = 0;
    int utids[5];

    for(; i < 5; ++i) {
        utids[i] = thr_create(func, (void*)i);
        lprintf("Create thread successfully! The return value is %d \n", utids[i]);
    }

    i = 0;
    for(; i < 5; ++i) {
        lprintf("Finding utid: %d", utids[i]);
        thr_stk_t *p = thr_find(utids[i]);
        lprintf("found %p", (void*)p);

        MAGIC_BREAK;
        lprintf("Removing it");
        thr_remove(p);

        lprintf("Finding utid: %d again", utids[i]);
        p = thr_find(utids[i]);
        lprintf("found %p", (void*)p);
        MAGIC_BREAK;
    }
}
