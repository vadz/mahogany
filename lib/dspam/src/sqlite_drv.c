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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#   include <pwd.h>
#   include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#ifdef TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   ifdef HAVE_SYS_TIME_H
#       include <sys/time.h>
#   else
#       include <time.h>
#   endif
#endif

#include "storage_driver.h"
#include "sqlite_drv.h"
#include "libdspam.h"
#include "config.h"
#include "error.h"
#include "language.h"
#include "util.h"
#include "config_shared.h"

#ifdef _WIN32
#   include <process.h>
#   include "dir_win32.h"
#endif

int
dspam_init_driver (DRIVER_CTX *DTX)
{
  return 0;
}

int
dspam_shutdown_driver (DRIVER_CTX *DTX)
{
  return 0;
}

int
_sqlite_drv_get_spamtotals (DSPAM_CTX * CTX)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[1024];
  char *err=NULL, **row;
  int nrow, ncolumn;
  int rc;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_sqlite_drv_get_spamtotals: invalid database handle (NULL)");
    return EINVAL;
  }

  memset(&s->control_totals, 0, sizeof(struct _ds_spam_totals));
  memset(&CTX->totals, 0, sizeof(struct _ds_spam_totals));

  snprintf (query, sizeof (query),
            "select spam_learned, innocent_learned, "
            "spam_misclassified, innocent_misclassified, "
            "spam_corpusfed, innocent_corpusfed, "
            "spam_classified, innocent_classified "
            " from dspam_stats");

  if ((sqlite_get_table(s->dbh, query, &row, &nrow, &ncolumn, &err))!=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  if (nrow>0 && row != NULL) {
    CTX->totals.spam_learned		= strtol (row[ncolumn], NULL, 0);
    CTX->totals.innocent_learned	= strtol (row[ncolumn+1], NULL, 0);
    CTX->totals.spam_misclassified	= strtol (row[ncolumn+2], NULL, 0);
    CTX->totals.innocent_misclassified 	= strtol (row[ncolumn+3], NULL, 0);
    CTX->totals.spam_corpusfed		= strtol (row[ncolumn+4], NULL, 0);
    CTX->totals.innocent_corpusfed	= strtol (row[ncolumn+5], NULL, 0);
    CTX->totals.spam_classified		= strtol (row[ncolumn+6], NULL, 0);
    CTX->totals.innocent_classified	= strtol (row[ncolumn+7], NULL, 0);
    rc = 0;
  } else {
    rc = EFAILURE;
  }

  sqlite_free_table(row);
  if ( !rc )
    memcpy(&s->control_totals, &CTX->totals, sizeof(struct _ds_spam_totals));

  return rc;
}

int
_sqlite_drv_set_spamtotals (DSPAM_CTX * CTX)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[1024];
  char *err=NULL;
  int result;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_sqlite_drv_set_spamtotals: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY)
  {
    _sqlite_drv_get_spamtotals (CTX);    /* undo changes to in memory totals */
    return 0;
  }

  /* dspam_stat_id insures only one stats record */

  if (s->control_totals.innocent_learned == 0)
  {
    snprintf (query, sizeof (query),
              "insert into dspam_stats(dspam_stat_id, spam_learned, " 
              "innocent_learned, spam_misclassified, innocent_misclassified, "
              "spam_corpusfed, innocent_corpusfed, "
              "spam_classified, innocent_classified) "
              "values(%d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld)",
              0,
              CTX->totals.spam_learned,
              CTX->totals.innocent_learned, 
              CTX->totals.spam_misclassified,
              CTX->totals.innocent_misclassified, 
              CTX->totals.spam_corpusfed,
              CTX->totals.innocent_corpusfed, 
              CTX->totals.spam_classified,
              CTX->totals.innocent_classified);
    result = sqlite_exec(s->dbh, query, NULL, NULL, &err);
  }

  if (s->control_totals.innocent_learned != 0 || result != SQLITE_OK)
  {
    snprintf (query, sizeof (query),
              "update dspam_stats set spam_learned = spam_learned %s %d, "
              "innocent_learned = innocent_learned %s %d, "
              "spam_misclassified = spam_misclassified %s %d, "
              "innocent_misclassified = innocent_misclassified %s %d, "
              "spam_corpusfed = spam_corpusfed %s %d, "
              "innocent_corpusfed = innocent_corpusfed %s %d, "
              "spam_classified = spam_classified %s %d, "
              "innocent_classified = innocent_classified %s %d ",
              (CTX->totals.spam_learned >
               s->control_totals.spam_learned) ? "+" : "-",
              abs (CTX->totals.spam_learned -
                   s->control_totals.spam_learned),
              (CTX->totals.innocent_learned >
               s->control_totals.innocent_learned) ? "+" : "-",
              abs (CTX->totals.innocent_learned -
                   s->control_totals.innocent_learned),
              (CTX->totals.spam_misclassified >
               s->control_totals.spam_misclassified) ? "+" : "-",
              abs (CTX->totals.spam_misclassified -
                   s->control_totals.spam_misclassified),
              (CTX->totals.innocent_misclassified >
               s->control_totals.innocent_misclassified) ? "+" : "-",
              abs (CTX->totals.innocent_misclassified -
                   s->control_totals.innocent_misclassified),
              (CTX->totals.spam_corpusfed >
               s->control_totals.spam_corpusfed) ? "+" : "-",
              abs (CTX->totals.spam_corpusfed -
                   s->control_totals.spam_corpusfed),
              (CTX->totals.innocent_corpusfed >
               s->control_totals.innocent_corpusfed) ? "+" : "-",
              abs (CTX->totals.innocent_corpusfed -
                   s->control_totals.innocent_corpusfed),
              (CTX->totals.spam_classified >
               s->control_totals.spam_classified) ? "+" : "-",
              abs (CTX->totals.spam_classified -
                  s->control_totals.spam_classified),
              (CTX->totals.innocent_classified >
               s->control_totals.innocent_classified) ? "+" : "-",
              abs (CTX->totals.innocent_classified -
                  s->control_totals.innocent_classified));

    if ((sqlite_exec(s->dbh, query, NULL, NULL, &err))!=SQLITE_OK)
    {
      _sqlite_drv_query_error (err, query);
      return EFAILURE;
    }
  }

  return 0;
}

