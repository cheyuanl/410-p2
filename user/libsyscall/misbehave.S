/* misbehave.S */
#include <syscall_int.h>

.global misbehave
misbehave:
    push    %esi	     /* Save callee-save register */
    movl    8(%esp), %esi    /* Save the parameter to %esi to pass to system 
                              * call */    
    int     $MISBEHAVE_INT   /* System call */
    pop     %esi	     /* Restore %esi */
    ret                      /* Return */
