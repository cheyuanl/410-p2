/* readline.S */
#include <syscall_int.h>

.global readline
readline:
    push    %esi	    /* Save callee-save register */
    leal    8(%esp), %esi   /* Pass the address of syscall arguments to %esi
                             * if syscall require more than one parameter */    
    int     $READLINE_INT   /* System call */
    pop     %esi	    /* Restore %esi */
    ret                     /* Return */
