/* set_term_color.S */
#include <syscall_int.h>

.global set_term_color
set_term_color:
    push    %esi	        /* Save callee-save register */
    movl    8(%esp), %esi       /* Save the parameter to %esi to pass to system
                                 * call */    
    int     $SET_TERM_COLOR_INT /* System call */
    pop	    %esi                /* Restore %esi */
    ret	                        /* Return */
