/** @file sem.c
 *  @brief Implementation of semaphore API.
 *
 *  This file contains methods for semaphores.
 *
 *  @author Che-Yuan Liang (cheyuanl)
 *  @bug No known bugs.
 */

#include <assert.h> /* panic() */
#include <cond.h>
#include <mutex.h>
#include <sem_type.h>
#include <simics.h>

/** @brief Initialize the semaphore
 *
 * This should be only called once before calling other sem
 * methods. The sem could be re-init after it is successfully
 * destroyed.
 *
 * @param sem Address of semaphore.
 * @param count Max allowed threads to decrease the count without
 *              being blocked.
 * @return 0 on success, -1 on error.
*/

int sem_init(sem_t *sem, int count) {
    /* check null pointer */
    if (!sem) {
        lprintf("Input semaphore is null");
        return -1;
    }
    /* initialize the mutex of the semaphore */
    if (mutex_init(&(sem->mutex)) < 0) {
        lprintf("Cannot initialize the mutex in semephore");
        return -1;
    }
    /* initialize the cv of the semaphore */
    if (cond_init(&(sem->cv)) < 0) {
        lprintf("Cannot initialize the cv in semaphore");
        return -1;
    }

    sem->count = count;
    sem->init = 1;

    return 0;
}

/** @brief Destroy the cond variable.
 *
 *  @note Only after this instruction is done, the sem
 *        can be called init again.
 *
 *  @param sem The address of semaphore.
 *  @return Void.
 */
void sem_destroy(sem_t *sem) {
    /* check illegal calls */
    if (!sem) {
        panic("sem_destroy: The input pointer in null");
    }

    if (!sem->init) {
        panic("sem_destroy: The semaphore hasn't been initialized");
    }

    /* delegate condition checking to cv and mutex */
    cond_destroy(&(sem->cv));
    mutex_destroy(&(sem->mutex));

    /* now we are safe */
    sem->init = 0;
}

/** @brief Wait until the count is greater than zero
 *
 *  The caller will go to sleep until the sem->count is greater than zero.
 *
 *  @param sem The address of semaphore.
 */
void sem_wait(sem_t *sem) {
    /* check illegal calls */
    if (!sem) {
        panic("sem_wait: The input pointer in null");
    }

    if (!sem->init) {
        panic("sem_wait: The semaphore hasn't been initialized");
    }

    mutex_lock(&(sem->mutex));

    while (sem->count <= 0) {
        /* go to sleep */
        cond_wait(&(sem->cv), &(sem->mutex));
    }
    sem->count--;

    mutex_unlock(&(sem->mutex));

    return;
}

/** @brief Signal the threads waiting on this semaphore
 *
 *  @param sem The address of semaphore.
 */
void sem_signal(sem_t *sem) {
    /* check illegal calls */
    if (!sem) {
        panic("sem_wait: The input pointer in null");
    }

    if (!sem->init) {
        panic("sem_wait: The semaphore hasn't been initialized");
    }

    mutex_lock(&(sem->mutex));

    sem->count++;
    /* signal the threads waiting on this semaphore */
    cond_signal(&(sem->cv));

    mutex_unlock(&(sem->mutex));

    return;
}
