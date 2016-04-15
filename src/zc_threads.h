/*
* This file is part of the zlog Library.
*
* Copyright (C) 2011 by Hardy Simpson <HardySimpson1984@gmail.com>
*
* Licensed under the LGPL v2.1, see the file COPYING in base directory.
*/
/**
* =============================================================================
*
* \file lockfile.h
* \breif
* \version 1.0
* \date 2013-07-07 12:34:20
* \author  Song min.Li (Li), lisongmin@126.com
* \copyright Copyright (c) 2013, skybility
*
* =============================================================================
*/

#ifndef __zc_threads_h
#define __zc_threads_h

/* Thread */
typedef unsigned long zc_tid_t;
typedef unsigned long zc_pid_t;
zc_tid_t zc_thread_self(void);
zc_pid_t zc_proc_self(void);

/* Thread local storage */
typedef void* zc_tls_t;
#ifdef _WIN32
/* for windows, destroy callback needs to be stdcall, not cdecl */
#define zc_tls_free_call __stdcall
#endif
typedef void(zc_tls_free_call* zc_tls_destroy_t)(void*);
int zc_tls_create(zc_tls_t* tls, zc_tls_destroy_t destroy);
void* zc_tls_get(zc_tls_t tls);
int zc_tls_set(zc_tls_t tls, void* val);
int zc_tls_destroy(zc_tls_t tls);

/* Mutex */
typedef void* zc_mutex_t;
int zc_mutex_init(zc_mutex_t* mtx);
int zc_mutex_trylock(zc_mutex_t mtx);
int zc_mutex_lock(zc_mutex_t mtx);
int zc_mutex_unlock(zc_mutex_t mtx);
int zc_mutex_destroy(zc_mutex_t mtx);

/* Atomic */
int zc_atomic_inc(volatile int* ptr);
int zc_atomic_dec(volatile int* ptr);

/* Rw lock */
typedef void* zc_rwlock_t;
int zc_rwlock_init(zc_rwlock_t* rwlock);
int zc_rwlock_wrlock(zc_rwlock_t rwlock);
int zc_rwlock_rdlock(zc_rwlock_t rwlock);
int zc_rwlock_unlock(zc_rwlock_t rwlock);
int zc_rwlock_destroy(zc_rwlock_t rwlock);

/* file lock */
typedef void* zc_lock_fd_t;
zc_lock_fd_t zc_lock_fd(char* path);
int zc_unlock_fd(zc_lock_fd_t fd);

#endif /*__zc_threads_h*/