int
_ds_getall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  buffer *query;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  char scratch[1024];
  struct _ds_spam_stat stat;
  unsigned long long token = 0;
  char *err=NULL, **row;
  int nrow, ncolumn, get_one = 0, i;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_getall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  stat.spam_hits = 0;
  stat.innocent_hits = 0;

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  snprintf (scratch, sizeof (scratch),
            "select token, spam_hits, innocent_hits "
            "from dspam_token_data where token in(");

  buffer_cat (query, scratch);
  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    snprintf (scratch, sizeof (scratch), "'%" LLU_FMT_SPEC "'", ds_term->key);
    buffer_cat (query, scratch);
    ds_term->s.innocent_hits = 0;
    ds_term->s.spam_hits = 0;
    ds_term->s.probability = 0;
    ds_term->s.status &= ~TST_DISK;
    ds_term = ds_diction_next(ds_c);
    if (ds_term != NULL)
      buffer_cat (query, ",");
    get_one = 1;
  }
  ds_diction_close(ds_c);
  buffer_cat (query, ")");

#ifdef VERBOSE
  LOGDEBUG ("sqlite query length: %ld\n", query->used);
  _sqlite_drv_query_error (strdup("VERBOSE DEBUG (INFO ONLY - NOT AN ERROR)"), query->data);
#endif

  if (!get_one) 
    return 0;

  if ((sqlite_get_table(s->dbh, query->data, &row, &nrow, &ncolumn, &err))
      !=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query->data);
    buffer_destroy(query);
    return EFAILURE;
  }

  if (nrow < 1) {
    sqlite_free_table(row);
    buffer_destroy(query);
    return 0;
  }

  if (row == NULL)
    return 0;

  stat.probability = 0;
  stat.status |= TST_DISK;
  for(i=1;i<=nrow;i++) {
    token = strtoull (row[(i*ncolumn)], NULL, 0);
    stat.spam_hits = strtol (row[1+(i*ncolumn)], NULL, 0);
    stat.innocent_hits = strtol (row[2+(i*ncolumn)], NULL, 0);

    if (stat.spam_hits < 0)
      stat.spam_hits = 0;
    if (stat.innocent_hits < 0)
      stat.innocent_hits = 0;

    ds_diction_addstat(diction, token, &stat);
  }

  sqlite_free_table(row);

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term && !s->control_token) {
    if (ds_term->s.spam_hits && ds_term->s.innocent_hits) {
      s->control_token = ds_term->key;
      s->control_sh = ds_term->s.spam_hits;
      s->control_ih = ds_term->s.innocent_hits;
    }
    ds_term = ds_diction_next(ds_c);
  }
  ds_diction_close(ds_c);

  if (!s->control_token)
  {
     ds_c = ds_diction_cursor(diction);
     ds_term = ds_diction_next(ds_c);
     s->control_token = ds_term->key;
     s->control_sh = ds_term->s.spam_hits;
     s->control_ih = ds_term->s.innocent_hits;
     ds_diction_close(ds_c);
  }

  buffer_destroy (query);
  return 0;
}

