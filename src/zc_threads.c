
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "zc_threads.h"
#include "zc_defs.h"

#ifdef _WIN32
#elif _HAVE_PTHREAD_H
#include <pthread.h>
#elif !defined(__STDC_NO_THREADS__)
#include <thr/threads.h>
#else
#error Either c11 or pthreads is needed
#endif
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ == 201112) && (__STDC_NO_ATOMICS__!=1)
#include <stdatomic.h>
#endif
#include <errno.h>

zc_tid_t zc_thread_self(void)
{
#ifdef _WIN32
    return (zc_tid_t)GetCurrentThreadId();
#elif _HAVE_PTHREAD_H
    return (zc_tid_t)pthread_self();
#else
    return (zc_tid_t)gettid();
#endif
}

zc_pid_t zc_proc_self(void)
{
#ifdef _WIN32
    return (zc_pid_t)GetCurrentProcessId();
#else
    return (zc_pid_t)getpid();
#endif
}

int zc_tls_create(zc_tls_t* tls, zc_tls_destroy_t destroy)
{
#ifdef _WIN32 
    DWORD_PTR idx = FlsAlloc(destroy);
    *tls = (zc_tls_t)idx;
    return (DWORD)idx == FLS_OUT_OF_INDEXES ? -1 : 0;
#elif _HAVE_PTHREAD_H
    return pthread_key_create((pthread_key_t*)tls, destroy);
#else
    return tss_create((tss_t*)tls, destroy);
#endif
}

void* zc_tls_get(zc_tls_t tls)
{
#ifdef _WIN32
    return FlsGetValue((DWORD)(DWORD_PTR)tls);
#elif _HAVE_PTHREAD_H
    return pthread_getspecific((pthread_key_t)tls);
#else
    return tss_get((tss_t)tls);
#endif
}

int zc_tls_set(zc_tls_t tls, void* val)
{
#ifdef _WIN32
    return FlsSetValue((DWORD)(DWORD_PTR)tls, val) ? 0 : -1;
#elif _HAVE_PTHREAD_H
    return pthread_setspecific((pthread_key_t)tls, val);
#else
    return tss_set((tss_t)tls, val);
#endif
}

int zc_tls_destroy(zc_tls_t tls)
{
#ifdef _WIN32
    return FlsFree((DWORD)(DWORD_PTR)tls) ? 0 : -1;
#endif
    return 0;
}

int zc_mutex_init(zc_mutex_t* mtx)
{
    int rc = 0;
#ifdef _WIN32
    CRITICAL_SECTION* mutex = malloc(sizeof(CRITICAL_SECTION));
    if (!mutex)
        return -1;
    InitializeCriticalSection(mutex);
    goto done;
#elif _HAVE_PTHREAD_H
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    if (!mutex)
        return -1;
    rc = pthread_mutex_init(mutex, NULL);
    if (rc != 0)
        goto err;
    goto done;
err:
    free(mutex);
    *mtx = NULL;
    return rc;
#else
    mtx_t* mutex = malloc(sizeof(mtx_t));
    if (!mutex)
        return -1;
    rc = mtx_init(mutex, mtx_plain | mtx_recursive);
    if(rc != thrd_success)
        goto err;
    goto done;
#endif
done:
    *mtx = mutex;
    return 0;
}

int zc_mutex_trylock(zc_mutex_t mtx)
{
    int rc = 0;
#ifdef _WIN32
    if (!TryEnterCriticalSection((CRITICAL_SECTION*)mtx))
        return EBUSY;
#elif _HAVE_PTHREAD_H
    rc = pthread_mutex_trylock((pthread_mutex_t*)mtx);
#else
    rc = mtx_trylock((mtx_t*)mtx);
    if (rc == thrd_busy)
        return EBUSY;
#endif
    return rc;
}
int zc_mutex_lock(zc_mutex_t mtx)
{
#ifdef _WIN32
    EnterCriticalSection((CRITICAL_SECTION*)mtx);
    return 0;
#elif _HAVE_PTHREAD_H
    return pthread_mutex_lock((pthread_mutex_t*)mtx);
#else
    return mtx_lock((mtx_t*)mtx);
#endif
}

int zc_mutex_unlock(zc_mutex_t mtx)
{
#ifdef _WIN32
    LeaveCriticalSection((CRITICAL_SECTION*)mtx);
    return 0;
#elif _HAVE_PTHREAD_H
    return pthread_mutex_unlock((pthread_mutex_t*)mtx);
#else
    return mtx_unlock((mtx_t*)mtx);
#endif
}

int zc_mutex_destroy(zc_mutex_t mtx)
{
    int rc = 0;
#ifdef _WIN32
    DeleteCriticalSection((CRITICAL_SECTION*)mtx);
#elif _HAVE_PTHREAD_H
    rc = pthread_mutex_destroy((pthread_mutex_t*)mtx);
#else
    mtx_destroy((mtx_t*)mtx);
#endif
    free(mtx);
    return rc;
}

