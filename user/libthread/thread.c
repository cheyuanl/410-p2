/** @file thread.c
 *  @brief The thread library.
 *
 *  This contains the variables, data structure methods,
 *  and implementation of API for libthread.
 *
 *  @author Che-Yuan Liang (cheyuanl)
 *  @bug No known bugs.
 */

#include <assert.h>
#include <cond.h>
#include <mutex.h>
#include <simics.h> /* lprintf() */
#include <string.h> /* memset() */
#include <syscall.h>
#include <thr_internals.h>
#include <thread.h>

#define MY_DEBUGx

/** @brief Round down the bits to page size */
#define PAGE_ALIGN_MASK ((unsigned int)~((unsigned int)(PAGE_SIZE - 1)))
/** @brief Round up the bits to page size */
#define PAGE_ROUNDUP(p) \
    (void *)(PAGE_ALIGN_MASK & (((unsigned int)p) + PAGE_SIZE - 1))
/** @brief Round down an address to page size and cast it to pointer type */ 
#define PAGE_ROUNDDN(p) (void *)(PAGE_ALIGN_MASK & ((unsigned int)p))
/** @brief Define NULL */
#define NULL 0

/* -- Private defintions -- */

/** @brief The next utid to be issued */
static int global_utid = 0;

/** @brief The size of a thread stack 
 * 
 * This variable should be set once thr_init is called. The value should 
 * be multiple of PAGE_SIZE.
 */

static int stk_size;

/** @brief The head of linked-list pointing to threads being created. */
static thr_stk_t *head = NULL;

/** @brief Lock to sync thr_create and the newly created child thread */
static mutex_t fork_mp;

/** @brief CV to sync thr_create and the newly created child thread  */
static cond_t fork_cv;

/** @brief Lock to make sure thr_create is sequential */
static mutex_t create_mp;

/** @brief Lock to sync thread linked-list operation */
static mutex_t thr_stk_list_mp;

/** @breif Lock for join operation */
static mutex_t join_mp;

/** @brief The thread stack for main(legacy) thread 
 * 
 *  In our thread library, we assume each thread has a header structure, 
 *  thr_stk_t in order to manage the threads. Since the legacy programs 
 *  don't have this header, we reserve this field for the main thread, 
 *  making it compatible to the our thread management data structure.
 *  
 */
thr_stk_t main_thr_stk;

/** @brief Record the kernel tid of main(legacy) thread 
 *  
 *  This variable should be set when thr_init is called.
 *  The purpose is to retrieve the main_thr_stk differently than what we do for
 *  normal thread's thr_stk_t. 
 *  The underlying assumption is that thr_init will only be called once by 
 *  the main thread.
*/
int main_thr_ktid;


/* -- utility for linked list -- */

/** @brief Serach for utid in thread list. 
 * 
 *  This search if the utid is created and yet been destroyed.
 * 
 *  @param utid The user thread id.
 *  @return The address of utid's header struct. If not found, return NULL. 
 */
static thr_stk_t *thr_find(int utid) {
    /* Begin critical section */
    mutex_lock(&thr_stk_list_mp);

    /* Start traversal from the head */
    thr_stk_t *curr_thr_stk = head;
    while (curr_thr_stk != NULL) {
        /* Found utid */
        if (curr_thr_stk->utid == utid) {
            /* End critical section */
            mutex_unlock(&thr_stk_list_mp);
            return curr_thr_stk;
        }
        curr_thr_stk = curr_thr_stk->next;
    }

    /* End critical section */
    mutex_unlock(&thr_stk_list_mp);
    /* utid is not found */
    return NULL;
}

/** @brief insert utid into thread list */
static int thr_insert(thr_stk_t *thr_stk) {
    mutex_lock(&thr_stk_list_mp);
    /* check validity */
    if (thr_stk == NULL) {
        mutex_unlock(&thr_stk_list_mp);
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
        if (thr_stk->next) {
            thr_stk->next->prev = thr_stk;
        }
    }
    mutex_unlock(&thr_stk_list_mp);
    return 0;
}

