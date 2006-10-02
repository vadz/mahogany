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

#ifndef _PGSQL_DRV_H
#  define _PGSQL_DRV_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <libpq-fe.h>

struct _pgsql_drv_storage
{
  PGconn *dbh;                   /* database connection */
  int pg_major_ver;              /* database major version */
  int pg_token_type;             /* type of token  */

  struct _ds_spam_totals control_totals;        /* totals at storage init */
  struct _ds_spam_totals merged_totals;         /* totals for merged group */

  /* control token data; used to measure deltas from getall to setall
   * enabling us to code a sql query based on increments/decrements
   * instead of query-coded data */

  unsigned long long control_token;     /* control token crc */
  long control_sh;              /* control token spam hits at getall */
  long control_ih;              /* control token innocent hits at getall */

  PGresult *iter_user;         /* get_nextuser iteration result */
  PGresult *iter_token;        /* get_nexttoken iteration result */
  PGresult *iter_sig;          /* get_nextsignature iteration result */

  char u_getnextuser[MAX_FILENAME_LENGTH];
  struct passwd p_getpwuid;
  struct passwd p_getpwnam;
  int dbh_attached;
};

/* Driver-specific functions */

int	_pgsql_drv_get_spamtotals	(DSPAM_CTX * CTX);
int	_pgsql_drv_set_spamtotals	(DSPAM_CTX * CTX);
void	_pgsql_drv_query_error		(const char *error, const char *query);
int	_pgsql_drv_token_type		(struct _pgsql_drv_storage *s, PGresult *result, int column);
char	*_pgsql_drv_token_write		(int type, unsigned long long token, char *buffer, size_t bufsz);
unsigned long long _pgsql_drv_token_read(int type, char *str);
int	_pgsql_drv_get_dbversion	(struct _pgsql_drv_storage *s);
PGconn *_pgsql_drv_connect		(DSPAM_CTX *CTX);
struct passwd *_pgsql_drv_getpwnam      (DSPAM_CTX * CTX, const char *name);
struct passwd *_pgsql_drv_getpwuid      (DSPAM_CTX * CTX, uid_t uid);
DSPAM_CTX *_pgsql_drv_init_tools( const char *home, config_t config,
 void *dbh, int mode);

#ifdef VIRTUAL_USERS
struct passwd *_pgsql_drv_setpwnam	(DSPAM_CTX * CTX, const char *name);
#endif

#ifdef PREFERENCES_EXTENSION
int _pgsql_drv_set_attributes(DSPAM_CTX *CTX, config_t config);
#endif

#endif /* _PGSQL_DRV_H */
