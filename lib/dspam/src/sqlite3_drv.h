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

#ifndef _SQLITE_DRV_H
#  define _SQLITE_DRV_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <sqlite3.h>

struct _sqlite_drv_storage
{
  struct sqlite3 *dbh;				/* database handle */
  struct _ds_spam_totals control_totals;        /* totals at storage init */
  struct _ds_spam_totals merged_totals;         /* totals for merged group */ 

  /* control token data; used to measure deltas from getall to setall 
   * enabling us to code a sql query based on increments/decrements 
   * instead of query-coded data */

  unsigned long long control_token;     /* control token crc */
  long control_sh;              /* control token spam hits at getall */
  long control_ih;              /* control token innocent hits at getall */

  sqlite3_stmt *iter_token;        /* get_nexttoken iteration result */
  sqlite3_stmt *iter_sig;          /* get_nextsignature iteration result */

  struct nt *dir_handles;
  int dbh_attached;
};

/* Driver-specific functions */

int	_sqlite_drv_get_spamtotals	(DSPAM_CTX * CTX);
int	_sqlite_drv_set_spamtotals	(DSPAM_CTX * CTX);
void	_sqlite_drv_query_error		(const char *error, const char *query);

#endif /* _SQLITE_DRV_H */

