/* If you want to use assembly language instead of C,
 * delete this autostack.c and provide an autostack.S
 * instead.
 */
#include <thread.h>

void install_autostack(void *stack_high, void *stack_low) {
    main_stk_hi = stack_high;
    main_stk_lo = stack_low;
}
