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

/** @brief Record the kernel tid of main(legacy) thread
 *
 *  This variable should be set when thr_init is called.
 *  The purpose is to retrieve the main_thr_stk differently than what we do for
 *  normal thread's thr_stk_t.
 *  The underlying assumption is that thr_init will only be called once by
 *  the main thread.
*/
static int main_thr_ktid;

/** @brief The size of a thread stack
 *
 * This variable should be set once when thr_init is called.
 * The value should be multiple of PAGE_SIZE.
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

/* -- Utilities for linked list -- */

/*  @note There are three utilities find/insert/remove.
 *        Only one thread thread can execute one of three
 *        function at a time.
 */

/** @brief Serach for utid in thread list.
 *
 *  Search if the utid is in the thread list.
 *
 *  @param utid The user thread id issued by libthread.
 *  @return The address of utid's header struct. If not found, return NULL.
 */
static thr_stk_t *thr_find(int utid) {
    /* begin critical section */
    mutex_lock(&thr_stk_list_mp);

    /* rraverse from head */
    thr_stk_t *curr_thr_stk = head;
    while (curr_thr_stk != NULL) {
        /* utid was found */
        if (curr_thr_stk->utid == utid) {
            /* dnd critical section */
            mutex_unlock(&thr_stk_list_mp);
            return curr_thr_stk;
        }
        curr_thr_stk = curr_thr_stk->next;
    }

    /* dnd critical section */
    mutex_unlock(&thr_stk_list_mp);
    /* utid was not found */
    return NULL;
}

/** @brief Insert utid to list.
 *
 *  @param thr_stk Address of thr_stk.
 *  @return -1 if the input thr_stk is NULL.
 *             else it should success and return 0.
 */
static int thr_insert(thr_stk_t *thr_stk) {
    /* begin critical section */
    mutex_lock(&thr_stk_list_mp);

    /* check pointer */
    if (thr_stk == NULL) {
        /* end critical section */
        mutex_unlock(&thr_stk_list_mp);
        return -1;
    }

    /* the thread should be set up at this point */
    assert(thr_stk->state != THR_UNAVAILABLE);

    /* no threads in the list */
    if (head == NULL) {
        head = thr_stk;
    }
    /* insert */
    else {
        /* set next */
        thr_stk->next = head->next;
        head->next = thr_stk;

        /* set prev */
        thr_stk->prev = head;
        if (thr_stk->next) {
            thr_stk->next->prev = thr_stk;
        }
    }

    /* end critical section */
    mutex_unlock(&thr_stk_list_mp);
    return 0;
}

