/* set_status.S */
#include <syscall_int.h>

.global set_status

set_status:
    push    %esi	    /* Save all general purpose register on stack  */
    movl    8(%ebp), %esi   /* Save the parameter to %esi. */    
    int     $SET_STATUS_INT /* Call set status with trap */
    pop	    %esi            /* Restore the general purpose register */
    ret	                    /* Return */
