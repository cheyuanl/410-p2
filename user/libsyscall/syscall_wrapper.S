/* set_status.S */
#include <spec/syscall_int.h>

.global set_status
.global vanish

set_status:
    pusha	                /* Save all general purpose register on stack  */
    movl    8(%ebp), %esi   /* Save the parameter to %esi. */    
    int     $SET_STATUS_INT /* Call set status with trap */
    popa	                /* Restore the general purpose register */
    ret	                    /* Return */

vanish: 
    pusha                   /* Save all general purpose register on stack  */
    int     $VANISH_INT     /* Call vanish with trap */
    popa                    /* Restore the general purpose register */
    ret                     /* Return */