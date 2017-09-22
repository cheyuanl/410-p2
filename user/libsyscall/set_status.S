/* set_status.S */
//#include <spec/syscall_int.h>

.global set_status

set_status:
    pushl %ebp  /* Save old %ebp */
    movl  %esp, %ebp  /* Save old %ebp */
    pusha	/* Save all general purpose register on stack  */

    movl 8(%ebp), %esi /* Save the parameter to %esi. */    
    int $0x59	/* Call the timer_handler */
    popa	/* Restore the general purpose register */
    popl  %ebp
    iret	/* Interrupt return, jump back to the program that was interrupted */
