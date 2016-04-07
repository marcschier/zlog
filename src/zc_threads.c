
#include "zc_threads.h"
#include "zc_defs.h"
#ifndef __STDC_NO_THREADS__
#include <thr/threads.h>
#elif _HAVE_PTHREAD_H
#include <pthread.h>
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

int zc_tls_create(zc_tls_t* tls, void(*destroy)(void*))
{
#ifdef _WIN32
    *tls = (zc_tls_t*)TlsAlloc();
    return (DWORD)*tls == TLS_OUT_OF_INDEXES ? -1 : 0;
#elif !defined(__STDC_NO_THREADS__)
    return tss_create((tss_t*)tls, destroy);
#else
    return pthread_create_key((pthread_key_t*)tls, destroy);
#endif
}

void* zc_tls_get(zc_tls_t tls)
{
#ifdef _WIN32
    return TlsGetValue((DWORD)tls);
#elif !defined(__STDC_NO_THREADS__)
    return tss_get((tss_t)tls);
#else
    return pthread_getspecific((pthread_key_t)tls);
#endif
}

int zc_tls_set(zc_tls_t tls, void* val)
{
#ifdef _WIN32
    return TlsSetValue((DWORD)tls, val) ? 0 : -1;
#elif !defined(__STDC_NO_THREADS__)
    return tss_set((tss_t)tls, val);
#else
    return pthread_setspecific((pthread_key_t)tls, val);
#endif
}

int zc_tls_destroy(zc_tls_t tls)
{
#ifdef _WIN32
    return TlsFree((DWORD)tls) ? 0 : -1;
#endif
    return 0;
}

int zc_mutex_init(zc_mutex_t* mtx)
{
    int rc;
#if !defined(__STDC_NO_THREADS__)
    mtx_t* mutex = malloc(sizeof(mtx_t));
    if (!mutex)
        return -1;
    rc = mtx_init(mutex, mtx_plain | mtx_recursive);
    if(rc != thrd_success)
        goto err;
#else
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    if (!mutex)
        return -1;
    rc = pthread_mutex_init(mutex, NULL);
    if (rc != 0)
        goto err;
#endif
    *mtx = mutex;
    return 0;
err:
    free(mutex);
    *mtx = NULL;
    return rc;
}

int zc_mutex_trylock(zc_mutex_t mtx)
{
    int rc;
#if !defined(__STDC_NO_THREADS__)
    rc = mtx_trylock((mtx_t*)mtx);
    if (rc == thrd_busy)
        return EBUSY;
#else
    rc = pthread_mutex_trylock((pthread_mutex_t*)tls);
#endif
    return rc;
}
int zc_mutex_lock(zc_mutex_t mtx)
{
#if !defined(__STDC_NO_THREADS__)
    return mtx_lock((mtx_t*)mtx);
#else
    return pthread_mutex_lock((pthread_mutex_t*)tls);
#endif
}

int zc_mutex_unlock(zc_mutex_t mtx)
{
#if !defined(__STDC_NO_THREADS__)
    return mtx_unlock((mtx_t*)mtx);
#else
    return pthread_mutex_unlock((pthread_mutex_t*)tls);
#endif
}

int zc_mutex_destroy(zc_mutex_t mtx)
{
    int rc = 0;
#if !defined(__STDC_NO_THREADS__)
    mtx_destroy((mtx_t*)mtx);
#else
    rc = pthread_mutex_destroy((pthread_mutex_t*)tls);
#endif
    free(mtx);
    return rc;
}

int zc_atomic_inc(volatile int* ptr)
{
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ == 201112) && (__STDC_NO_ATOMICS__!=1)
    return atomic_fetch_add(ptr, 1)
#elif defined(WIN32)
    return InterlockedIncrement(ptr);
#elif defined(__GNUC__)
    return __sync_add_and_fetch(ptr, 1)
#else
#error Need to add atomic incr.
#endif
}

int zc_atomic_dec(volatile int* ptr)
{
    /*if macro DEC_REF returns DEC_RETURN_ZERO that means the ref count has reached zero.*/
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
    lock->exclusive = false;
    *rwlock = lock;
#else
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
#else
    rc = pthread_rwlock_wrlock((pthread_rwlock_t*)rwlock);
#endif
    return rc;
}

int zc_rwlock_rdlock(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifdef _WIN32
    AcquireSRWLockShared(&((srw_lock_t*)rwlock)->lock);
#else
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
        ReleaseSRWLockExclusive(&lock->lock);
        lock->exclusive = 0;
    }
    else
    {
        ReleaseSRWLockShared(&lock->lock);
    }
#else
    rc = pthread_rwlock_unlock((pthread_rwlock_t*)rwlock);
#endif
    return rc;
}

int zc_rwlock_destroy(zc_rwlock_t rwlock)
{
    int rc = 0;
#ifndef _WIN32
    rc = pthread_rwlock_destroy((pthread_rwlock_t*)rwlock);
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
    }
#else
    ret = close(*(int*)fd) == 0;
    if (!ret)
        zc_error("unlock file error : %s ", strerror(errno));
#endif
    free(fd);
    return ret ? 0 : -1;
}
