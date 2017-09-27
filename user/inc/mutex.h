/** @file mutex.h
 *  @brief This file declare the methods of mutex
 */

#ifndef _MUTEX_H
#define _MUTEX_H

#include <mutex_type.h>
int mutex_init(mutex_t *mp);
void mutex_destroy(mutex_t *mp);
void mutex_lock(mutex_t *mp);
void mutex_unlock(mutex_t *mp);

#endif /* _MUTEX_H */
