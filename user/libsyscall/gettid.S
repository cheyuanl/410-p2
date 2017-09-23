/* gettid.S */
#include <syscall_int.h>

.global gettid

gettid:
    int     $GETTID_INT /* System call */
    ret	                    /* Return */
