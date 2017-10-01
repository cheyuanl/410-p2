#include <assert.h>
#include <cond.h>
#include <mutex.h>
#include <simics.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <thr_internals.h>
#include <thread.h>

#define MY_DEBUG

#ifndef PAGE_ALIGN_MASK
#define PAGE_ALIGN_MASK ((unsigned int)~((unsigned int)(PAGE_SIZE - 1)))
#endif

#define PAGE_ROUNDUP(p) \
    (void *)(PAGE_ALIGN_MASK & (((unsigned int)p) + PAGE_SIZE - 1))
#define PAGE_ROUNDDN(p) (void *)(PAGE_ALIGN_MASK & ((unsigned int)p))

/* --- Define private fields for libthread --- */

/** @brief The next utid to be issued */
static int global_utid = 0;

/** @brief The size of a thread stack */
static int stk_size;

/** @brief The head of linked-list */
static thr_stk_t *head = NULL;

/** @brief The thread stack for main(legacy) thread */
thr_stk_t main_thr_stk;

/** @brief Record the kernel tid of main(legacy) thread */
int main_thr_ktid;

/* --- linked-list utility ---- */

/** @brief serach for utid in thread list */
thr_stk_t *thr_find(int utid) {
    thr_stk_t *curr_thr_stk = head;
    while (curr_thr_stk != NULL) {
        if (curr_thr_stk->utid == utid) {
            return curr_thr_stk;
        }
    }
    return NULL;
}

/** @brief insert utid into thread list */
int thr_insert(thr_stk_t *thr_stk) {
    /* check validity */
    if (thr_stk == NULL) {
        return -1;
    }

    /* empty thread list */
    assert(thr_stk->state == THR_UNAVAILABLE);
    if (head == NULL) {
        head = thr_stk;
    } else {
        /* set next */
        thr_stk->next = head->next;
        head->next = thr_stk;

        /* set prev */
        thr_stk->prev = head;
        thr_stk->next->prev = thr_stk;
    }
    return 0;
}

/** @brief delete utid from thread list */
int thr_remove(thr_stk_t *thr_stk) {
    /* utid is not found */
    if (thr_stk == NULL) {
        return -1;
    }

    /* utid is the only element */
    if (thr_stk == head) {
        head = NULL;
        return 0;
    }

    /* utid is the first element in list */
    if (thr_stk->prev == NULL) {
        head = thr_stk->next;
        if (thr_stk->next) {
            thr_stk->next->prev = NULL;
        }
    } else {
        thr_stk->prev->next = thr_stk->next;
        if (thr_stk->next) {
            thr_stk->next->prev = thr_stk->prev;
        }
    }

    return 0;
}

/** @brief free  the thread stack */
int remove_stk_frame(thr_stk_t *thr_stk) {
    void *stk_lo = (void *)thr_stk + sizeof(thr_stk_t) - stk_size;
    int status = remove_pages(stk_lo);
    return status;
}

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

int thr_join(int tid, void **statusp) {
    /* check if tid is currently in running */
    thr_stk_t *thr_stk = thr_find(tid);
    if (thr_stk == NULL) {
        return -1;
    }

    /* TODO: conditional wait */

    /* set status */
    *statusp = thr_stk->exit_status;

    /* clean up */
    /* TOOD: better error handling, haven't think through */
    if (thr_remove(thr_stk) != 0 || remove_stk_frame(thr_stk) != 0) {
        return -1;
    }

    return 0;
}

/** @brief collect the garbage thread */
void thr_func_wrapper(void *(*func)(void *), void *args) {
    thr_stk_t *thr_stk = get_thr_stk();

    /** begin critical section? **/
    thr_stk->state = THR_RUNNING;
    void *ret_val = func(args);
/** end   critical section? **/

#ifdef MY_DEBUG
    lprintf("child returned with (int)ret_val: %d", (int)ret_val);
#endif

    /** if func never call thr_exit, it will reach here */
    /** begin critical section? **/
    thr_stk->state = THR_EXITED;
    thr_exit(ret_val);
    /** end   critical section? **/

    /* should never reach here */
    assert(0);
    return;
}

void thr_exit(void *status) {
    thr_stk_t *thr_stk = get_thr_stk();
    assert(thr_stk != NULL);

    /* store status into internal struct */
    thr_stk->exit_status = status;
    vanish();
}

thr_stk_t *install_stk_header(void *thr_stk_lo, void *args, void *ret_addr) {
    void *hi = thr_stk_lo + stk_size;
    thr_stk_t *thr_stk = hi - sizeof(thr_stk_t);

    /* fill in header from low addr to high addr */
    /* note: esp should be aligned to 4 */
    assert((int)thr_stk->ret_addr % 4 == 0);
    thr_stk->ret_addr = ret_addr;
    thr_stk->args = args;
    thr_stk->cv_next = NULL;
    thr_stk->next = NULL;
    thr_stk->prev = NULL;
    thr_stk->utid = global_utid++;
    thr_stk->state = THR_UNAVAILABLE;
    thr_stk->zero = 0;

#ifdef MY_DEBUG
    lprintf("utid: %d was issued", thr_stk->utid);
    lprintf("utid addr: %p was issued", &thr_stk->utid);
#endif

    return thr_stk;
}

