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

#ifndef _UTIL_H
#  define _UTIL_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include "config.h"
#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#else /* _WIN32 */
#include <winsock2.h>
#endif /* !_WIN32/_WIN32 */

void	chomp	(char *string);
char *	ltrim	(char *str);
char *	rtrim	(char *str);
int	lc	(char *buff, const char *string);
#ifndef HAVE_STRCASESTR
char *  strcasestr (const char *, const char *);
#endif

#define ALLTRIM(str)  ltrim(rtrim(str))

#ifndef HAVE_STRSEP
char *strsep (char **stringp, const char *delim);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy (char *, const char *, size_t);
size_t strlcat (char *, const char *, size_t);
#endif

/*
 * Specialized Functions
 * Utilities specialized for DSPAM functions.
 *
 *  _ds_userdir_path()
 *    Generates the path for files within DSPAM's filesystem, according to the
 *    filesystem structure specified at configure time.
 *
 *  _ds_prepare_path_for()
 *    Creates any necessary subdirectories to support the supplied path
 *
 *  _ds_get_crc64()
 *    Generates the CRC of the supplied string, using CRC64
 *
 *  _ds_compute_complexity()
 *    Calculates the complexity of a token based on its nGram depth
 *
 *  _ds_compute_sparse()
 *    Calculates the number of sparse skips in a token
 *
 *  _ds_compute_weight()
 *    Calculates the markovian weight of a token
 */

#ifndef HAVE_STRTOK_R
char * strtok_r(char *s1, const char *s2, char **lasts);
#endif

#ifndef HAVE_INET_NTOA_R
unsigned int i2a
  (char* dest,unsigned int x);
char *inet_ntoa_r
  (struct in_addr in, char *buf, int len);
#endif

const char *	_ds_userdir_path (
  char *buff,
  const char *home,
  const char *filename,
  const char *extension);

int _ds_prepare_path_for   (const char *filename);
int _ds_compute_complexity (const char *token);
int _ds_compute_sparse     (const char *token);
int _ds_compute_weight     (const char *token);
char *_ds_truncate_token   (const char *token);

int	_ds_extract_address(
  char *buf, 
  const char *address, 
  size_t len);

double	_ds_gettime(void);

/*
 * Functions for working with locks.
 *
 * We use lock files under Unix and named mutexes under Win32. A lock must
 * first be opened, then an attempt to acquire it can be made and, whether it
 * succeeded or not, it must be closed, after releasing it if we did acquire
 * it, later.
 */

/* Initializes the object used for locking, must call _ds_close_lock() later. */
_ds_lock_t _ds_open_lock(const char *name);

/* Try to acquire the lock opened by _ds_open_lock(). Return 0 on success. */
int _ds_acquire_lock(_ds_lock_t lock);

/* Release the lock acquired previously by _ds_acquire_lock() */
int _ds_release_lock(_ds_lock_t lock);

/* Close the lock object. Should release it before if we had locked it. */
void _ds_close_lock(_ds_lock_t lock);

#ifndef _WIN32
int _ds_get_fcntl_lock  (int fd);
int _ds_free_fcntl_lock (int fd);
#endif

unsigned long long _ds_getcrc64
  (const char *);
int _ds_pow2
  (int exp);
double chi2Q
  (double x, int v);
float _ds_round
  (float n);

#ifdef _WIN32

/*
 * Win32 support functions:
 *
 *   _ds_win32_dir() returns the directory for DSPAM data/config files
 *
 *   _ds_win32_configfile() returns the default config file name
 */

const char *_ds_win32_configfile();
const char *_ds_win32_dir();

#endif /* _WIN32 */

#endif /* _UTIL_H */