int
_ds_setall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  struct _ds_spam_stat stat, stat2;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  buffer *query;
  char scratch[1024];
  char *err=NULL;
  int update_one = 0;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_setall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY &&
        (CTX->training_mode != DST_TOE ||
          (diction->whitelist_token == 0 && (!(CTX->flags & DSF_NOISE)))))
    return 0;

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  if (s->control_token == 0)
  {
    ds_c = ds_diction_cursor(diction);
    ds_term = ds_diction_next(ds_c);
    if (ds_term == NULL)
    {
      stat.spam_hits = 0;
      stat.innocent_hits = 0;
    }
    else
    {
      stat.spam_hits = ds_term->s.spam_hits;
      stat.innocent_hits = ds_term->s.innocent_hits;
    }
    ds_diction_close(ds_c);
  }
  else
  {
    ds_diction_getstat(diction, s->control_token, &stat);
  }

  snprintf (scratch, sizeof (scratch),
            "update dspam_token_data set last_hit = date('now'), "
            "spam_hits = max(0, spam_hits %s %d), "
            "innocent_hits = max(0, innocent_hits %s %d) "
            "where token in(",
            (stat.spam_hits > s->control_sh) ? "+" : "-",
            abs (stat.spam_hits - s->control_sh),
            (stat.innocent_hits > s->control_ih) ? "+" : "-",
            abs (stat.innocent_hits - s->control_ih));

  buffer_cat (query, scratch);

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    int wrote_this = 0;
    if (CTX->training_mode == DST_TOE          && 
        CTX->classification == DSR_NONE        &&
	CTX->operating_mode == DSM_CLASSIFY    &&
        diction->whitelist_token != ds_term->key  &&
        (!ds_term->name || strncmp(ds_term->name, "bnr.", 4)))  
    {
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    if (!(ds_term->s.status & TST_DIRTY)) {
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    ds_diction_getstat(diction, ds_term->key, &stat2);

    if (!(stat2.status & TST_DISK))
    {
      char insert[1024];

        snprintf(insert, sizeof (insert),
                 "insert into dspam_token_data(token, spam_hits, "
                 "innocent_hits, last_hit) values('%" LLU_FMT_SPEC "', %ld, %ld, "
                 "date('now'))",
                 ds_term->key,
                 stat2.spam_hits > 0 ? (long) 1 : (long) 0, 
                 stat2.innocent_hits > 0 ? (long) 1 : (long) 0);

      if ((sqlite_exec(s->dbh, insert, NULL, NULL, &err)) != SQLITE_OK)
      {
        stat2.status |= TST_DISK;
        free(err);
      }
    }

    if ((stat2.status & TST_DISK))
    {
      snprintf (scratch, sizeof (scratch), "'%" LLU_FMT_SPEC "'", ds_term->key);
      buffer_cat (query, scratch);
      update_one = 1;
      wrote_this = 1;
      ds_term->s.status |= TST_DISK;
    }
    ds_term = ds_diction_next(ds_c);
    if (ds_term && wrote_this) 
      buffer_cat (query, ",");
  }
  ds_diction_close(ds_c);

  if (query->used && query->data[strlen (query->data) - 1] == ',')
  {
    query->used--;
    query->data[strlen (query->data) - 1] = 0;

  }

  buffer_cat (query, ")");

  LOGDEBUG("Control: [%ld %ld] [%ld %ld]", s->control_sh, s->control_ih, stat.spam_hits, stat.innocent_hits);

  if (update_one)
  {
    if ((sqlite_exec(s->dbh, query->data, NULL, NULL, &err))!=SQLITE_OK)
    {
      _sqlite_drv_query_error (err, query->data);
      buffer_destroy(query);
      return EFAILURE;
    }
  }

  buffer_destroy (query);
  return 0;
}

