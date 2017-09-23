/* set_status.S */
#include <syscall_int.h>

.global set_status

set_status:
    push    %esi	    /* Save callee-save register */
    movl    8(%esp), %esi   /* Save the parameter to %esi to pass to system call */    
    int     $SET_STATUS_INT /* System call */
    pop	    %esi            /* Restore %esi */
    ret	                    /* Return */
