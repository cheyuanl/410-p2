/* getchar.S */
#include <syscall_int.h>

.global getchar
getchar:
    int     $GETCHAR_INT    /* System call */
    ret                     /* Return */
