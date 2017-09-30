#include <syscall_int.h>
.global thr_create_asm

thr_create_asm:
    /* setup */
    push    %ebp
    mov     %esp, %ebp
    push    %esi            /* save %esi */
    push    %edi            /* save %edi */

    /* fetch arguments */
    mov     8(%ebp),  %edx  /* 1st arg, addr of first element in child
                             * stack, which is expected %ebp after 
                             * thread fork. */
    mov     12(%ebp), %ecx  /* 2nd arg, addr of last element in child
                             * stack, which is expected %esp after 
                             * thread fork. */
    mov     16(%ebp), %esi  /* 3rd arg, addr of ktid in child stack */
    mov     20(%ebp), %edi  /* 4th arg, the child thread func */

    int     $THREAD_FORK_INT
    cmp     $0x0, %eax

    jne     parent_thread   /* jmp to parent thread */
    mov     %ecx, %esp      /* restore the expected %esp for the child
                             * thread */
    mov     %edx, %ebp      /* restore the expected %ebp for the child
                             * thread */
    mov     %eax, (%esi)    /* store the ktid returned by syscall */
    jmp     %edi            /* Jump the func for the child thread */
    
parent_thread:
    pop     %esi            /* restore %esi */
    pop     %edi            /* restore %edi */
    leave                   /* clean up */
    ret
