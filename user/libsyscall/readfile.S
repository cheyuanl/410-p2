/* readfile.S */
#include <syscall_int.h>

.global readfile
readfile:
    push    %esi	    /* Save callee-save register */
    leal    8(%esp), %esi   /* Pass the address of syscall arugments to %esi
                             * if syscall require more than one parameter */    
    int     $READFILE_INT   /* System call */
    pop     %esi	    /* Restore %esi */
    ret                     /* Return */
