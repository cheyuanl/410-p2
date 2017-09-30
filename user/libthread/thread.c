#include <simics.h>
#include <stdlib.h>
#include <syscall.h>
#include <thr_internals.h>
#include <thread.h>

#ifndef PAGE_ALIGN_MASK
#define PAGE_ALIGN_MASK ((unsigned int)~((unsigned int)(PAGE_SIZE - 1)))
#endif

#define PAGE_ROUNDUP(p) \
    (void *)(PAGE_ALIGN_MASK & (((unsigned int)p) + PAGE_SIZE - 1))
#define PAGE_ROUNDDN(p) (void *)(PAGE_ALIGN_MASK & ((unsigned int)p))

/* --- Define private fields for libthread --- */

/** @brief The next tid to be issued */
static int global_tid = 0;

/** @brief The size of a thread stack */
static int stk_size;

/** @brief The head of linked-list */
// static thr_stk_t *head = NULL;

/* --- Function definition --- */

void *stk_alloc(void *hi, int nbyte) {
    /* ensure stack is aligned to page (debug print) */
    if (PAGE_ROUNDDN(hi) != hi || (nbyte % PAGE_SIZE)) {
        lprintf("Requested stk is invalid. hi: %p nbyte: %d", hi, nbyte);
    }
    void *lo = hi - nbyte;
    /* return lo */
    if (new_pages(lo, nbyte) == 0) {
        return lo;
    } else {
        return 0;
    }
}

/** @brief collect the garbage thread */
/** TODO this part is basically broken! */
void thr_gc() {
    /** TODO: find the thr_stk_t and free it */
    lprintf("thr_gc was called");

    thr_stk_t *thr_stk = get_thr_stk();
    void* stk_lo = (void*)thr_stk+sizeof(thr_stk_t)-stk_size;
    lprintf("thr_stk: %p", thr_stk);
    lprintf("free: %p", stk_lo);
    free(stk_lo);

    int ret_from_child = get_eax();
    lprintf("set status to %d", ret_from_child);
    MAGIC_BREAK;

    set_status(ret_from_child);
    vanish();

    return;
}

thr_stk_t *install_stk_header(void *thr_stk_lo, void *args, void *ret_addr) {
    void *hi = thr_stk_lo + stk_size;
    thr_stk_t *thr_stk = hi - sizeof(thr_stk_t);

    /* note: esp should be aligned */
    /* fill in header from low addr to high addr */
    thr_stk->ret_addr = ret_addr; /* TODO: where? */
    thr_stk->args = args;
    thr_stk->cv_next = 0x0;
    thr_stk->next = 0x0;
    thr_stk->prev = 0x0;
    thr_stk->tid = global_tid++;
    thr_stk->zero = 0;

    lprintf("tid: %d was issued", thr_stk->tid);

    return thr_stk;
}

/** @brief Initiaize the multi-thread stack boundaries */
int thr_init(unsigned int size) {
    /* NOTE: if this method was called, it means the program
     *       is a multi-thread program, so it shouldn't support
     *       autostack growth.
     *
     * TODO: carefully examiane the sixe
     * TODO: uninstall the auto stack growth? */

    lprintf("thr_init was called");
    lprintf("requested stack size: %d", size);

    /* round-up thread stack size to page size */
    stk_size = (int)PAGE_ROUNDUP(size + sizeof(thr_stk_t));
    lprintf("stk_size was set to %d", stk_size);

    /* set the head of thread stack to be slightly lower then main_stk_lo */
    thr_stk_head = PAGE_ROUNDDN(main_stk_lo);
    lprintf("stk_head was set to %p", thr_stk_head);

    /* set the candidate address to allocate the stack */
    thr_stk_curr = thr_stk_head;

    MAGIC_BREAK;

    return 0;
}

int thr_create(void *(*func)(void *), void *args) {
    /* request a memory block starting from where ? */
    /* TODO: currently the thread stack lives on heap,
     *       since I don't konw how to find a chunck of
     *       memory on stack...
     */
    lprintf("Allocating space for thread stack");
    void *thr_stk_lo = stk_alloc(thr_stk_curr, stk_size);
    lprintf("stk_alloc: %p", thr_stk_lo);
    if (thr_stk_lo == NULL) {
        return -1;
    }

    /* move thr_stk_curr down */
    thr_stk_curr -= stk_size;

    thr_stk_t *thr_stk = install_stk_header(thr_stk_lo, args, thr_gc);

    /* NOTE: a thread newly created by thread fork has no software exception
     *       handler registered */
    /* NOTE: at this point, child shouldn't alter the stack... or parent
     * should
     * do
     *       nothing, since the stack is corrupted from parents point of
     * view.
     */
    lprintf("ebp addr %p", &thr_stk->args);
    lprintf("esp addr %p", &thr_stk->ret_addr);
    lprintf("func addr %p", func);
    lprintf("ret_addr %p", thr_stk->ret_addr);
    lprintf("gc: %p", thr_gc);

    //    MAGIC_BREAK;

    int ret = thr_create_asm(&thr_stk->zero, &thr_stk->ret_addr, func);
    lprintf("thr_create_asm return: %p", (void *)ret);

    //    MAGIC_BREAK;

    /* error. no thread created */;
    if (ret == -1) {
        free(thr_stk);
        return -1;
    }

    lprintf("Hello from parent");

    return ret;
}

/** TODO: this function call is quiet inefficient */
thr_stk_t *get_thr_stk() {
    int *curr_ebp = get_ebp();
    while (*curr_ebp != (int)NULL) {
        curr_ebp = *(int **)(curr_ebp);
    }

    return (thr_stk_t *)curr_ebp;
}

int thr_getid() {
    thr_stk_t *thr_stk = get_thr_stk();
    int my_tid = thr_stk->tid;
    lprintf("tid = %d", my_tid);

//    MAGIC_BREAK;

    return my_tid;
}
