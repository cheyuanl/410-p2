#include <syscall_int.h>

.global get_cursor_pos

get_cursor_pos:
    push    %esi	        /* Save callee-save register */
    leal    8(%esp), %esi       /* Save the parameter to %esi to pass to 
                                 * system call */    
    int     $GET_CURSOR_POS_INT /* System call */
    pop	    %esi                /* Restore %esi */
    ret	                        /* Return */
