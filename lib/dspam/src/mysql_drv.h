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

#ifndef _MYSQL_DRV_H
#  define _MYSQL_DRV_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <mysql.h>

typedef struct _mysql_drv_dbh {
  MYSQL *dbh_read;
  MYSQL *dbh_write;
} *_mysql_drv_dbh_t;

struct _mysql_drv_storage
{
  _mysql_drv_dbh_t dbt;				/* db connections */
  struct _ds_spam_totals control_totals;        /* totals at storage init */
  struct _ds_spam_totals merged_totals;         /* totals for merged group */ 

  /* control token data; used to measure deltas from getall to setall 
   * enabling us to code a sql query based on increments/decrements 
   * instead of query-coded data */

  unsigned long long control_token;     /* control token crc */
  long control_sh;              /* control token spam hits at getall */
  long control_ih;              /* control token innocent hits at getall */

  MYSQL_RES *iter_user;         /* get_nextuser iteration result */
  MYSQL_RES *iter_token;        /* get_nexttoken iteration result */
  MYSQL_RES *iter_sig;          /* get_nextsignature iteration result */

  char u_getnextuser[MAX_FILENAME_LENGTH];
  struct passwd p_getpwuid;
  struct passwd p_getpwnam;
  int dbh_attached;
};

/* Driver-specific functions */

int	_mysql_drv_get_spamtotals	(DSPAM_CTX * CTX);
int	_mysql_drv_set_spamtotals	(DSPAM_CTX * CTX);
void	_mysql_drv_query_error		(const char *error, const char *query);
MYSQL	*_mysql_drv_connect		(DSPAM_CTX *CTX, const char *prefix);
MYSQL	*_mysql_drv_sig_write_handle	(DSPAM_CTX *CTX, 
	struct _mysql_drv_storage *s);
struct passwd *_mysql_drv_getpwnam      (DSPAM_CTX * CTX, const char *name);
struct passwd *_mysql_drv_getpwuid      (DSPAM_CTX * CTX, uid_t uid);
DSPAM_CTX     *_mysql_drv_init_tools	(const char *home, config_t config,
	void *dbt, int mode);

#ifdef VIRTUAL_USERS
struct passwd *_mysql_drv_setpwnam	(DSPAM_CTX * CTX, const char *name);
#endif

#ifdef PREFERENCES_EXTENSION
int _mysql_drv_set_attributes(DSPAM_CTX *CTX, config_t config);
#endif
#endif /* _MYSQL_DRV_H */

