/** @file handler.c
 *  @brief Handle the fault for multi-threaded program
 *
 *  If there is a fault in multi-threaded program, the whole application
 *  is not likely to work properly. So we will kill the whole program
 *  instead of just one thread. 
 *
 *  Since the multi-threaded program will not support auto-stack growth,
 *  we just re-register the handler to overwrite the auto-stack handler
 *  for legacy code. 
 *
 *  @author Zhipeng Zhao (zzhao1)
 *  @bug No known bugs.
 */
#include <stdio.h>
#include <libsimics/simics.h>
#include <thr_internals.h>
#include <syscall.h>
#include <assert.h>
#include <excp_handler.h>

void excp_handler(void *arg, ureg_t *ureg);

/** @brief Install the handler. 
 *
 *  Just register a handler through swexn.
 *
 *  @return Void
 */
void install_handler() {
    /* Register the handler without specifying the ureg values */
    if(swexn(esp3, excp_handler, NULL, NULL) < 0){
        panic("Failed to register a software exception handler");
    }                   
}

/** @brief Exception handler. 
 *
 *  When meeting a fatal error, we will terminate the whole task instead
 *  of single thread for a multi-threaded program. Before that, we print
 *  the register values, the cause of the error, the thread ID.
 *
 *  @param arg The opaque argument. Not used here.
 *  @param ureg The reg values and cause of the fault. 
 *
 *  @return Void
 */
void excp_handler(void *arg, ureg_t *ureg){
    /* Decode the cause */
    switch(ureg->cause){
        case SWEXN_CAUSE_DIVIDE:
            printf("Divide Error Exception\n"); 
            lprintf("Divide Error Exception"); 
            break;
        case SWEXN_CAUSE_DEBUG:
            printf("Debug Exception\n"); 
            lprintf("Debug Exception"); 
            break;
        case SWEXN_CAUSE_BREAKPOINT:
            printf("Breakpoint Exception\n"); 
            lprintf("Breakpoint Exception"); 
            break;
        case SWEXN_CAUSE_OVERFLOW:
            printf("Overflow Exception\n"); 
            lprintf("Overflow Exception"); 
            break;
        case SWEXN_CAUSE_BOUNDCHECK:
            printf("BOUND Range Exceeded Exception\n"); 
            lprintf("BOUND Range Exceeded Exception"); 
            break;
        case SWEXN_CAUSE_OPCODE:
            printf("Invalid Opcode Exception\n"); 
            lprintf("Invalid Opcode Exception"); 
            break;
        case SWEXN_CAUSE_NOFPU:
            printf("Device Not Available Exception\n"); 
            lprintf("Device Not Available Exception"); 
            break;
        case SWEXN_CAUSE_SEGFAULT:
            printf("Segment Not Present\n"); 
            lprintf("Segment Not Present"); 
            break;
        case SWEXN_CAUSE_STACKFAULT:
            printf("Stack Fault Exception\n"); 
            lprintf("Stack Fault Exception"); 
            break;
        case SWEXN_CAUSE_PROTFAULT:
            printf("General Protection Exception\n"); 
            lprintf("General Protection Exception"); 
            break;
        case SWEXN_CAUSE_PAGEFAULT:
            printf("Page-Fault Exception\n"); 
            printf("Invalid memory access at 0x%08x \n",ureg->cr2);
            lprintf("Page-Fault Exception"); 
            printf("Invalid memory access at 0x%08x",ureg->cr2);
            break;
        case SWEXN_CAUSE_FPUFAULT:
            printf("x87 FPU Floating-Point Error\n"); 
            lprintf("x87 FPU Floating-Point Error"); 
            break;
        case SWEXN_CAUSE_ALIGNFAULT:
            printf("Alignment Check Exception\n"); 
            lprintf("Alignment Check Exception"); 
            break;
        case SWEXN_CAUSE_SIMDFAULT:
            printf("SIMD Floating-Point Exception\n"); 
            lprintf("SIMD Floating-Point Exception"); 
            break;
        default:
            printf("Invalid Exception Value\n"); 
            lprintf("Invalid Exception Value"); 
    }

    /* Print the register values */
    printf("Thread: %d \n", gettid());
    printf("Registers:\n");
    printf("eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x,\n",
           ureg->eax, ureg->ebx, ureg->ecx);
    printf("edx: 0x%08x, edi: 0x%08x, esi: 0x%08x,\n",
           ureg->edx, ureg->edi, ureg->esi);
    printf("ebp: 0x%08x, esp: 0x%08x, eip: 0x%08x,\n",
           ureg->ebp, ureg->esp, ureg->eip);
    printf(" ss:     0x%04x,  cs:     0x%04x, "
           " ds:     0x%04x,\n",
           ureg->ss, ureg->cs, ureg->ds);
    printf(" es:     0x%04x,  fs:     0x%04x, "
           " gs:     0x%04x,\n",
           ureg->es, ureg->fs, ureg->gs);
    printf("eflags = 0x%08x \n",ureg->eflags);
    printf("error_code = 0x%08x \n",ureg->error_code);

    lprintf("Thread: %d", gettid());
    lprintf("Registers:");
    lprintf("eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x,",
           ureg->eax, ureg->ebx, ureg->ecx);
    lprintf("edx: 0x%08x, edi: 0x%08x, esi: 0x%08x,",
           ureg->edx, ureg->edi, ureg->esi);
    lprintf("ebp: 0x%08x, esp: 0x%08x, eip: 0x%08x,",
           ureg->ebp, ureg->esp, ureg->eip);
    lprintf(" ss:     0x%04x,  cs:     0x%04x, "
           " ds:     0x%04x,",
           ureg->ss, ureg->cs, ureg->ds);
    lprintf(" es:     0x%04x,  fs:     0x%04x, "
           " gs:     0x%04x,",
           ureg->es, ureg->fs, ureg->gs);
    lprintf("eflags = 0x%08x",ureg->eflags);
    lprintf("error_code = 0x%08x",ureg->error_code);

    /* Exit the task */
    task_vanish(-1);
}

