/** @file rwlock.c
 *  @brief Implementation of reader/writer lock.
 *
 *  This file contains implementation for rwlock. We implement
 *  a rwlock that satisfy the second rw problem. That is it is
 *  in favor of writer, whenever there is any writer waiting.
 *  the reader cannot grab the lock.
 *
 *  @author Che-Yuan Liang (cheyuanl)
 *  @bug No known bugs.
 */
#include <simics.h> /* lprintf() */
#include <assert.h> /* panic() */
#include <cond.h>
#include <mutex.h>
#include <rwlock.h>
#include <thread.h> /* thr_getid() */

/** @brief Initialize the rwlock
 *
 *  This should be only called once before calling other
 *  rwlock methods. The rwlock could be re-init after it is
 *  successfully destroyed.
 *
 *  @param rwlock Address of rwlock.
 *  @return 0 on success, -1 on error.
 */
int rwlock_init(rwlock_t *rwlock) {
    /* check null pointer */
    if (!rwlock) {
        lprintf("Input rwlock is null");
        return -1;
    }
    /* initialize the mutex of the rwlock */
    if (mutex_init(&(rwlock->mutex)) < 0) {
        lprintf("Cannot initialize the mutex in rwlock");
        return -1;
    }
    /* initialize the cv of the rwlock */
    if (cond_init(&(rwlock->writer_cv)) < 0) {
        lprintf("Cannot initialize the writer cv in rwlock");
        return -1;
    }
    if (cond_init(&(rwlock->reader_cv)) < 0) {
        lprintf("Cannot initialize the reader cv in rwlock");
        return -1;
    }

    rwlock->init = 1;

    rwlock->num_reader = 0;
    rwlock->num_wait_writer = 0;

    rwlock->writer_tid = 0;
    rwlock->write_flag = 0;

    return 0;
}

/** @brief Acquire the lock.
 *
 *  The method will perform different operatio depend on the type of
 *  lock requested. This function is in favor of writer, if there is any
 *  writer waiting in the line, the reader will conditional wait on that.
 *
 *  @param rwlock Address of the rwlock.
 *  @param type The type of lock (either RWLOCK_READ or RWLOCK_WRITE)
 *  @return Void.
 */
void rwlock_lock(rwlock_t *rwlock, int type) {
    if (!rwlock) {
        panic("rwlock_lock: The input pointer is null");
    }

    if (!rwlock->init) {
        panic("rwlock_lock: The rwlock hasn't been intialized");
    }

    if ((type != RWLOCK_READ) && (type != RWLOCK_WRITE)) {
        panic("rwlock_lock: Invalid lock type %d", type);
    }

    mutex_lock(&(rwlock->mutex));

    /* reader */
    if (type == RWLOCK_READ) {
        /* if any thread is writing or if there is any writer waiting,
           the reader should be blocked */
        while ((rwlock->num_wait_writer > 0) || (rwlock->write_flag)) {
            cond_wait(&(rwlock->reader_cv), &(rwlock->mutex));
        }
        /* reader gets the lock */
        rwlock->num_reader++;

        mutex_unlock(&(rwlock->mutex));
        return;
    }
    /* writer */
    else {
        /* register as waiting writer, so reader thread can yield */
        rwlock->num_wait_writer++;
        /* wait if any thread is writing or any thread is reading */
        while (rwlock->write_flag == 1 || rwlock->num_reader > 0) {
            cond_wait(&(rwlock->writer_cv), &(rwlock->mutex));
        }
        /* writer gets the semaphore */
        rwlock->num_wait_writer--;
        rwlock->writer_tid = thr_getid();
        rwlock->write_flag = 1;

        mutex_unlock(&(rwlock->mutex));
        return;
    }
}

/** @brief Notify the waiting threads base on the state of the lock
 *
 *  Depend on if the caller is a reader or writer, this function will have
 *  different operation. If caller thread is last reader, it will signal
 *  the writer if there is any. If the caller thread is the writer and there
 *  is no writer waiting, it should wake up the waiting reader threads. The
 *  threads should come in a FIFO manner based on the current implementation
 *  of conditional variable.
 *
 *  @param rwlock The address of rwlock.
 *  @return Void.
 */
void rwlock_unlock(rwlock_t *rwlock) {
    if (!rwlock) {
        panic("rwlock_unlock: The input pointer is null");
    }

    if (!rwlock->init) {
        panic("rwlock_unlock: The rwlock hasn't been intialized");
    }

    mutex_lock(&(rwlock->mutex));

    /* caller thread is writer */
    if ((rwlock->write_flag) && (rwlock->writer_tid == thr_getid())) {
        /* clear write_flag */
        rwlock->write_flag = 0;

        /* Only when there is no writer is waiting,
         * other readers can be waked up */
        if (rwlock->num_wait_writer == 0) {
            cond_signal(&(rwlock->reader_cv));
        } else {
            cond_signal(&(rwlock->writer_cv));
        }
    }
    /* caller thread is reader */
    else {
        rwlock->num_reader--;
        /* now the writer can get in */
        if (rwlock->num_reader == 0) {
            cond_signal(&(rwlock->writer_cv));
        }
    }

    mutex_unlock(&(rwlock->mutex));
    return;
}

/** @brief Destroy the rwlock.
 *
 *  @note Only after this instruction is done, the rwlock
 *        can be called init again.
 *
 *  @param rwlock The address of rwlock.
 *  @return Void.
 */
void rwlock_destroy(rwlock_t *rwlock) {
    if (!rwlock) {
        panic("rwlock_destroy: The input pointer is null");
    }

    if (!rwlock->init) {
        panic("rwlock_destroy: The rwlock hasn't been initialized");
    }

    /* delegate condition checking to cv and mutex */
    mutex_destroy(&(rwlock->mutex));
    cond_destroy(&(rwlock->reader_cv));
    cond_destroy(&(rwlock->writer_cv));

    /* now we are safe */
    rwlock->init = 0;
    return;
}

/** @brief Downgrade the thread to reader
 *
 *  This will make the caller thread become reader lock's holder.
 *
 *  @param rwlock The address of rwlock.
 *  @return Void.
 */
void rwlock_downgrade(rwlock_t *rwlock) {
    if (!rwlock) {
        panic("rwlock_downgrade: The input pointer is null");
    }

    if (!rwlock->init) {
        panic("rwlock_downgrade: The rwlock hasn't been initialized");
    }

    if (!rwlock->write_flag) {
        panic("rwlock_downgrade: This method is without writer being locked");
    }

    if ((rwlock->write_flag) && (rwlock->writer_tid != thr_getid())) {
        panic("rwlock_downgrade: The caller isn't the write lock hodler");
    }

    /* now the caller thread's state */
    mutex_lock(&(rwlock->mutex));
    rwlock->write_flag = 0;
    rwlock->num_reader++;

    /* now the caller thread is reader */
    mutex_unlock(&(rwlock->mutex));
    return;
}
