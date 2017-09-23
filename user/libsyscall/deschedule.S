/* deschedule.S */
#include <syscall_int.h>

.global deschedule

deschedule:
    push    %esi	    /* Save callee-save register */
    movl    8(%esp), %esi   /* Save the parameter to %esi to pass to system call */    
    int     $DESCHEDULE_INT /* System call */
    pop	    %esi            /* Restore %esi */
    ret	                    /* Return */
