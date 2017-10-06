/* make_runnable.S */
#include <syscall_int.h>

.global make_runnable

make_runnable:
    push    %esi	    /* Save callee-save register */
    movl    8(%esp), %esi   /* Save the parameter to %esi to pass to system 
                             * call */    
    int     $MAKE_RUNNABLE_INT /* System call */
    pop	    %esi            /* Restore %esi */
    ret	                    /* Return */
