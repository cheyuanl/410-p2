/* xchg_wrapper.S */

.global xchg_wrapper
xchg_wrapper:
    movl    4(%esp), %ecx   /* Pass the lock_available's addr to reg */    
    movl    8(%esp), %edx   /* Pass the new value to another reg */    
    xchg    (%ecx), %edx    /* Atomically exchange lock_available's value and the new value */
    movl    %edx, %eax      /* Return the lock_available's value */
    ret                     /* Return */
