/** @file autostack.c
 *  @brief Automatically grow the stack for Legacy single-threaded
 *         programs. 
 *
 *  Before call main(), we will install the handler for
 *  autostack growth using the syscall swexn. 
 *
 *  When a pagefault happens, the handler will check if this is
 *  a pagefault, if this is in user-mode, and if this is due to
 *  access non-present page(auto stack growth should happen due to
 *  access to non-present page instead of page-level protection 
 *  violation). Then the handler would allocate new pages by 
 *  doubling the existing stack size. Lastly, the handler would
 *  retry the instruction that has caused the pagefault. 
 *
 *  @author Zhipeng zhao (zzhao1)
 *  @bug No known bugs.
 */
#include <thread.h>
#include <syscall.h>
#include <string.h>
#include <simics.h>
#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <excp_handler.h>

/** @brief The current aligned stack high addr */
static void *stack_high_aligned;

/** @brief The current aligned stack low addr */
static void *stack_low_aligned;

static void auto_stack_handler(void *arg, ureg_t *ureg);


/** @brief Install the autostack handler. 
 *
 *  Pass the stack_high and stack_low to thread lib. Allocate memory
 *  for the exception stack. We put the exception stack in heap, so that
 *  the Legacy single-threaded stack can grow dynamically withou worrying
 *  about the exception stack. At last, we register the handler without
 *  specifying the ureg values.
 *
 *  @param stack_high The high addr of the stack for Legacy program.
 *  @param stack_low The low addr of the stack for Legacy program.
 *  @return Void
 */
void install_autostack(void *stack_high, void *stack_low) {
    void *exp_low;

    /* Check the input pointer */
    if(!stack_high || !stack_low){
        panic("input pointer is NULL");
    }
    else{
        /* store the high and low addr to thr lib */
        main_stk_hi = stack_high;
        main_stk_lo = stack_low;

        /* Store the aligned high and low addr of current stack */
        stack_high_aligned = PAGE_ROUNDUP(stack_high);
        stack_low_aligned = PAGE_ROUNDDN(stack_low);

        /* Allocate memory for exception stack */
        exp_low = _malloc(PAGE_SIZE*EXCP_STK_SIZE);
        if(!exp_low){
            panic("Failed to allocate the exception stack");
        }

        /* Calculate the esp3, which is one word higher than the 
         * exception stack*/
        esp3 = exp_low + PAGE_SIZE*EXCP_STK_SIZE;

        /* Register the handler without specifying the ureg values */
        if(swexn(esp3, auto_stack_handler, NULL, NULL) < 0){
            panic("Failed to register a software exception handler");
        }                   
    }
}

/** @brief Autostack handler. 
 *
 *  Pass the stack_high and stack_low to thread lib. Allocate memory
 *  for the exception stack. We put the exception stack in heap, so that
 *  the Legacy single-threaded stack can grow dynamically withou worrying
 *  about the exception stack. At last, we register the handler without
 *  specifying the ureg values.
 *
 *  @param arg The opaque argument. Not used here.
 *  @param ureg The reg values and cause of the fault. 
 *
 *  @return Void
 */
static void auto_stack_handler(void *arg, ureg_t *ureg){
    int size;
    void *new_stack_low;

    /* It's page fault. For auto-stack growth, it should be caused by 
     * a non-present page. And it should be executing in user mode. 
     * The memory access that caused the fault is outside the current
     * stack region. */
    if ((ureg->cause == SWEXN_CAUSE_PAGEFAULT) && 
       (BIT(ureg->error_code, 0) == 0) &&
       (BIT(ureg->error_code, 2) == 1) &&
       (ureg->cr2 < (unsigned int)stack_low_aligned)){

        /* Calculate the size of the current stack region */
        size = (unsigned int)stack_high_aligned -
               (unsigned int)stack_low_aligned;

        /* Double current stack size */
        new_stack_low = stack_low_aligned - size;
        if(new_pages(new_stack_low, size) < 0){
                panic("Failed to allocate the growth stack");
        }
        stack_low_aligned = new_stack_low;
    }

    /* Re-register the handler since kernel would de-register the 
     * handler after call it. Also, pass the ureg back to kernel
     * to retry the instruction that caused the fault. */
    if(swexn(esp3, auto_stack_handler, arg, ureg) < 0){
        panic("Failed to register a software exception handler");
    }
}
