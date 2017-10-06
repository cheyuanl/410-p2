/** @file malloc.c
 *  @brief Thread-safe malloc family wrapper.
 *
 *  To make the malloc thread-safe, we just put a hammer there,
 *  a global mutex that initialized at thr_init() and lock the
 *  malloc methods. Since, malloc touches Heap which is shared
 *  by all the threads, we just lock these methods, so that at
 *  one time only one thread can manipulate on Heap. 
 *
 *  @author Zhipeng Zhao (zzhao1)
 *  @bug No known bugs.
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <mutex.h>
#include <thr_internals.h>

/** @brief Malloc wrapper.
 *
 *  @param __size The request memory size in bytes.
 *  @return malloc's return value
 **/
void *malloc(size_t __size)
{
    mutex_lock(&malloc_mp);
    void *ret = _malloc(__size);
    mutex_unlock(&malloc_mp);
    return ret;
}

/** @brief Calloc wrapper.
 *
 *  @param __nelt The number of elements to be allocated
 *  @param __eltsize The size of elements
 *  @return calloc's return value
 **/
void *calloc(size_t __nelt, size_t __eltsize)
{
    mutex_lock(&malloc_mp);
    void *ret = _calloc(__nelt, __eltsize);
    mutex_unlock(&malloc_mp);
    return ret;
}


/** @brief Realloc wrapper.
 *
 *  @param __buf The memory that to be reallocated
 *  @param __new_size The new size for the memory block
 *  @return realloc's return value
 **/
void *realloc(void *__buf, size_t __new_size)
{
    mutex_lock(&malloc_mp);
    void *ret = _realloc(__buf, __new_size);
    mutex_unlock(&malloc_mp);
    return ret;
}


/** @brief Free wrapper.
 *
 *  @param __buf The memory that to be freed
 *  @return void
 **/
void free(void *__buf)
{
    mutex_lock(&malloc_mp);
    _free(__buf);
    mutex_unlock(&malloc_mp);
}
