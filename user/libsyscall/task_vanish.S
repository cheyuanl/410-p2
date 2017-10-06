/* task_vanish.S */
#include <syscall_int.h>

.global task_vanish
task_vanish:
    push    %esi	     /* Save callee-save register */
    movl    8(%esp), %esi    /* Pass the address of syscall arugments to %esi
                              * if syscall require more than one parameter */
    int     $TASK_VANISH_INT /* System call */
    pop     %esi	     /* Restore %esi */
    ret                      /* Return */
