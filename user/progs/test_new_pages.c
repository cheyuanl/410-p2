#include <libsimics/simics.h>
#include <stdio.h>
#include <syscall.h>

#define PAGE_ALIGN_MASK ((unsigned int) ~((unsigned int) (PAGE_SIZE-1)))

/** @brief Request new pages starting from lower address */
int main() {
    int *base = (int*)0x01000000;
    int status;

    while(1) {
        status = new_pages(base, PAGE_SIZE);
        if(status == 0) {
            printf("base ptr: %p\n", base);
            printf("new page returns: %d\n", status);
            MAGIC_BREAK;
            return 0;
        }
        base += PAGE_SIZE;
        base = (int*)((int)base & PAGE_ALIGN_MASK);
    }
    return -1;
}