#include <syscall_int.h>
.global thr_create_asm

thr_create_asm:
    /* setup */
    push    %ebp
    mov     %esp, %ebp
    sub     $0x8, %esp

    mov     8(%ebp), %eax  /* first arg */
    mov     %eax, -4(%ebp) 
    mov     12(%ebp), %eax /* second arg */
    mov     %eax, -8(%ebp) 
    mov     16(%ebp), %eax /* third arg */
    mov     %eax, %esi

    int     $THREAD_FORK_INT
    cmp     $0x0, %eax

    jne     parent_thread  /* parent thread do nothing */
    mov     -8(%ebp), %esp
    mov     -4(%ebp), %ebp
    jmp     %esi
    
parent_thread:    
    leave
    ret