int
_ds_get_spamrecord (DSPAM_CTX * CTX, unsigned long long token,
                    struct _ds_spam_stat *stat)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[1024];
  char *err=NULL, **row;
  int nrow, ncolumn;


  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_get_spamrecord: invalid database handle (NULL)");
    return EINVAL;
  }

  snprintf (query, sizeof (query),
            "select spam_hits, innocent_hits from dspam_token_data "
            "where token = '%" LLU_FMT_SPEC "' ", token);

  stat->probability = 0.0;
  stat->spam_hits = 0;
  stat->innocent_hits = 0;
  stat->status &= ~TST_DISK;

  if ((sqlite_get_table(s->dbh, query, &row, &nrow, &ncolumn, &err))!=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  if (nrow < 1)
    sqlite_free_table(row);

  if (nrow < 1 || row == NULL)
    return 0;

  stat->spam_hits = strtol (row[0], NULL, 0);
  stat->innocent_hits = strtol (row[1], NULL, 0);
  stat->status |= TST_DISK;
  sqlite_free_table(row);
  return 0;
}

int
_ds_set_spamrecord (DSPAM_CTX * CTX, unsigned long long token,
                    struct _ds_spam_stat *stat)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[1024];
  char *err=NULL;
  int result = 0;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_set_spamrecord: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY)
    return 0;

  /* It's either not on disk or the caller isn't using stat.disk */
  if (!(stat->status & TST_DISK))
  {
    snprintf (query, sizeof (query),
              "insert into dspam_token_data(token, spam_hits, "
              "innocent_hits, last_hit)"
              " values('%" LLU_FMT_SPEC "', %ld, %ld, date('now'))",
              token, 
              stat->spam_hits > 0 ? stat->spam_hits : 0,
              stat->innocent_hits > 0 ? stat->innocent_hits : 0);
    result = sqlite_exec(s->dbh, query, NULL, NULL, &err);
  }

  if ((stat->status & TST_DISK) || result)
  {
    /* insert failed; try updating instead */
    snprintf (query, sizeof (query), "update dspam_token_data "
              "set spam_hits = %ld, "
              "innocent_hits = %ld "
              "where token = %" LLD_FMT_SPEC,
              stat->spam_hits > 0 ? stat->spam_hits : 0,
              stat->innocent_hits > 0 ? stat->innocent_hits : 0,
              token);

    if ((sqlite_exec(s->dbh, query, NULL, NULL, &err))!=SQLITE_OK)
    {
      _sqlite_drv_query_error (err, query);
      return EFAILURE;
    }
  }

  return 0;
}

int
_ds_init_storage (DSPAM_CTX * CTX, void *dbh)
{
  struct _sqlite_drv_storage *s;
  FILE *file;
  char buff[1024];
  char filename[MAX_FILENAME_LENGTH];
  char *err=NULL;
  struct stat st;
  int noexist;

  buff[0] = 0;

  if (CTX == NULL)
    return EINVAL;

  if (!CTX->home) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    return EINVAL;
  } 

  if (CTX->flags & DSF_MERGED) {
    LOG(LOG_ERR, ERR_DRV_NO_MERGED);
    return EINVAL;
  }

  /* don't init if we're already initted */
  if (CTX->storage != NULL)
  {
    LOGDEBUG ("_ds_init_storage: storage already initialized");
    return EINVAL;
  }

  s = malloc (sizeof (struct _sqlite_drv_storage));
  if (s == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  s->dbh = NULL;
  s->control_token = 0;
  s->iter_token = NULL;
  s->iter_sig = NULL;
  s->control_token = 0;
  s->control_sh = 0;
  s->control_ih = 0;
  s->dbh_attached = (dbh) ? 1 : 0;

  if (CTX->group == NULL || CTX->group[0] == 0)
    _ds_userdir_path (filename, CTX->home, CTX->username, "sdb");
  else
    _ds_userdir_path (filename, CTX->home, CTX->group, "sdb");
  _ds_prepare_path_for (filename);

  noexist = stat(filename, &st);

  if (dbh)
    s->dbh = dbh;
  else
    s->dbh = sqlite_open(filename, 0660, &err);
                                                                                
  if (s->dbh == NULL)
  {
    LOGDEBUG
      ("_ds_init_storage: sqlite_open: unable to initialize database: %s", err);    return EUNKNOWN;
  }

  /* Commit timeout of 20 minutes */
  sqlite_busy_timeout(s->dbh, 1000 * 60 * 20);

  /* Create database objects */

  if (noexist) {

    sqlite_exec(s->dbh, 
                "create table dspam_token_data (token char(20) primary key, "
                "spam_hits int, innocent_hits int, last_hit date)",
                NULL,
                NULL,
                &err);

    sqlite_exec(s->dbh,
                "create index id_token_data_02 on dspam_token_data"
                "(innocent_hits)",
                NULL,
                NULL,
                &err);

    sqlite_exec(s->dbh,
                "create table dspam_signature_data ("
                "signature char(128) primary key, data blob, created_on date)",
                NULL,
                NULL,
                &err);
                                                                                
    sqlite_exec(s->dbh,
                "create table dspam_stats (dspam_stat_id int primary key, "
                "spam_learned int, innocent_learned int, "
                "spam_misclassified int, innocent_misclassified int, "
                "spam_corpusfed int, innocent_corpusfed int, "
                "spam_classified int, innocent_classified int)",
                NULL,
                NULL,
                &err);
  }

  if (_ds_read_attribute(CTX->config->attributes, "SQLitePragma")) {
    char pragma[1024];
    attribute_t t = _ds_find_attribute(CTX->config->attributes, "SQLitePragma");
    while(t != NULL) {
      snprintf(pragma, sizeof(pragma), "PRAGMA %s", t->value);
      if ((sqlite_exec(s->dbh, pragma, NULL, NULL, &err))!=SQLITE_OK)
      {
        LOG(LOG_WARNING, "sqlite.pragma function error: %s: %s", err, pragma);
        _sqlite_drv_query_error (err, pragma);
      }
      t = t->next;
    } 
  } else if (CTX->home) {
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/sqlite.pragma", CTX->home);
    file = fopen(filename, "r");
    if (file != NULL) {
      while((fgets(buff, sizeof(buff), file))!=NULL) {
        chomp(buff);
        if ((sqlite_exec(s->dbh, buff, NULL, NULL, &err))!=SQLITE_OK)
        {
          LOG(LOG_WARNING, "sqlite.pragma function error: %s: %s", err, buff);
          _sqlite_drv_query_error (err, buff);
        }
      }
      fclose(file);
    }
  }

  CTX->storage = s;
  s->dir_handles = nt_create (NT_INDEX);

  s->control_token = 0;
  s->control_ih = 0;
  s->control_sh = 0; 

  /* get spam totals on successful init */
  if (CTX->username != NULL)
  {
      if (_sqlite_drv_get_spamtotals (CTX))
      {
        LOGDEBUG ("unable to load totals.  using zero values.");
      }
  }
  else
  {
    memset (&CTX->totals, 0, sizeof (struct _ds_spam_totals));
    memset (&s->control_totals, 0, sizeof (struct _ds_spam_totals));
  }

  return 0;
}

