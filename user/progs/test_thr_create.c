#include <libsimics/simics.h>
#include <syscall.h>
#include <thread.h>

void* func(void* i) {
    lprintf("Hello from child %d", thr_getid());
    lprintf("Argument: %d", (int)i);
    MAGIC_BREAK;
    // while (1) {
    //     yield(-1);
    //     sleep(10);
    // }
    return 0;
}

int main() {
    thr_init(1024);
    int ret = -2;
    int x = 10;
    ret = thr_create(func, (void*)x);
    lprintf("Create thread successfully! The return value is %d \n", ret);
    return 0;
}