/** @brief delete utid from thread list */
static int thr_remove(thr_stk_t *thr_stk) {
    mutex_lock(&thr_stk_list_mp);
    /* utid is not found */
    if (thr_stk == NULL) {
        mutex_unlock(&thr_stk_list_mp);
        return -1;
    }

    /* utid is the only element */
    if (thr_stk == head) {
        head = NULL;
        mutex_unlock(&thr_stk_list_mp);
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
    mutex_unlock(&thr_stk_list_mp);
    return 0;
}

/** @brief free  the thread stack */
int remove_stk_frame(thr_stk_t *thr_stk) {
    mutex_lock(&thr_stk->mp);
    void *stk_lo = (void *)thr_stk + sizeof(thr_stk_t) - stk_size;
    mutex_lock(&join_mp);
    mutex_unlock(&thr_stk->mp);
    int status = remove_pages(stk_lo);
    mutex_unlock(&join_mp);
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
    mutex_lock(&join_mp);

    /* check if tid is currently in running */
    thr_stk_t *thr_stk = thr_find(tid);
    if (thr_stk == NULL) {
        return -1;
    }

    mutex_lock(&thr_stk->mp);
    mutex_unlock(&join_mp);

    /* TODO: conditional wait */
    while (thr_stk->state != THR_EXITED) {
        cond_wait(&thr_stk->cv, &thr_stk->mp);
    }

    if (thr_stk->join) {
        return -1;
    } else {
        thr_stk->join = 1;
    }

    /* st status */
    if (statusp != NULL) *statusp = thr_stk->exit_status;

    mutex_unlock(&thr_stk->mp);

    /* clean up */
    /* TOOD: better error handling, haven't think through */
    if (thr_remove(thr_stk) != 0) {
        return -1;
    }
    if (remove_stk_frame(thr_stk) != 0) {
        return -1;
    }

    return 0;
}

/** @brief collect the garbage thread */
void thr_func_wrapper(void *(*func)(void *), void *args) {
#ifdef MY_DEBUG
    lprintf("Hello from func wrapper");
#endif
    thr_stk_t *thr_stk = get_thr_stk();

    /* wait until the ktid field is set by the creation thread */
    mutex_lock(&fork_mp);
    while (thr_stk->ktid == 0) /* initial value is 0 */
        cond_wait(&fork_cv, &fork_mp);
    mutex_unlock(&fork_mp);

    /** begin critical section? **/
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

int thr_yield(int tid) {
    /* Yield to unspecified thread */
    if (tid == -1)
        return yield(-1);
    else {
        /* Find the thr_stk head using the utid */
        thr_stk_t *thr_stk = thr_find(tid);
        /* The thr_stk does not exist */
        if (!thr_stk) {
            lprintf("The utid %d does not exist. \n", tid);
            return -1;
        }
        /* The specified thread is not runnable. */
        /*
                else if (thr_stk->state != THR_RUNNABLE) {
                    lprintf("The utid %d is not runnable. \n", tid);
                    return -1;
                }
        */
        else {
#ifdef MY_DEBUG
            lprintf("thr->utid = %d, thr->ktid = %d \n", thr_stk->utid,
                    thr_stk->ktid);
            lprintf("thr_stk addr = %p \n", thr_stk);
            MAGIC_BREAK;
#endif
            return yield(thr_stk->ktid);
        }
    }
}

void thr_exit(void *status) {
    thr_stk_t *thr_stk = get_thr_stk();
    assert(thr_stk != NULL);

    mutex_lock(&thr_stk->mp);

    /* store status into internal struct */
    thr_stk->exit_status = status;
    thr_stk->state = THR_EXITED;
    cond_broadcast(&thr_stk->cv);

    mutex_unlock(&thr_stk->mp);
    vanish();
}

thr_stk_t *install_stk_header(void *thr_stk_lo, void *args, void *func) {
    void *hi = thr_stk_lo + stk_size;
    thr_stk_t *thr_stk = hi - sizeof(thr_stk_t);

    /* fill in header from low addr to high addr */
    /* note: esp should be aligned to 4 */
    assert((int)thr_stk->ret_addr % 4 == 0);
    thr_stk->ret_addr = vanish;
    thr_stk->func = func;
    thr_stk->args = args;
    thr_stk->cv_next = NULL;
    thr_stk->next = NULL;
    thr_stk->prev = NULL;
    thr_stk->utid = global_utid;
    /* Initialize kernel assigned ID as 0 */
    thr_stk->ktid = 0;
    thr_stk->state = THR_UNAVAILABLE;
    thr_stk->zero = 0;
    thr_stk->join = 0;

    mutex_init(&thr_stk->mp);
    cond_init(&thr_stk->cv);

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
    main_thr_stk.state = THR_UNAVAILABLE;

    /* Initialize the malloc lock. Malloc family would not be called
     * before thr_init. */
    mutex_init(&malloc_mp);

    /* The whole process of thr_create should be serial, we do not
     * want any part of it to be interleaved with another thr_create,
     * since thr_create involves so many global operations */
    mutex_init(&create_mp);

    /* Initialize the lock and cv for thr_create. Let's say T1 creates
     * T2. After thr_create, T2 has to wait until its ktid is installed
     * by T1.  */
    mutex_init(&fork_mp);
    cond_init(&fork_cv);

    /* Initialize the main thread's private lock and cv */
    mutex_init(&main_thr_stk.mp);
    cond_init(&main_thr_stk.cv);

    /* Initialize the mutex for thread-list */
    mutex_init(&thr_stk_list_mp);

    /* Initialize the mutex for thr_join */
    mutex_init(&join_mp);

    // MAGIC_BREAK;

    /* add main thread to thread list */
    if (thr_insert(&main_thr_stk) < 0) {
        return -1;
    }

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
    mutex_lock(&create_mp);
    void *thr_stk_lo = stk_alloc(thr_stk_curr, stk_size);
#ifdef MY_DEBUG
    lprintf("stk_alloc: %p", thr_stk_lo);
#endif
    if (thr_stk_lo == NULL) {
        return -1;
    }

    /* move thr_stk_curr down */
    thr_stk_curr -= stk_size;

    thr_stk_t *thr_stk = install_stk_header(thr_stk_lo, args, (void *)func);

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

    int ret = thr_create_asm(&thr_stk->zero, &thr_stk->ret_addr);
    /* Store the ktid back to the newly created thread */
    mutex_lock(&fork_mp);
    thr_stk->ktid = ret;
    cond_signal(&fork_cv);
    mutex_unlock(&fork_mp);

#ifdef MY_DEBUG
    lprintf("thr_create_asm return: %p", (void *)ret);
#endif

    //    MAGIC_BREAK;

    /* error. no thread created */;
    if (ret == -1) {
        if (remove_stk_frame(thr_stk) < 0) {
            lprintf("warning! remove stk frame failed");
        }
        return -1;
    }

#ifdef MY_DEBUG
    lprintf("Hello from parent");
#endif

    if (thr_insert(thr_stk) < 0) {
        return -1;
    }

    int ret_utid = global_utid++;
    mutex_unlock(&create_mp);

    return ret_utid;
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
