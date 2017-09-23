/* vanish.S */
#include <syscall_int.h>

.global vanish
vanish: 
    int     $VANISH_INT     /* System call */
    ret                     /* Return */
