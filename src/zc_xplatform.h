/*
 * This file is part of the zlog Library.
 *
 * Copyright (C) 2011 by Hardy Simpson <HardySimpson1984@gmail.com>
 *
 * Licensed under the LGPL v2.1, see the file COPYING in base directory.
 */
#ifndef __zc_xplatform_h
#define __zc_xplatform_h

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <WinBase.h>
#include <stdbool.h>
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif
#endif

 /* Types */
#include <limits.h>

#define ZLOG_INT32_LEN   sizeof("-2147483648") - 1
#define ZLOG_INT64_LEN   sizeof("-9223372036854775808") - 1

#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define ZLOG_MAX_UINT32_VALUE  (uint32_t) 0xffffffffLL
#else
#define ZLOG_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#endif

#define ZLOG_MAX_INT32_VALUE   (uint32_t) 0x7fffffff


/* String manipulation */
#include <string.h>

#if defined(_WIN32)

#include <Windows.h>
#include <WinBase.h>

#include "zlog_win.h"

#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#define STRICMP(_a_,_C_,_b_) ( _stricmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( _strnicmp(_a_,_b_,_n_) _C_ 0 )

#else

#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
#include <AvailabilityMacros.h>
#endif

#include <strings.h>

#define STRCMP(_a_,_C_,_b_) ( strcmp(_a_,_b_) _C_ 0 )
#define STRNCMP(_a_,_C_,_b_,_n_) ( strncmp(_a_,_b_,_n_) _C_ 0 )
#define STRICMP(_a_,_C_,_b_) ( strcasecmp(_a_,_b_) _C_ 0 )
#define STRNICMP(_a_,_C_,_b_,_n_) ( strncasecmp(_a_,_b_,_n_) _C_ 0 )

#endif

/* Time and networking */

#if defined(_WIN32)
#include <time.h>
#include <winsock.h>

extern struct tm *localtime_r(const time_t *timep, struct tm *result);
extern int gettimeofday(struct timeval *tv, struct timezone *tz);
extern int fsync(int fd);
extern int gethostname_nowinsock(char *name, size_t len);
#define zlog_gethostname gethostname_nowinsock

#else
#include <sys/time.h>
#define zlog_gethostname gethostname
#endif

/* File */
#if defined(_WIN32)
#include <io.h>

#define zlog_stat _stat64
#define popen _popen
#define pclose _pclose
#define open _open
#define write _write
#define close _close
#define fileno _fileno
#define unlink _unlink

#define STDOUT_FILENO fileno(stdout)
#define STDERR_FILENO fileno(stderr)

#define ZLOG_DEFAULT_FILE_PERMS _S_IWRITE
#define FILE_NEWLINE "\n"
#define FILE_NEWLINE_LEN 1
#define MAXLEN_PATH 1024
#else
#include <unistd.h>
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
#define zlog_stat stat64
#else
#define zlog_stat stat
#endif
#define ZLOG_DEFAULT_FILE_PERMS 0x0600
#define FILE_NEWLINE "\n"
#define FILE_NEWLINE_LEN 1
#define MAXLEN_PATH 1024
#endif

/* Define zlog_fsync to fdatasync() in Linux and fsync() for all the rest */
#ifdef __linux__
#define zlog_fsync fdatasync
#else
#define zlog_fsync fsync
#endif

#define MAXLEN_CFG_LINE (MAXLEN_PATH * 4)

#endif
