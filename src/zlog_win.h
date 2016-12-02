#ifndef _ZLOG_WIN_H_
#define _ZLOG_WIN_H_

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <Windows.h>
#include <WinBase.h>
#include <time.h>
#include <winsock.h>
#include <stdbool.h>

#ifndef localtime_r
extern struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif
extern int gettimeofday(struct timeval *tv, struct timezone *tz);
extern int fsync (int fd);
extern int gethostname_nowinsock(char *name, size_t len);


/*
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 *
 * priorities (these are ordered)
 */
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

#define	LOG_PRIMASK	0x07	/* mask to extract priority part (internal) */
				/* extract priority */
#define	LOG_PRI(p)	((p) & LOG_PRIMASK)
#define	LOG_MAKEPRI(fac, pri)	((fac) | (pri))

#endif