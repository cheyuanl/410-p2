#include <syscall_int.h>
.global thr_create_asm

thr_create_asm:
    /* setup */
    push    %ebp
    mov     %esp, %ebp
    push    %esi            /* save %esi */

    /* fetch arguments */
    mov     8(%ebp),  %edx  /* 1st arg */
    mov     12(%ebp), %ecx  /* 2nd arg */
    mov     16(%ebp), %esi  /* 3rd arg */

    int     $THREAD_FORK_INT
    cmp     $0x0, %eax

    jne     parent_thread   /* jmp to parent thread */
    mov     %ecx, %esp
    mov     %edx, %ebp
    jmp     %esi
    
parent_thread:
    pop     %esi            /* restore %esi */
    leave                   /* clean up */
    ret