/** @brief Delete utid's thr_stk from thread list.
 *
 *  @param thr_stk Address of thr_stk with utid.
 *  @return -1 thr_stk is empty, else success and return 0.
 *
*/
static int thr_remove(thr_stk_t *thr_stk) {
    /* begin critical section */
    mutex_lock(&thr_stk_list_mp);

    /* nuul pointer */
    if (thr_stk == NULL) {
        /* end critical section */
        mutex_unlock(&thr_stk_list_mp);
        return -1;
    }

    /* utid is the only element */
    if (thr_stk == head) {
        head = NULL;
        /* end critical section */
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
    /* end critical section */
    mutex_unlock(&thr_stk_list_mp);
    return 0;
}

/* -- Definitions -- */

/** @brief Perform thread fork, move chlld thread to the new stack
 *         and jmp eip
 *
 *  @param  ebp The address that ebp should store for child thread.
 *  @param  esp The address that esp should store for child thread.
 *  @return The id of child thread. -1 if thread creation failed.
 */
int thr_create_asm(void *ebp, void *esp);

/** @brief Allocate the size */
static void *stk_alloc(void *hi, int nbyte) {
    /* ensure stack is aligned to page (debug print) */
    if (PAGE_ROUNDDN(hi) != hi || (nbyte % PAGE_SIZE)) {
        panic("Requested stk is invalid. hi: %p nbyte: %d", hi, nbyte);
    }
    void *lo = hi - nbyte;
    /* return lo */
    if (new_pages(lo, nbyte) == 0) {
        return lo;
    } else {
        return 0;
    }
}

/** @brief Free a thread stack from page.
 *
 *  @note This function is only used by thr_join. Since it contains
 *        pairs of locks interact with thr_join that ensure the
 *        thr_join is thread-safe.
 *
 *  @param thr_stk The address of thr_stk, (the jointee thread).
 *  @return -1 if fail, else return 0 as success.
 */
static int _remove_stk_frame(thr_stk_t *thr_stk) {
    if (thr_stk == NULL) {
        return -1;
    }

    /* =lock the jointee (finer grain)*/
    mutex_lock(&thr_stk->mp);
    /* get the lower bound of thr_stk */
    void *stk_lo = (void *)thr_stk + sizeof(thr_stk_t) - stk_size;
    /* ===lock the thr_join operation */
    mutex_lock(&join_mp);
    /* =unlock the jointee (finer grain) */
    mutex_unlock(&thr_stk->mp);
    int status = remove_pages(stk_lo);
    /* ===unlock the thr_join operation */
    mutex_unlock(&join_mp);
    return status;
}

/** @brief Let the caller go to sleep before the target thread exit.
 *
 *  @param tid The target thread that caller is monitoring.
 *  @param statusp The pointer to the pointer of status struct.
 *  @return 0 if the target thread exited, else return -1.
*/

int thr_join(int tid, void **statusp) {
    /* ===lock the join operation */
    mutex_lock(&join_mp);

    /* check if tid is currently registered */
    thr_stk_t *thr_stk = thr_find(tid);
    if (thr_stk == NULL) {
        return -1;
    }

    /* =lock the target thr_stk */
    mutex_lock(&thr_stk->mp);
    /* ===unlock the join operation, so other threads can call join now */
    mutex_unlock(&join_mp);

    /* sleep untill target thread wake this thread up */
    while (thr_stk->state != THR_EXITED) {
        cond_wait(&thr_stk->cv, &thr_stk->mp);
    }

    /* if the thread is under join already, return -1 */
    if (thr_stk->join_flag) {
        return -1;
    }
    /* indicate that the target thread was called join if not.*/
    else {
        thr_stk->join_flag = 1;
    }

    /* retrieve the status */
    if (statusp != NULL) {
        *statusp = thr_stk->exit_status;
    }

    /* =unlock the target thr_stk */
    mutex_unlock(&thr_stk->mp);

    /* clean up the thr_stk, only one thread should do this! */
    if (thr_remove(thr_stk) != 0) {
        return -1;
    }
    if (_remove_stk_frame(thr_stk) != 0) {
        return -1;
    }

    return 0;
}

/** @brief Dispatch the child function and execute a thr_exit after it.
 *
 *  @param  func  The pointer to function.
 *  @param  args  This pointer to arguments.
 *  @return Void. This function should vanish before return.
 */
void thr_func_wrapper(void *(*func)(void *), void *args) {
#ifdef MY_DEBUG
    lprintf("Hello from func wrapper");
#endif
    thr_stk_t *thr_stk = get_thr_stk();

    /* wait until the ktid field is set by the creation thread */
    mutex_lock(&fork_mp);
    /* initial value is 0 */
    while (thr_stk->ktid == 0) {
        cond_wait(&fork_cv, &fork_mp);
    }
    mutex_unlock(&fork_mp);

    void *ret_val = func(args);

#ifdef MY_DEBUG
    lprintf("child returned with (int)ret_val: %d", (int)ret_val);
#endif

    /* if func didn't call thr_exit, it will reach here */
    thr_stk->state = THR_EXITED;
    thr_exit(ret_val);

    /* should never reach here */
    panic("thread should've exited.");
    return;
}

/** @brief Let the scheduler to run other thread.
 *
 *  This function simply map the utid to ktid and delegate
 *  the job to system call yield.
 *
 *  @param tid The target thread id to yield. -1 if unspecify.
 *  @return -1 if the target thread doesn't exist.
 */
int thr_yield(int tid) {
    /* Yield to unspecified thread */
    if (tid == -1)
        return yield(-1);
    else {
        /* Find the thr_stk head using the utid */
        thr_stk_t *thr_stk = thr_find(tid);
        /* The thr_stk does not exist */
        if (!thr_stk) {
            panic("The utid %d does not exist. \n", tid);
            return -1;
        } else {
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

/** @brief Set the status and vanish.
 *
 *  The caller will post a status, onto the thr_stk, waiting for
 *  thr_join to collect the status.
 *
 *  @param status The pointer to an arbitrary structure defined by user program.
 *  @return Void.
 */
void thr_exit(void *status) {
    thr_stk_t *thr_stk = get_thr_stk();
    /* when this function is called,
     * the thread stack shouldn't be removed yet */
    assert(thr_stk != NULL);

    /* lock this thr_stk since we are going to write it */
    mutex_lock(&thr_stk->mp);

    /* store status into the internal struct */
    thr_stk->exit_status = status;
    /* mark thread as exit so that joiner can see */
    thr_stk->state = THR_EXITED;
    /* there might be multiple joiner try to join on this thread at
       this point, signal to all of them, but only one of them will
       get the chance to join this thread.ß */
    cond_broadcast(&thr_stk->cv);

    /* release the lock */
    mutex_unlock(&thr_stk->mp);

    vanish();
}

/** @brief Setup the fields in thr_stk
 *
 *  @param thr_stk_lo The lowest address of thr_stk.
 *  @param args The address of arguments.
 *  @param funct The address of function.
*/

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
    /* The thread shuoldn't be used before this state is cleared */
    thr_stk->state = THR_UNAVAILABLE;
    thr_stk->zero = 0;
    thr_stk->join_flag = 0;

    mutex_init(&thr_stk->mp);
    cond_init(&thr_stk->cv);

#ifdef MY_DEBUG
    lprintf("utid: %d was issued", thr_stk->utid);
    lprintf("utid addr: %p was issued", &thr_stk->utid);
#endif

    return thr_stk;
}

/** @brief Initiaize the multi-thread stack boundaries
 *
 *  Initialize the internal variable for libthread.
 *
 *  @assumption Only "main" thread will call this function once.
 *  @param size The requested stack size.
 *  @note: We will round up the requested size, so internally there will be less
 *         exterenal fragmentation on the stack.
 *  @return -1 if fail, 0 if success.s
 */
int thr_init(unsigned int size) {
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
        if (_remove_stk_frame(thr_stk) < 0) {
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
