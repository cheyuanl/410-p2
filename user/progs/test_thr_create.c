#include <libsimics/simics.h>
#include <thread.h>

void* func(void* i) {
    lprintf("Hello child");
    MAGIC_BREAK;
    return 0;
}

int main() {
    
    thr_init(1024);

    int x = 10;
    thr_create(func, (void*)x);

    return 0;
}