int
_ds_shutdown_storage (DSPAM_CTX * CTX)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  struct nt_node *node_nt;
  struct nt_c c_nt;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_shutdown_storage: invalid database handle (NULL)");
    return EINVAL;
  }

  node_nt = c_nt_first (s->dir_handles, &c_nt);
  while (node_nt != NULL)
  {
    DIR *dir;
    dir = (DIR *) node_nt->ptr;
    closedir (dir);
    node_nt = c_nt_next (s->dir_handles, &c_nt);
  }
                                                                                
  nt_destroy (s->dir_handles);


  /* Store spam totals on shutdown */
  if (CTX->username != NULL && CTX->operating_mode != DSM_CLASSIFY)
  {
      _sqlite_drv_set_spamtotals (CTX);
  }

  if (!s->dbh_attached)
    sqlite_close(s->dbh);

  s->dbh = NULL;

  free(s);
  CTX->storage = NULL;

  return 0;
}

int
_ds_create_signature_id (DSPAM_CTX * CTX, char *buf, size_t len)
{
  char session[64];
  char digit[6];
  int pid, j;

  pid = getpid ();
  snprintf (session, sizeof (session), "%8lx%d", (long) time (NULL), pid);

  for (j = 0; j < 2; j++)
  {
    snprintf (digit, 6, "%d", rand ());
    strlcat (session, digit, 64);
  }

  strlcpy (buf, session, len);
  return 0;
}

int
_ds_get_signature (DSPAM_CTX * CTX, struct _ds_spam_signature *SIG,
                   const char *signature)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  unsigned long length;
  unsigned char *mem;
  char query[128];
  char *err=NULL, **row;
  int nrow, ncolumn;
  void *ptr;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_get_signature: invalid database handle (NULL)");
    return EINVAL;
  }

  snprintf (query, sizeof (query),
            "select data, length(data) "
            " from dspam_signature_data where signature = \"%s\"",
            signature);

  if ((sqlite_get_table(s->dbh, query, &row, &nrow, &ncolumn, &err))!=SQLITE_OK)  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  if (nrow<1)
    sqlite_free_table(row);
  if (nrow<1 || row == NULL)
    return EFAILURE;

  length = strlen(row[ncolumn]);
  if (length == 0)
  {
    sqlite_free_table(row);
    return EFAILURE;
  }

  mem = malloc(length+1);
  if (mem == NULL) {
   LOG(LOG_CRIT, ERR_MEM_ALLOC);
   sqlite_free_table(row);
   return EUNKNOWN;
  }

  length = sqlite_decode_binary((unsigned char *) row[ncolumn], mem);
  if (length<=0) {
    LOG(LOG_ERR, "sqlite_decode_binary() failed with error %d", length);
    return EFAILURE;
  }

  ptr = realloc(mem, length);
  if (ptr)
    SIG->data = ptr;
  else {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    SIG->data = mem;
  }
  SIG->length = length;

  sqlite_free_table(row);
  return 0;
}

