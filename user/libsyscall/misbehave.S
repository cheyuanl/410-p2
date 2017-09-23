#include <syscall_int.h>

.global misbehave
misbehave:
    push    %esi	         /* Save callee-save register */
    movl    8(%esp), %esi    /* Pass the address of syscall arugments to %esi if syscall require more than one parameter */    
    int     $MISBEHAVE_INT   /* System call */
    pop     %esi	         /* Restore callee-save register */
    ret                      /* Return */