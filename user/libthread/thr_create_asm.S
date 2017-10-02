#include <syscall_int.h>
.global thr_create_asm

thr_create_asm:
    /* setup */
    push    %ebp
    mov     %esp, %ebp

    /* fetch arguments */
    mov     8(%ebp),  %edx  /* 1st arg, addr of first element in child
                             * stack, which is expected %ebp after
                             * thread fork. */
    mov     12(%ebp), %ecx  /* 2nd arg, addr of last element in child
                             * stack, which is expected %esp after
                             * thread fork. */

    int     $THREAD_FORK_INT
    cmp     $0x0, %eax

    jne     parent_thread   /* jmp to parent thread */
    mov     %ecx, %esp      /* restore the expected %esp for the child
                             * thread */
    mov     %edx, %ebp      /* restore the expected %ebp for the child
                             * thread */
    jmp     thr_func_wrapper/* Jump the func for the child thread */

parent_thread:
    leave                   /* clean up */
    ret