int
_ds_set_signature (DSPAM_CTX * CTX, struct _ds_spam_signature *SIG,
                   const char *signature)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  unsigned long length;
  char *mem;
  char scratch[1024];
  buffer *query;
  char *err=NULL;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_set_signature; invalid database handle (NULL)");
    return EINVAL;
  }

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  mem = calloc (1, 2 + (257*SIG->length)/254);
  if (mem == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    buffer_destroy(query);
    return EUNKNOWN;
  }

  length = sqlite_encode_binary(SIG->data, SIG->length, (unsigned char *) mem);
  if (length<0) {
   LOG(LOG_ERR, "sqlite_encode_binary() failed on error %d", length);
   buffer_destroy(query);
   return EFAILURE;
  }

  snprintf (scratch, sizeof (scratch),
            "insert into dspam_signature_data(signature, created_on, data) "
            "values(\"%s\", date('now'), '",
            signature);
  buffer_cat (query, scratch);
  buffer_cat (query, mem);
  buffer_cat (query, "')");

  if ((sqlite_exec(s->dbh, query->data, NULL, NULL, &err))!=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query->data);
    buffer_destroy(query);
    free(mem);
    return EFAILURE;
  }

  free (mem);
  buffer_destroy(query);
  return 0;
}

int
_ds_delete_signature (DSPAM_CTX * CTX, const char *signature)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[128];
  char *err=NULL;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_delete_signature: invalid database handle (NULL)");
    return EINVAL;
  }

  snprintf (query, sizeof (query),
            "delete from dspam_signature_data where signature = \"%s\"",
             signature);

  if ((sqlite_exec(s->dbh, query, NULL, NULL, &err))!=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  return 0;
}

int
_ds_verify_signature (DSPAM_CTX * CTX, const char *signature)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[128];
  char *err=NULL, **row;
  int nrow, ncolumn;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_verify_signature: invalid database handle (NULL)");
    return EINVAL;
  }

  snprintf (query, sizeof (query),
        "select signature from dspam_signature_data where signature = \"%s\"",
        signature);

  if ((sqlite_get_table(s->dbh, query, &row, &nrow, &ncolumn, &err))!=SQLITE_OK)  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  sqlite_free_table(row);

  if (nrow<1) {
    return -1;
  }

  return 0;
}