/** @brief Initiaize the multi-thread stack boundaries */
/** Assumption: only "main" thread will call this function. */
int thr_init(unsigned int size) {
/* NOTE: if this method was called, it means the program
 *       is a multi-thread program, so it shouldn't support
 *       autostack growth.
 *
 * TODO: carefully examiane the sixe
 * TODO: uninstall the auto stack growth? */

#ifdef MY_DEBUG
    lprintf("thr_init was called");
    lprintf("requested stack size: %d", size);
#endif

    /* round-up thread stack size to page size */
    stk_size = (int)PAGE_ROUNDUP(size + sizeof(thr_stk_t));
#ifdef MY_DEBUG
    lprintf("stk_size was set to %d", stk_size);
#endif

    /* set the head of thread stack to be slightly lower then main_stk_lo */
    thr_stk_head = PAGE_ROUNDDN(main_stk_lo);
#ifdef MY_DEBUG
    lprintf("stk_head was set to %p", thr_stk_head);
#endif

    /* set the candidate address to allocate the stack */
    thr_stk_curr = thr_stk_head;

#ifdef MY_DEBUG
    lprintf("initializing main_thr_stk");
#endif
    memset(&main_thr_stk, 0, sizeof(thr_stk_t));
    main_thr_stk.utid = global_utid++; /* main always get uid = 0 */
    main_thr_stk.ktid = gettid();
    main_thr_ktid = main_thr_stk.ktid;

    /* Initialize the malloc lock. Malloc family would not be called
     * before thr_init. */
    mutex_init(&malloc_mp);
    // MAGIC_BREAK;

    return 0;
}

int thr_create(void *(*func)(void *), void *args) {
/* request a memory block starting from where ? */
/* TODO: currently the thread stack lives on heap,
 *       since I don't konw how to find a chunck of
 *       memory on stack...
 */
#ifdef MY_DEBUG
    lprintf("Allocating space for thread stack");
#endif
    void *thr_stk_lo = stk_alloc(thr_stk_curr, stk_size);
#ifdef MY_DEBUG
    lprintf("stk_alloc: %p", thr_stk_lo);
#endif
    if (thr_stk_lo == NULL) {
        return -1;
    }

    /* move thr_stk_curr down */
    thr_stk_curr -= stk_size;

    thr_stk_t *thr_stk = install_stk_header(thr_stk_lo, args, thr_exit);

/* NOTE: a thread newly created by thread fork has no software exception
 *       handler registered */
/* NOTE: at this point, child shouldn't alter the stack... or parent
 * should
 * do
 *       nothing, since the stack is corrupted from parents point of
 * view.
 */
#ifdef MY_DEBUG
    lprintf("thr_stk addr %p", thr_stk);
    lprintf("ebp addr %p", &thr_stk->zero);
    lprintf("esp addr %p", &thr_stk->ret_addr);
    lprintf("func addr %p", func);
    lprintf("ret_addr %p", thr_stk->ret_addr);
#endif
    //    MAGIC_BREAK;

    int ret = thr_create_asm(&thr_stk->zero, &thr_stk->ret_addr, &thr_stk->ktid,
                             func);
#ifdef MY_DEBUG
    lprintf("thr_create_asm return: %p", (void *)ret);
#endif

    //    MAGIC_BREAK;

    /* error. no thread created */;
    if (ret == -1) {
        free(thr_stk);
        return -1;
    }

#ifdef MY_DEBUG
    lprintf("Hello from parent");
#endif

    return ret;
}

/** TODO: this function call is quiet inefficient */
thr_stk_t *get_thr_stk() {
    /* quick hack */
    if (gettid() == main_thr_ktid) {
        return &main_thr_stk;
    }

    int *curr_ebp = get_ebp();
    while (*curr_ebp != (int)NULL) {
        curr_ebp = *(int **)(curr_ebp);
    }

    /* Return the starting addr of thr_stk head. Which is
     * thr_stk->ret_addr. Add one int entry to complement the curr_ebp
     * size, which is the thr_stk->zero. */
    curr_ebp = curr_ebp + 1 - sizeof(thr_stk_t) / sizeof(int);

    return (thr_stk_t *)curr_ebp;
}

int thr_getid() {
    /* quick hack */
    if (gettid() == main_thr_ktid) {
        return main_thr_stk.utid;
    }

    thr_stk_t *thr_stk = get_thr_stk();
    int my_utid = thr_stk->utid;
    // lprintf("utid = %d", my_utid);

    return my_utid;
}
