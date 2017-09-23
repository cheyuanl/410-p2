/* sleep.S */
#include <syscall_int.h>

.global sleep

sleep:
    push    %esi	    /* Save callee-save register */
    movl    8(%esp), %esi   /* Save the parameter to %esi to pass to system call */    
    int     $SLEEP_INT /* System call */
    pop	    %esi            /* Restore %esi */
    ret	                    /* Return */
