/* vanish.S */
#include <syscall_int.h>

.global vanish
vanish: 
    int     $VANISH_INT     /* Call vanish with trap */
    ret                     /* Return */
