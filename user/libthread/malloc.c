/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <mutex.h>
#include <thr_internals.h>

#define MY_DEBUG

#ifdef MY_DEBUG
#include <thread.h> /* Just for */
#include <libsimics/simics.h>
#include <syscall.h>
#endif

void *malloc(size_t __size)
{
    mutex_lock(&malloc_mp);
#ifdef MY_DEBUG
    lprintf("Utid %d, Ktid %d gets the malloc lock.\n", 
             thr_getid(), gettid());
#endif
    void *ret = _malloc(__size);
    mutex_unlock(&malloc_mp);
#ifdef MY_DEBUG
    lprintf("Utid %d, Ktid %d releases the malloc lock.\n", 
             thr_getid(), gettid());
#endif
    return ret;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
    mutex_lock(&malloc_mp);
    void *ret = _calloc(__nelt, __eltsize);
    mutex_unlock(&malloc_mp);
    return ret;
}

void *realloc(void *__buf, size_t __new_size)
{
    mutex_lock(&malloc_mp);
    void *ret = _realloc(__buf, __new_size);
    mutex_unlock(&malloc_mp);
    return ret;
}

void free(void *__buf)
{
    mutex_lock(&malloc_mp);
#ifdef MY_DEBUG
    lprintf("Utid %d, Ktid %d gets the free lock.\n", 
             thr_getid(), gettid());
#endif
    _free(__buf);
    mutex_unlock(&malloc_mp);
#ifdef MY_DEBUG
    lprintf("Utid %d, Ktid %d releases the free lock.\n", 
             thr_getid(), gettid());
#endif
    return;
}