int zc_atomic_inc(volatile int* ptr)
{
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ == 201112) && (__STDC_NO_ATOMICS__!=1)
    return atomic_fetch_add(ptr, 1) + 1;
#elif defined(WIN32)
    return InterlockedIncrement(ptr);
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, 1);
#else
#error Need to add atomic incr.
#endif
}

int zc_atomic_dec(volatile int* ptr)
{
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ == 201112) && (__STDC_NO_ATOMICS__!=1)
    return atomic_fetch_sub(ptr, 1) - 1;
#elif defined(WIN32)
    return InterlockedDecrement(ptr);
#elif defined(__GNUC__)
    return __sync_sub_and_fetch(ptr, 1);
#else
#error Need to add atomic decr.
#endif
}

#ifdef _WIN32
typedef struct
{
    SRWLOCK lock;
    int exclusive;
}
srw_lock_t;
#endif

int zc_rwlock_init(zc_rwlock_t* rwlock)
{
    int rc = 0;
#ifdef _WIN32
    srw_lock_t* lock;
    lock = malloc(sizeof(srw_lock_t));
    if (!lock)
        return -1;
    InitializeSRWLock(&lock->lock);
    lock->exclusive = 0;
    *rwlock = lock;
#elif _HAVE_PTHREAD_H
    pthread_rwlock_t* lock = malloc(sizeof(pthread_rwlock_t));
    if (!lock)
        return -1;
    rc = pthread_rwlock_init(lock, NULL);
    if (rc == 0)
        *rwlock = lock;
    else
        free(lock);
#endif
    return rc;
}

int zc_rwlock_wrlock(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifdef _WIN32
    srw_lock_t* lock;
    lock = (srw_lock_t*)rwlock;
    AcquireSRWLockExclusive(&lock->lock);
    lock->exclusive = 1;
#elif _HAVE_PTHREAD_H
    rc = pthread_rwlock_wrlock((pthread_rwlock_t*)rwlock);
#endif
    return rc;
}

int zc_rwlock_rdlock(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifdef _WIN32
    AcquireSRWLockShared(&((srw_lock_t*)rwlock)->lock);
#elif _HAVE_PTHREAD_H
    rc = pthread_rwlock_rdlock((pthread_rwlock_t*)rwlock);
#endif
    return rc;
}

int zc_rwlock_unlock(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifdef _WIN32
    srw_lock_t* lock;
    lock = (srw_lock_t*)rwlock;
    if (lock->exclusive)
    {
        lock->exclusive = 0;
        ReleaseSRWLockExclusive(&lock->lock);
    }
    else
    {
        ReleaseSRWLockShared(&lock->lock);
    }
#elif _HAVE_PTHREAD_H
    rc = pthread_rwlock_unlock((pthread_rwlock_t*)rwlock);
#endif
    return rc;
}

int zc_rwlock_destroy(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifndef _WIN32
#if _HAVE_PTHREAD_H
    rc = pthread_rwlock_destroy((pthread_rwlock_t*)rwlock);
#endif
#endif
    free(rwlock);
    return rc;
}

zc_lock_fd_t zc_lock_fd(char* path)
{
    zc_lock_fd_t ret;
    if (!path || strlen(path) <= 0)
        return NULL;
#ifdef _WIN32
    ret = malloc(sizeof(HANDLE));
    if (!ret)
        return NULL;
    *(HANDLE*)ret = CreateFile(
        path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*(HANDLE*)ret == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        zc_error("lock file error : %d ", err);
        goto err;
    }
#else
    ret = malloc(sizeof(int));
    if (!ret)
        return NULL;
    *(int*)ret = open(path, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG | S_IRWXO);
    if (*(int*)ret == -1)
    {
        zc_error("lock file error : %s ", strerror(errno));
        goto err;
    }
#endif
    return ret;
err:
    free(ret);
    ret = NULL;
    return ret;
}

int zc_unlock_fd(zc_lock_fd_t fd)
{
    bool ret = false;
    if (fd == NULL)
        return 0;
#ifdef _WIN32
    if (*(HANDLE*)fd && *(HANDLE*)fd != INVALID_HANDLE_VALUE)
    {
        ret = FALSE != CloseHandle(*(HANDLE*)fd);
        if (!ret)
        {
            DWORD err = GetLastError();
            zc_error("unlock file error : %d ", err);
        }
        *(HANDLE*)fd = INVALID_HANDLE_VALUE;
    }
#else
    ret = close(*(int*)fd) == 0;
    if (!ret)
        zc_error("unlock file error : %s ", strerror(errno));
    *(int*)fd = 0;
#endif
    free(fd);
    return ret ? 0 : -1;
}
