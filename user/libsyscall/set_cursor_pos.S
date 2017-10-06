/* set_cursor_pos.S */
#include <syscall_int.h>

.global set_cursor_pos
set_cursor_pos:
    push    %esi	        /* Save callee-save register */
    leal    8(%esp), %esi       /* Pass the address of syscall arguments to %esi
                                 * if syscall require more than one parameter */    
    int     $SET_CURSOR_POS_INT /* System call */
    pop	    %esi                /* Restore %esi */
    ret	                        /* Return */