char *
_ds_get_nextuser (DSPAM_CTX * CTX)
{
  static char user[MAX_FILENAME_LENGTH];
  static char path[MAX_FILENAME_LENGTH];
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  struct nt_node *node_nt, *prev;
  struct nt_c c_nt;
  char *x = NULL, *y;
  DIR *dir = NULL;

  struct dirent *entry;

  if (s->dir_handles->items == 0)
  {
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/data", CTX->home);
    dir = opendir (filename);
    if (dir == NULL)
    {
      LOG (LOG_WARNING,
           "unable to open directory '%s' for reading: %s",
           CTX->home, strerror (errno));
      return NULL;
    }

    nt_add (s->dir_handles, (void *) dir);
    strlcpy (path, filename, sizeof (path));
  }
  else
  {
    node_nt = c_nt_first (s->dir_handles, &c_nt);
    while (node_nt != NULL)
    {
      if (node_nt->next == NULL)
        dir = (DIR *) node_nt->ptr;
      node_nt = c_nt_next (s->dir_handles, &c_nt);
    }
  }

  while ((entry = readdir (dir)) != NULL)
  {
    struct stat st;
    char filename[MAX_FILENAME_LENGTH];
    snprintf (filename, sizeof (filename), "%s/%s", path, entry->d_name);

    if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
      continue;

    if (stat (filename, &st)) {
      continue;
    }

    /* push a new directory */
    if (st.st_mode & S_IFDIR)
    {
      DIR *ndir;

      ndir = opendir (filename);
      if (ndir == NULL)
        continue;
      strlcat (path, "/", sizeof (path));
      strlcat (path, entry->d_name, sizeof (path));
      nt_add (s->dir_handles, (void *) ndir);
      return _ds_get_nextuser (CTX);
    }
    else if (!strncmp
             (entry->d_name + strlen (entry->d_name) - 4, ".sdb", 4))
    {
      strlcpy (user, entry->d_name, sizeof (user));
      user[strlen (user) - 4] = 0;
      return user;
    }
  }

  /* pop current directory */
  y = strchr (path, '/');
  while (y != NULL)
  {
    x = y;
    y = strchr (x + 1, '/');
  }
  if (x)
    x[0] = 0;

  /* pop directory handle from list */
  node_nt = c_nt_first (s->dir_handles, &c_nt);
  prev = NULL;
  while (node_nt != NULL)
  {
    if (node_nt->next == NULL)
    {
      dir = (DIR *) node_nt->ptr;
      closedir (dir);
      if (prev != NULL) {
        prev->next = NULL;
        s->dir_handles->insert = NULL;
      }
      else
        s->dir_handles->first = NULL;
      free (node_nt);
      s->dir_handles->items--;
      prev = node_nt;
      break;
    }
    prev = node_nt;
    node_nt = c_nt_next (s->dir_handles, &c_nt);
  }
  if (s->dir_handles->items > 0)
    return _ds_get_nextuser (CTX);

  /* done */

  user[0] = 0;
  return NULL;
}

struct _ds_storage_record *
_ds_get_nexttoken (DSPAM_CTX * CTX)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  struct _ds_storage_record *st;
  char query[128];
  char *err=NULL;
  const char **row, *query_tail=NULL;
  int ncolumn, x;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_get_nexttoken: invalid database handle (NULL)");
    return NULL;
  }

  st = calloc (1, sizeof (struct _ds_storage_record));
  if (st == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  if (s->iter_token == NULL)
  {
    snprintf (query, sizeof (query),
              "select token, spam_hits, innocent_hits, strftime('%%s', "
              "last_hit) from dspam_token_data");

    if ((sqlite_compile(s->dbh, query, &query_tail, &s->iter_token, &err))
        !=SQLITE_OK) 
    {
      _sqlite_drv_query_error (err, query);
      free(st); 
      return NULL;
    }
  }

  if ((x = sqlite_step(s->iter_token, &ncolumn, &row, NULL))
      !=SQLITE_ROW) {
    if (x != SQLITE_DONE) {
      _sqlite_drv_query_error (err, query);
      s->iter_token = NULL;
      free(st);
      return NULL;
    }
    sqlite_finalize((struct sqlite_vm *) s->iter_token, &err);
    s->iter_token = NULL;
    free(st);
    return NULL;
  }

  st->token = strtoull (row[0], NULL, 0);
  st->spam_hits = strtol (row[1], NULL, 0);
  st->innocent_hits = strtol (row[2], NULL, 0);
  st->last_hit = (time_t) strtol (row[3], NULL, 0);

  return st;
}

struct _ds_storage_signature *
_ds_get_nextsignature (DSPAM_CTX * CTX)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  struct _ds_storage_signature *st;
  unsigned long length;
  char query[128];
  unsigned char *mem;
  char *err=NULL;
  const char **row, *query_tail=NULL;
  int ncolumn, x;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_get_nextsignature: invalid database handle (NULL)");
    return NULL;
  }

  st = calloc (1, sizeof (struct _ds_storage_signature));
  if (st == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  if (s->iter_sig == NULL)
  {
    snprintf (query, sizeof (query),
              "select data, signature, strftime('%%s', created_on), "
              "length(data) from dspam_signature_data");

   if ((sqlite_compile(s->dbh, query, &query_tail, &s->iter_sig, &err))
        !=SQLITE_OK)
    {
      _sqlite_drv_query_error (err, query);
      free(st);
      return NULL;
    }
  }
                                                                                
  if ((x = sqlite_step(s->iter_sig, &ncolumn, &row, NULL))
      !=SQLITE_ROW) {
    if (x != SQLITE_DONE) {
      _sqlite_drv_query_error (err, query);
      s->iter_sig = NULL;
      free(st);
      return NULL;
    }
    sqlite_finalize((struct sqlite_vm *) s->iter_sig, &err);
    s->iter_sig = NULL;
    free(st);
    return NULL;
  }

  length = strtol(row[3], NULL, 0);
  if (length == 0)
  {
    return _ds_get_nextsignature(CTX);
  }

 mem = malloc (length+1);
  if (mem == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    sqlite_finalize(s->iter_sig, &err);
    s->iter_sig = NULL;
    free(st);
    return NULL;
  }

  length = sqlite_decode_binary((const unsigned char *) &row[ncolumn], mem);
  if (length<0) {
    LOG(LOG_ERR, "sqlite_decode_binary() failed with error %d", length);
    s->iter_sig = NULL;
    free(st);
    return NULL;
  }

  st->data = realloc(mem, length);
  strlcpy(st->signature, row[1], sizeof(st->signature));
  st->length = length;
  st->created_on = (time_t) strtol(row[2], NULL, 0);

  return st;
}

