/* remove_pages.S */
#include <syscall_int.h>

.global remove_pages
remove_pages:
    push    %esi	      /* Save callee-save register */
    movl    8(%esp), %esi     /* Pass the address of syscall arugments to %esi
                               * if syscall require more than one parameter */    
    int     $REMOVE_PAGES_INT /* System call */
    pop     %esi	      /* Restore %esi */
    ret                       /* Return */
