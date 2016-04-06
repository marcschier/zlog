
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "zc_xplatform.h"

/* Credits Henry Rawas (henryr@schakra.com) */

#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64

struct timezone {
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

/* fnGetSystemTimePreciseAsFileTime is NULL if and only if it hasn't been initialized. */
static VOID(WINAPI *fnGetSystemTimePreciseAsFileTime)(LPFILETIME) = NULL;

static void InitHighResAbsoluteTime() {
    FARPROC fp;
    HMODULE module;

    if (fnGetSystemTimePreciseAsFileTime != NULL)
        return;

    /* Use GetSystemTimeAsFileTime as fallbcak where GetSystemTimePreciseAsFileTime is not available */
    fnGetSystemTimePreciseAsFileTime = GetSystemTimeAsFileTime;
    module = GetModuleHandleA("kernel32.dll");
    if (module) {
        fp = GetProcAddress(module, "GetSystemTimePreciseAsFileTime");
        if (fp) {
            fnGetSystemTimePreciseAsFileTime = (VOID(WINAPI*)(LPFILETIME)) fp;
        }
    }

    assert(fnGetSystemTimePreciseAsFileTime != NULL);
}

int gettimeofday_fast(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10;  /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz) {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (NULL == fnGetSystemTimePreciseAsFileTime) {
        InitHighResAbsoluteTime();
    }

    if (NULL != tv) {
        fnGetSystemTimePreciseAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10;  /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#ifndef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    struct tm *ret = localtime(timep);
    if (ret)
    {
        memcpy(result, ret, sizeof(struct tm));
    }
    return ret;
}
#endif

int gethostname_nowinsock(char *name, size_t len)
{
    int rc;
    rc = GetComputerNameExA(ComputerNameDnsHostname, name, &len);
    if (rc == 0) {
        sprintf(name, "localhost");
    }
    return 0;
}

int fsync (int fd)
{
    HANDLE h = (HANDLE) _get_osfhandle (fd);
    DWORD err;

    if (h == INVALID_HANDLE_VALUE)
    {
        errno = EBADF;
        return -1;
    }

    if (!FlushFileBuffers (h))
    {
        /* Translate some Windows errors into rough approximations of Unix
         * errors.  MSDN is useless as usual - in this case it doesn't
         * document the full range of errors.
         */
        err = GetLastError ();
        switch (err)
        {
        /* eg. Trying to fsync a tty. */
        case ERROR_INVALID_HANDLE:
            errno = EINVAL;
            break;
        default:
            errno = EIO;
        }
        return -1;
    }
    return 0;
}
