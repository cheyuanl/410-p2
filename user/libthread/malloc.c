/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>

void *malloc(size_t __size)
{
  return _malloc(__size);
}

void *calloc(size_t __nelt, size_t __eltsize)
{
  return _calloc(__nelt, __eltsize);
}

void *realloc(void *__buf, size_t __new_size)
{
  return _realloc(__buf, __new_size);
}

void free(void *__buf)
{
  _free(__buf);
  return;
}
