/* $Id$ */

/*
 DSPAM
 COPYRIGHT (C) 2002-2006 JONATHAN A. ZDZIARSKI

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; version 2
 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#ifndef _DSPAM_ERROR_H
#  define _DSPAM_ERROR_H

#ifdef _WIN32
#   define LOG_CRIT     2
#   define LOG_ERR      3
#   define LOG_WARNING  4
#   define LOG_INFO     6
#else
#include <syslog.h>
#endif
#ifdef DAEMON
#include <pthread.h>
extern pthread_mutex_t  __syslog_lock;
#endif

#ifdef DEBUG
extern int DO_DEBUG;
#endif

#ifndef DEBUG
#ifdef HAVE_ISO_VARARGS
#define LOGDEBUG( ... );
#else
inline void LOGDEBUG (const char *err, ...) { UNUSED(err); }
#endif
#else
void LOGDEBUG (const char *err, ... );
#endif

#ifdef _WIN32
#ifdef HAVE_ISO_VARARGS
#define LOG ( ... );
#else
inline void LOG (int prio, const char *err, ... ) { UNUSED(prio); UNUSED(err); }
#endif
#else
void LOG (int prio, const char *err, ... );
#endif

char *format_date_r (char *buf);

void  debug_out (const char *);

#endif /* _DSPAM_ERROR_H */
