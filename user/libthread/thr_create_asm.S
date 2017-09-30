#include <syscall_int.h>
.global thr_create_asm

thr_create_asm:
    /* setup */
    push    %ebp
    mov     %esp, %ebp
    //sub     $0x8, %esp
    push    %esi /* Save %esi */

    mov     8(%ebp), %eax  /* first arg */
    mov     %eax, %edx 
    mov     12(%ebp), %eax /* second arg */
    mov     %eax, %ecx 
    mov     16(%ebp), %eax /* third arg */
    mov     %eax, %esi

    int     $THREAD_FORK_INT
    cmp     $0x0, %eax

    jne     parent_thread  /* parent thread do nothing */
    mov     %ecx, %esp
    mov     %edx, %ebp
    jmp     %esi
    
parent_thread:
    pop     %esi    
    leave
    ret
