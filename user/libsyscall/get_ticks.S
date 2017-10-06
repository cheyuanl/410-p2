/* get_ticks.S */
#include <syscall_int.h>

.global get_ticks
get_ticks:
    int     $GET_TICKS_INT   /* System call */
    ret                      /* Return */
