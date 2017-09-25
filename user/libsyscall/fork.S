/* fork.S */
#include <syscall_int.h>

.global fork
fork: 
    int     $FORK_INT       /* System call */
    ret                     /* Return */
