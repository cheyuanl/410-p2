#include <syscall_int.h>

.global new_pages
print:
    push    %esi	        /* Save callee-save register */
    leal    8(%esp), %esi   /* Pass the address of syscall arugments to %esi if syscall require more than one parameter */    
    int     $NEW_PAGES_INT  /* System call */
    pop     %esi	        /* Restore callee-save register */
    ret                     /* Return */