void
_sqlite_drv_query_error (const char *error, const char *query)
{
  FILE *file;
  time_t tm = time (NULL);
  char ct[128];
  char fn[MAX_FILENAME_LENGTH];

  LOG (LOG_WARNING, "query error: %s: see sql.errors for more details",
       error);

  snprintf (fn, sizeof (fn), "%s/sql.errors", LOGDIR);

  snprintf (ct, sizeof (ct), "%s", ctime (&tm));
  chomp (ct);

  file = fopen (fn, "a");

  if (file == NULL)
  {
    LOG(LOG_ERR, ERR_IO_FILE_WRITE, fn, strerror (errno));
  }
  else
  {
    fprintf (file, "[%s] %d: %s: %s\n", ct, (int) getpid (), error, query);
    fclose (file);
  }

  free((char *)error);
  return;
}

int
_ds_del_spamrecord (DSPAM_CTX * CTX, unsigned long long token)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  char query[128];
  char *err=NULL;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_delete_signature: invalid database handle (NULL)");
    return EINVAL;
  }
                                                                                
  snprintf (query, sizeof (query),
            "delete from dspam_token_data where token = \"%" LLU_FMT_SPEC "\"",
            token);
                                                                                
  if ((sqlite_exec(s->dbh, query, NULL, NULL, &err))!=SQLITE_OK)
  {
    _sqlite_drv_query_error (err, query);
    return EFAILURE;
  }

  return 0;
}

int _ds_delall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _sqlite_drv_storage *s = (struct _sqlite_drv_storage *) CTX->storage;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  buffer *query;
  char *err=NULL;
  char scratch[1024];
  char queryhead[1024];
  int writes = 0;

  if (diction->items < 1)
    return 0;

  if (s->dbh == NULL)
  {
    LOGDEBUG ("_ds_delall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  snprintf (queryhead, sizeof(queryhead),
            "delete from dspam_token_data "
            "where token in(");

  buffer_cat (query, queryhead);

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while (ds_term)
  {
    snprintf (scratch, sizeof (scratch), "'%" LLU_FMT_SPEC "'", ds_term->key);
    buffer_cat (query, scratch);
    ds_term = ds_diction_next(ds_c);
   
    if (writes > 2500 || ds_term == NULL) {
      buffer_cat (query, ")");

      if ((sqlite_exec(s->dbh, query->data, NULL, NULL, &err))!=SQLITE_OK)
      {
        _sqlite_drv_query_error (err, query->data);
        buffer_destroy(query);
        return EFAILURE;
      }

      buffer_copy(query, queryhead);
      writes = 0;
   
    } else { 
      writes++;
      if (ds_term)
        buffer_cat (query, ",");
    }
  }
  ds_diction_close(ds_c);

  if (writes) {
    buffer_cat (query, ")");

    if ((sqlite_exec(s->dbh, query->data, NULL, NULL, &err))!=SQLITE_OK)
    {
      _sqlite_drv_query_error (err, query->data);
      buffer_destroy(query);
      return EFAILURE;
    }
  }

  buffer_destroy (query);
  return 0;
}

void *_ds_connect (DSPAM_CTX *CTX)
{
  return NULL;
}


/* Preference Stubs for Flat-File */

agent_pref_t _ds_pref_load(config_t config, const char *user,
  const char *home, void *dbh)
{
  return _ds_ff_pref_load(config, user, home, dbh);
}

int _ds_pref_set(config_t config, const char *user, const char *home,
  const char *attrib, const char *value, void *dbh)
{
  return _ds_ff_pref_set(config, user, home, attrib, value, dbh);
}

int _ds_pref_del(config_t config, const char *user, const char *home,
  const char *attrib, void *dbh)
{
  return _ds_ff_pref_del(config, user, home, attrib, dbh);
}

