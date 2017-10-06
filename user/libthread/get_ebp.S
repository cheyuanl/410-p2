/* get_ebp.S */
.global get_ebp

get_ebp:
    movl %ebp, %eax /* Get current %ebp */
    ret
