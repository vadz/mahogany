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
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <mysql.h>
#ifdef DAEMON
#include <pthread.h>
#endif

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

#ifdef USE_LDAP
#include "ldap_client.h"
#endif

#include "storage_driver.h"
#include "mysql_drv.h"
#include "libdspam.h"
#include "config.h"
#include "error.h"
#include "language.h"
#include "util.h"
#include "pref.h"
#include "config_shared.h"

#define MYSQL_RUN_QUERY(A, B) mysql_query(A, B)

int
dspam_init_driver (DRIVER_CTX *DTX) 
{
#if defined(MYSQL4_INITIALIZATION) && MYSQL_VERSION_ID >= 40001
  const char *server_default_groups[]=
  { "server", "embedded", "mysql_SERVER", 0 };

  if (mysql_server_init(0, NULL, (char**) server_default_groups)) {
    LOGDEBUG("dspam_init_driver() failed");
    return EFAILURE;
  }
#endif

  if (DTX == NULL)
    return 0;

  /* Establish a series of stateful connections */

  if (DTX->flags & DRF_STATEFUL) {
    int i, connection_cache = 3;

    if (_ds_read_attribute(DTX->CTX->config->attributes, "MySQLConnectionCache"))
      connection_cache = atoi(_ds_read_attribute(DTX->CTX->config->attributes, "MySQLConnectionCache"));

    DTX->connection_cache = connection_cache;
    DTX->connections = calloc(1, sizeof(struct _ds_drv_connection *)*connection_cache); 
    if (DTX->connections == NULL) {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      return EUNKNOWN;
    }

    for(i=0;i<connection_cache;i++) {
      DTX->connections[i] = calloc(1, sizeof(struct _ds_drv_connection));
      if (DTX->connections[i]) {
#ifdef DAEMON
        LOGDEBUG("initializing lock %d", i);
        pthread_mutex_init(&DTX->connections[i]->lock, NULL);
#endif
        DTX->connections[i]->dbh = (void *) _ds_connect(DTX->CTX);
      }
    }
  }

  return 0;
}

int
dspam_shutdown_driver (DRIVER_CTX *DTX)
{
  if (DTX != NULL) {
    if (DTX->flags & DRF_STATEFUL && DTX->connections) {
      int i;

      for(i=0;i<DTX->connection_cache;i++) {
        if (DTX->connections[i]) {
          if (DTX->connections[i]->dbh) {
            _mysql_drv_dbh_t dbt = (_mysql_drv_dbh_t) DTX->connections[i]->dbh;
            mysql_close(dbt->dbh_read);
            if (dbt->dbh_write != dbt->dbh_read)
              mysql_close(dbt->dbh_write);
          }
             
#ifdef DAEMON
          LOGDEBUG("destroying lock %d", i);
          pthread_mutex_destroy(&DTX->connections[i]->lock);
#endif
          free(DTX->connections[i]);
        }
      }

      free(DTX->connections);
      DTX->connections = NULL;
    }
  }

#if defined(MYSQL4_INITIALIZATION) && MYSQL_VERSION_ID >= 40001
  mysql_server_end();
#endif
  return 0;
}

int
_mysql_drv_get_spamtotals (DSPAM_CTX * CTX)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  char query[1024];
  struct passwd *p;
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct _ds_spam_totals user, group;
  int uid = -1, gid = -1;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_mysql_drv_get_spamtotals: invalid database handle (NULL)");
    return EINVAL;
  }

  memset(&s->control_totals, 0, sizeof(struct _ds_spam_totals));
  if (CTX->flags & DSF_MERGED) {
    memset(&s->merged_totals, 0, sizeof(struct _ds_spam_totals));
    memset(&group, 0, sizeof(struct _ds_spam_totals));
  }

  memset(&CTX->totals, 0, sizeof(struct _ds_spam_totals));
  memset(&user, 0, sizeof(struct _ds_spam_totals));

  if (!CTX->group || CTX->flags & DSF_MERGED) 
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else 
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_mysql_drv_get_spamtotals: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    if (!(CTX->flags & DSF_MERGED))
      return EINVAL;
  } else {

    uid = p->pw_uid;
  }

  if (CTX->flags & DSF_MERGED) {
    p = _mysql_drv_getpwnam (CTX, CTX->group);
    if (p == NULL)
    {
      LOGDEBUG ("_mysql_drv_getspamtotals: unable to _mysql_drv_getpwnam(%s)",
                CTX->group);
      return EINVAL;
    }

  }

  gid = p->pw_uid;

  snprintf (query, sizeof (query),
            "select uid, spam_learned, innocent_learned, "
            "spam_misclassified, innocent_misclassified, "
            "spam_corpusfed, innocent_corpusfed, "
            "spam_classified, innocent_classified "
            " from dspam_stats where (uid = %d or uid = %d)",
            uid, gid);
  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    return EFAILURE;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL) {
    LOGDEBUG("mysql_use_result() failed in _ds_get_spamtotals()");
    return EFAILURE;
  }

  while ((row = mysql_fetch_row (result)) != NULL) {
    int rid = atoi(row[0]);
    if (rid == uid) {
      user.spam_learned			= strtol (row[1], NULL, 0);
      user.innocent_learned		= strtol (row[2], NULL, 0);
      user.spam_misclassified		= strtol (row[3], NULL, 0);
      user.innocent_misclassified  	= strtol (row[4], NULL, 0);
      user.spam_corpusfed		= strtol (row[5], NULL, 0);
      user.innocent_corpusfed		= strtol (row[6], NULL, 0);
      if (row[7] != NULL && row[8] != NULL) {
        user.spam_classified		= strtol (row[7], NULL, 0);
        user.innocent_classified	= strtol (row[8], NULL, 0);
      } else {
        user.spam_classified = 0;
        user.innocent_classified = 0;
      }
    } else {
      group.spam_learned           = strtol (row[1], NULL, 0);
      group.innocent_learned       = strtol (row[2], NULL, 0);
      group.spam_misclassified     = strtol (row[3], NULL, 0);
      group.innocent_misclassified = strtol (row[4], NULL, 0);
      group.spam_corpusfed         = strtol (row[5], NULL, 0);
      group.innocent_corpusfed     = strtol (row[6], NULL, 0);
    if (row[7] != NULL && row[8] != NULL) {
        group.spam_classified      = strtol (row[7], NULL, 0);
        group.innocent_classified  = strtol (row[8], NULL, 0);
      } else {
      group.spam_classified = 0;
          group.innocent_classified = 0;
      }
    }
  }

  mysql_free_result (result);

  if (CTX->flags & DSF_MERGED) {
    memcpy(&s->merged_totals, &group, sizeof(struct _ds_spam_totals));
    memcpy(&s->control_totals, &user, sizeof(struct _ds_spam_totals));
    CTX->totals.spam_learned 
      = user.spam_learned + group.spam_learned;
    CTX->totals.innocent_learned 
      = user.innocent_learned + group.innocent_learned;
    CTX->totals.spam_misclassified 
      = user.spam_misclassified + group.spam_misclassified;
    CTX->totals.innocent_misclassified
      = user.innocent_misclassified + group.innocent_misclassified;
    CTX->totals.spam_corpusfed
      = user.spam_corpusfed + group.spam_corpusfed;
    CTX->totals.innocent_corpusfed
      = user.innocent_corpusfed + group.innocent_corpusfed;
    CTX->totals.spam_classified
      = user.spam_classified + group.spam_classified;
    CTX->totals.innocent_classified
      = user.innocent_classified + group.innocent_classified;
  } else {
    memcpy(&CTX->totals, &user, sizeof(struct _ds_spam_totals));
    memcpy(&s->control_totals, &user, sizeof(struct _ds_spam_totals));
  }

  return 0;
}

int
_mysql_drv_set_spamtotals (DSPAM_CTX * CTX)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  char query[1024];
  int result = 0;
  struct _ds_spam_totals user;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_mysql_drv_set_spamtotals: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY)
  {
    _mysql_drv_get_spamtotals (CTX);    /* undo changes to in memory totals */
    return 0;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_mysql_drv_get_spamtotals: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  /* Subtract the group totals from our active set */
  if (CTX->flags & DSF_MERGED) {
    memcpy(&user, &CTX->totals, sizeof(struct _ds_spam_totals));
    CTX->totals.innocent_learned -= s->merged_totals.innocent_learned;
    CTX->totals.spam_learned -= s->merged_totals.spam_learned;
    CTX->totals.innocent_misclassified -= s->merged_totals.innocent_misclassified;
    CTX->totals.spam_misclassified -= s->merged_totals.spam_misclassified;
    CTX->totals.innocent_corpusfed -= s->merged_totals.innocent_corpusfed;
    CTX->totals.spam_corpusfed -= s->merged_totals.spam_corpusfed;
    CTX->totals.innocent_classified -= s->merged_totals.innocent_classified;
    CTX->totals.spam_classified -= s->merged_totals.spam_classified;

    if (CTX->totals.innocent_learned < 0)
      CTX->totals.innocent_learned = 0;
    if (CTX->totals.spam_learned < 0)
      CTX->totals.spam_learned = 0;

  }

  result = -1;

  if (s->control_totals.innocent_learned == 0)
  {
    snprintf (query, sizeof (query),
              "insert into dspam_stats(uid, spam_learned, innocent_learned, "
              "spam_misclassified, innocent_misclassified, "
              "spam_corpusfed, innocent_corpusfed, "
              "spam_classified, innocent_classified) "
              "values(%d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld)",
              (int) p->pw_uid, CTX->totals.spam_learned,
              CTX->totals.innocent_learned, CTX->totals.spam_misclassified,
              CTX->totals.innocent_misclassified, CTX->totals.spam_corpusfed,
              CTX->totals.innocent_corpusfed, CTX->totals.spam_classified,
              CTX->totals.innocent_classified);
    result = MYSQL_RUN_QUERY (s->dbt->dbh_write, query);
  }

  if (result)
  {
    snprintf (query, sizeof (query),
              "update dspam_stats set spam_learned = spam_learned %s %d, "
              "innocent_learned = innocent_learned %s %d, "
              "spam_misclassified = spam_misclassified %s %d, "
              "innocent_misclassified = innocent_misclassified %s %d, "
              "spam_corpusfed = spam_corpusfed %s %d, "
              "innocent_corpusfed = innocent_corpusfed %s %d, "
              "spam_classified = spam_classified %s %d, "
              "innocent_classified = innocent_classified %s %d "
              "where uid = %d",
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
                  s->control_totals.innocent_classified), (int) p->pw_uid);

    if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
      if (CTX->flags & DSF_MERGED)
        memcpy(&CTX->totals, &user, sizeof(struct _ds_spam_totals));
      return EFAILURE;
    }
  }

  if (CTX->flags & DSF_MERGED)
    memcpy(&CTX->totals, &user, sizeof(struct _ds_spam_totals));

  return 0;
}

int
_ds_getall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  buffer *query;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  char scratch[1024];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct _ds_spam_stat stat;
  unsigned long long token = 0;
  int get_one = 0;
  int uid, gid;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_getall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_getall_spamrecords: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  uid = p->pw_uid;
                                                                                
  if (CTX->flags & DSF_MERGED) {
    p = _mysql_drv_getpwnam (CTX, CTX->group);
    if (p == NULL)
    {
      LOGDEBUG ("_ds_getall_spamrecords: unable to _mysql_drv_getpwnam(%s)",
                CTX->group);
      return EINVAL;
    }
  }
                                                                                
  gid = p->pw_uid;

  stat.spam_hits     = 0;
  stat.innocent_hits = 0;
  stat.probability   = 0.00000;

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  if (uid != gid) {
    snprintf (scratch, sizeof (scratch),
            "select uid, token, spam_hits, innocent_hits "
            "from dspam_token_data where (uid = %d or uid = %d) and token in(",
            uid, gid);
  } else {
    snprintf (scratch, sizeof (scratch),
            "select uid, token, spam_hits, innocent_hits "
            "from dspam_token_data where uid = %d and token in(",
            uid);
  }
  buffer_cat (query, scratch);

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while (ds_term)
  {
    if (_ds_match_attribute(CTX->config->attributes, "MySQLSupressQuote", "on"))
      snprintf(scratch, sizeof(scratch), "%llu", ds_term->key);
    else
      snprintf (scratch, sizeof (scratch), "'%llu'", ds_term->key);
    buffer_cat (query, scratch);
    ds_term->s.innocent_hits = 0;
    ds_term->s.spam_hits = 0;
    ds_term->s.probability = 0.00000;
    ds_term->s.status = 0;
    ds_term = ds_diction_next(ds_c);
    if (ds_term)
      buffer_cat (query, ",");
    get_one = 1;
  }
  ds_diction_close(ds_c);
  buffer_cat (query, ")");

#ifdef VERBOSE
  LOGDEBUG ("mysql query length: %ld\n", query->used);
  _mysql_drv_query_error ("VERBOSE DEBUG (INFO ONLY - NOT AN ERROR)", query->data);
#endif

  if (!get_one) 
    return 0;

  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query->data))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query->data);
    buffer_destroy(query);
    return EFAILURE;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL) {
    buffer_destroy(query);
    LOGDEBUG("mysql_use_result() failed in _ds_getall_spamrecords()"); 
    return EFAILURE;
  }

  while ((row = mysql_fetch_row (result)) != NULL)
  {
    int rid = atoi(row[0]);
    token = strtoull (row[1], NULL, 0);
    stat.spam_hits = strtol (row[2], NULL, 0);
    stat.innocent_hits = strtol (row[3], NULL, 0);
    stat.status = 0;

    if (rid == uid) 
      stat.status |= TST_DISK;

    ds_diction_addstat(diction, token, &stat);
  }

  /* Control token */
  stat.spam_hits = 10;
  stat.innocent_hits = 10;
  stat.status = 0;
  ds_diction_touch(diction, CONTROL_TOKEN, "$$CONTROL$$", 0);
  ds_diction_addstat(diction, CONTROL_TOKEN, &stat);
  s->control_token = CONTROL_TOKEN;
  s->control_ih = 10;
  s->control_sh = 10;

  mysql_free_result (result);
  buffer_destroy (query);
  return 0;
}

int
_ds_setall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct _ds_spam_stat control, stat;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  buffer *query;
  char scratch[1024];
  struct passwd *p;
  int update_any = 0;
#if MYSQL_VERSION_ID >= 40100
  buffer *insert;
  int insert_any = 0;
#endif

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_setall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY && 
      (CTX->training_mode != DST_TOE || 
        (diction->whitelist_token == 0 && (!(CTX->flags & DSF_NOISE)))))
    return 0;

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_setall_spamrecords: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

#if MYSQL_VERSION_ID >= 40100
  insert = buffer_create(NULL);
  if (insert == NULL)
  {
    buffer_destroy(query);
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }
#endif

  ds_diction_getstat(diction, s->control_token, &control);
  snprintf (scratch, sizeof (scratch),
            "update dspam_token_data set last_hit = current_date(), "
            "spam_hits = greatest(0, spam_hits %s %d), "
            "innocent_hits = greatest(0, innocent_hits %s %d) "
            "where uid = %d and token in(",
            (control.spam_hits > s->control_sh) ? "+" : "-",
            abs (control.spam_hits - s->control_sh),
            (control.innocent_hits > s->control_ih) ? "+" : "-",
            abs (control.innocent_hits - s->control_ih), (int) p->pw_uid);

  buffer_cat (query, scratch);

#if MYSQL_VERSION_ID >= 40100
  buffer_copy (insert, "insert into dspam_token_data(uid, token, spam_hits, "
                       "innocent_hits, last_hit) values");
#endif

  /*
   *  Add each token in the diction to either an update or an insert queue 
   */

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    int use_comma = 0;
    if (ds_term->key == s->control_token) {
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    /* Don't write lexical tokens if we're in TOE mode classifying */

    if (CTX->training_mode == DST_TOE            && 
        CTX->operating_mode == DSM_CLASSIFY      &&
        ds_term->key != diction->whitelist_token &&
        (!ds_term->name || strncmp(ds_term->name, "bnr.", 4)))
    {
      ds_term = ds_diction_next(ds_c);
      continue;
    }
      
    ds_diction_getstat(diction, ds_term->key, &stat);

    /* Changed tokens are marked as "dirty" by libdspam */

    if (!(stat.status & TST_DIRTY)) {
      ds_term = ds_diction_next(ds_c);
      continue;
    } else {
      stat.status &= ~TST_DIRTY;
    }

    /* This token wasn't originally loaded from disk, so try an insert */

    if (!(stat.status & TST_DISK))
    {
      char ins[1024];
#if MYSQL_VERSION_ID >= 40100
      snprintf (ins, sizeof (ins),
                "%s(%d, '%llu', %d, %d, current_date())",
                 (insert_any) ? ", " : "",
                 (int) p->pw_uid,
                 ds_term->key,
                 stat.spam_hits > 0 ? 1 : 0,
                 stat.innocent_hits > 0 ? 1 : 0);

      insert_any = 1;
      buffer_cat(insert, ins);
#else
      snprintf(ins, sizeof (ins),
               "insert into dspam_token_data(uid, token, spam_hits, "
               "innocent_hits, last_hit) values(%d, '%llu', %d, %d, "
               "current_date())",
               p->pw_uid,
               ds_term->key,
               stat.spam_hits > 0 ? 1 : 0,
               stat.innocent_hits > 0 ? 1 : 0);

      if (MYSQL_RUN_QUERY (s->dbt->dbh_write, ins))
        stat.status |= TST_DISK;
#endif
    }

    if (stat.status & TST_DISK) {
    if (_ds_match_attribute(CTX->config->attributes, "MySQLSupressQuote", "on"))
      snprintf (scratch, sizeof (scratch), "%llu", ds_term->key);
    else
      snprintf (scratch, sizeof (scratch), "'%llu'", ds_term->key);

      buffer_cat (query, scratch);
      update_any = 1;
      use_comma = 1;
    }
 
    ds_term->s.status |= TST_DISK;

    ds_term = ds_diction_next(ds_c);
    if (ds_term && use_comma)
      buffer_cat (query, ",");
  }
  ds_diction_close(ds_c);

  /* Just incase */

  if (query->used && query->data[strlen (query->data) - 1] == ',')
  {
    query->used--;
    query->data[strlen (query->data) - 1] = 0;
  }

  buffer_cat (query, ")");

  LOGDEBUG("Control: [%ld %ld] [%ld %ld] Delta: [%ld %ld]",
    s->control_sh, s->control_ih, 
    control.spam_hits, control.innocent_hits,
    control.spam_hits - s->control_sh, control.innocent_hits - s->control_ih);

  if (update_any)
  {
    if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query->data))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query->data);
      buffer_destroy(query);
      return EFAILURE;
    }
  }

#if MYSQL_VERSION_ID >= 40100
  if (insert_any)
  {
     snprintf (scratch, sizeof (scratch),
            " ON DUPLICATE KEY UPDATE last_hit = current_date(), "
            "spam_hits = greatest(0, spam_hits %s %d), "
            "innocent_hits = greatest(0, innocent_hits %s %d) ",
            (control.spam_hits > s->control_sh) ? "+" : "-",
            abs (control.spam_hits - s->control_sh) > 0 ? 1 : 0,
            (control.innocent_hits > s->control_ih) ? "+" : "-",
            abs (control.innocent_hits - s->control_ih) > 0 ? 1 : 0);

    buffer_cat(insert, scratch);
    if (MYSQL_RUN_QUERY (s->dbt->dbh_write, insert->data))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), insert->data);
      buffer_destroy(insert);
      return EFAILURE;
    }
  }

  buffer_destroy(insert);
#endif

  buffer_destroy (query);
  return 0;
}

int
_ds_get_spamrecord (DSPAM_CTX * CTX, unsigned long long token,
                    struct _ds_spam_stat *stat)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  char query[1024];
  struct passwd *p;
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_get_spamrecord: invalid database handle (NULL)");
    return EINVAL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_get_spamrecord: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  if (_ds_match_attribute(CTX->config->attributes, "MySQLSupressQuote", "on"))
    snprintf (query, sizeof (query),
            "select spam_hits, innocent_hits from dspam_token_data "
            "where uid = %d " "and token in(%llu) ", (int) p->pw_uid, token);
  else
    snprintf (query, sizeof (query),
            "select spam_hits, innocent_hits from dspam_token_data "
            "where uid = %d " "and token in('%llu') ", (int) p->pw_uid, token);

  stat->probability = 0.00000;
  stat->spam_hits = 0;
  stat->innocent_hits = 0;
  stat->status &= ~TST_DISK;

  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    return EFAILURE;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL) {
    LOGDEBUG("mysql_use_result() failed in _ds_get_spamrecord()"); 
    return EFAILURE;
  }

  row = mysql_fetch_row (result);
  if (row == NULL)
  {
    mysql_free_result (result);
    return 0;
  }

  stat->spam_hits = strtol (row[0], NULL, 0);
  stat->innocent_hits = strtol (row[1], NULL, 0);
  stat->status |= TST_DISK;
  mysql_free_result (result);
  return 0;
}

int
_ds_set_spamrecord (DSPAM_CTX * CTX, unsigned long long token,
                    struct _ds_spam_stat *stat)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  char query[1024];
  struct passwd *p;
  int result = 0;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_set_spamrecord: invalid database handle (NULL)");
    return EINVAL;
  }

  if (CTX->operating_mode == DSM_CLASSIFY)
    return 0;

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_set_spamrecord: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  /* It's either not on disk or the caller isn't using stat.status */
  if (!(stat->status & TST_DISK))
  {
    snprintf (query, sizeof (query),
              "insert into dspam_token_data(uid, token, spam_hits, "
              "innocent_hits, last_hit)"
              " values(%d, '%llu', %ld, %ld, current_date())",
              (int) p->pw_uid, token, stat->spam_hits, stat->innocent_hits);
    result = MYSQL_RUN_QUERY (s->dbt->dbh_write, query);
  }

  if ((stat->status & TST_DISK) || result)
  {
    /* insert failed; try updating instead */
    snprintf (query, sizeof (query), "update dspam_token_data "
              "set spam_hits = %ld, "
              "innocent_hits = %ld "
              "where uid = %d "
              "and token = %lld", stat->spam_hits,
              stat->innocent_hits, (int) p->pw_uid, token);

    if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
      return EFAILURE;
    }
  }

  return 0;
}

int
_ds_init_storage (DSPAM_CTX * CTX, void *dbh)
{
  struct _mysql_drv_storage *s;
  _mysql_drv_dbh_t dbt = (_mysql_drv_dbh_t) dbh;

  if (CTX == NULL) {
    return EINVAL;
  }

  /* don't init if we're already initted */
  if (CTX->storage != NULL)
  {
    LOGDEBUG ("_ds_init_storage: storage already initialized");
    return EINVAL;
  }

  if (dbt) {
    if (mysql_ping(dbt->dbh_read))
      return EFAILURE;
  }

  s = calloc (1, sizeof (struct _mysql_drv_storage));
  if (s == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  s->dbh_attached = (dbt) ? 1 : 0;
  s->u_getnextuser[0] = 0;
  memset(&s->p_getpwnam, 0, sizeof(struct passwd));
  memset(&s->p_getpwuid, 0, sizeof(struct passwd));

  if (dbt)
    s->dbt = dbt;
  else
    s->dbt = _ds_connect(CTX);

  if (s->dbt == NULL)
  {
    LOGDEBUG
      ("_ds_init_storage: mysql_init: unable to initialize handle to database");
    free(s);
    return EUNKNOWN;
  }

  CTX->storage = s;

  s->control_token = 0;
  s->control_ih = 0;
  s->control_sh = 0;

  /* get spam totals on successful init */
  if (CTX->username != NULL)
  {
      if (_mysql_drv_get_spamtotals (CTX))
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
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;

  if (CTX->storage == NULL)
  {
    LOGDEBUG ("_ds_shutdown_storage: called with NULL storage handle");
    return EINVAL;
  }

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_shutdown_storage: invalid database handle (NULL)");
    return EINVAL;
  }

  /* Store spam totals on shutdown */
  if (CTX->username != NULL && CTX->operating_mode != DSM_CLASSIFY)
  {
      _mysql_drv_set_spamtotals (CTX);
  }

  if (! s->dbh_attached) {
    mysql_close (s->dbt->dbh_read);
    if (s->dbt->dbh_write != s->dbt->dbh_read)
      mysql_close (s->dbt->dbh_write);
  }
  s->dbt = NULL;

  if (s->p_getpwnam.pw_name)
    free(s->p_getpwnam.pw_name);
  if (s->p_getpwuid.pw_name)
    free(s->p_getpwuid.pw_name);
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
  struct passwd *p;

  pid = getpid ();
  if (_ds_match_attribute(CTX->config->attributes, "MySQLUIDInSignature", "on"))
  {
    if (!CTX->group || CTX->flags & DSF_MERGED)
      p = _mysql_drv_getpwnam (CTX, CTX->username);
    else
      p = _mysql_drv_getpwnam (CTX, CTX->group);

    if (!p) {
      LOG(LOG_ERR, "Unable to determine UID for %s", CTX->username);
      return EINVAL;
    }
    snprintf (session, sizeof (session), "%d,%8lx%d", (int) p->pw_uid, 
              (long) time(NULL), pid);
  }
  else
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
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  unsigned long *lengths;
  char *mem;
  char query[128];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int uid;
  MYSQL *dbh;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_get_signature: invalid database handle (NULL)");
    return EINVAL;
  }

  dbh = _mysql_drv_sig_write_handle(CTX, s);

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_get_signature: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  if (_ds_match_attribute(CTX->config->attributes, "MySQLUIDInSignature", "on"))
  {
    char *u, *sig, *username;
    void *dbt = s->dbt;
    int dbh_attached = s->dbh_attached;
    sig = strdup(signature);
    u = strchr(sig, ',');
    if (!u) {
      LOGDEBUG("unable to locate uid in signature");
      return EFAILURE;
    }
    u[0] = 0;
    u = sig;
    uid = atoi(u);
    free(sig);

    /* Change the context's username and reinitialize storage */

    p = _mysql_drv_getpwuid (CTX, uid);
    if (p == NULL) {
      LOG(LOG_CRIT, "_ds_get_signature(): _mysql_drv_getpwuid(%d) failed: aborting", uid);
      return EFAILURE;
    }

    username = strdup(p->pw_name);
    _ds_shutdown_storage(CTX);
    free(CTX->username);
    CTX->username = username;
    _ds_init_storage(CTX, (dbh_attached) ? dbt : NULL);
    s = (struct _mysql_drv_storage *) CTX->storage;

    dbh = _mysql_drv_sig_write_handle(CTX, s);
  } else {
    uid = p->pw_uid;
  }

  snprintf (query, sizeof (query),
          "select data, length from dspam_signature_data "
          "where uid = %d and signature = \"%s\"", uid, signature);

  if (mysql_real_query (dbh, query, strlen (query)))
  {
    _mysql_drv_query_error (mysql_error (dbh), query);
    return EFAILURE;
  }

  result = mysql_use_result (dbh);
  if (result == NULL) {
    LOGDEBUG("mysql_use_result() failed in _ds_get_signature");
    return EFAILURE;
  }

  row = mysql_fetch_row (result);
  if (row == NULL)
  {
    mysql_free_result (result);
    LOGDEBUG("mysql_fetch_row() failed in _ds_get_signature");
    return EFAILURE;
  }

  lengths = mysql_fetch_lengths (result);
  if (lengths == NULL || lengths[0] == 0)
  {
    mysql_free_result (result);
    LOGDEBUG("mysql_fetch_lengths() failed in _ds_get_signature");
    return EFAILURE;
  }

  mem = malloc (lengths[0]);
  if (mem == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    mysql_free_result (result);
    return EUNKNOWN;
  }

  memcpy (mem, row[0], lengths[0]);
  SIG->data = mem;
  SIG->length = strtol (row[1], NULL, 0);

  mysql_free_result (result);

  return 0;
}

int
_ds_set_signature (DSPAM_CTX * CTX, struct _ds_spam_signature *SIG,
                   const char *signature)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  unsigned long length;
  char *mem;
  char scratch[1024];
  buffer *query;
  struct passwd *p;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_set_signature; invalid database handle (NULL)");
    return EINVAL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_set_signature: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  query = buffer_create (NULL);
  if (query == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  mem = calloc(1, SIG->length*3);
  if (mem == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    buffer_destroy(query);
    return EUNKNOWN;
  }

  length = mysql_real_escape_string (s->dbt->dbh_write, mem, SIG->data, SIG->length);

  snprintf (scratch, sizeof (scratch),
            "insert into dspam_signature_data(uid, signature, length, created_on, data) values(%d, \"%s\", %ld, current_date(), \"",
            (int) p->pw_uid, signature, SIG->length);
  buffer_cat (query, scratch);
  buffer_cat (query, mem);
  buffer_cat (query, "\")");

  if (mysql_real_query (s->dbt->dbh_write, query->data, query->used))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query->data);
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
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  char query[128];

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_delete_signature: invalid database handle (NULL)");
    return EINVAL;
  }


  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_delete_signature: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  snprintf (query, sizeof (query),
            "delete from dspam_signature_data where uid = %d and signature = \"%s\"",
            (int) p->pw_uid, signature);
  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    return EFAILURE;
  }

  return 0;
}

int
_ds_verify_signature (DSPAM_CTX * CTX, const char *signature)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  char query[128];
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_verify_signature: invalid database handle (NULL)");
    return EINVAL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_verisy_signature: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }

  snprintf (query, sizeof (query),
      "select signature from dspam_signature_data "
      "where uid = %d and signature = \"%s\"",
            (int) p->pw_uid, signature);
  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    return EFAILURE;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL) {
    return -1;
  }

  row = mysql_fetch_row (result);
  if (row == NULL)
  {
    mysql_free_result (result);
    return -1;
  }

  mysql_free_result (result);
  return 0;
}

char *
_ds_get_nextuser (DSPAM_CTX * CTX)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
#ifndef VIRTUAL_USERS
  struct passwd *p;
  uid_t uid;
#else
  char *virtual_table, *virtual_uid, *virtual_username;
#endif
  char query[128];
  MYSQL_ROW row;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_get_nextuser: invalid database handle (NULL)");
    return NULL;
  }

#ifdef VIRTUAL_USERS
  if ((virtual_table 
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualTable"))==NULL)
  { virtual_table = "dspam_virtual_uids"; }

  if ((virtual_uid 
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualUIDField"))==NULL)
  { virtual_uid = "uid"; }

  if ((virtual_username = _ds_read_attribute(CTX->config->attributes, 
    "MySQLVirtualUsernameField")) ==NULL)
  { virtual_username = "username"; } 
#endif

  if (s->iter_user == NULL)
  {
#ifdef VIRTUAL_USERS
    snprintf(query, sizeof(query), "select distinct %s from %s", 
      virtual_username, 
      virtual_table);
#else
    strcpy (query, "select distinct uid from dspam_stats");
#endif
    if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
      return NULL;
    }

    s->iter_user = mysql_use_result (s->dbt->dbh_read);
    if (s->iter_user == NULL)
      return NULL;
  }

  row = mysql_fetch_row (s->iter_user);
  if (row == NULL)
  {
    mysql_free_result (s->iter_user);
    s->iter_user = NULL;
    return NULL;
  }

#ifdef VIRTUAL_USERS
  strlcpy (s->u_getnextuser, row[0], sizeof (s->u_getnextuser));
#else
  uid = (uid_t) atoi (row[0]);
  p = _mysql_drv_getpwuid (CTX, uid);
  if (p == NULL)
  {
    mysql_free_result (s->iter_user);
    s->iter_user = NULL;
    return NULL;
  }

  strlcpy (s->u_getnextuser, p->pw_name, sizeof (s->u_getnextuser));
#endif

  return s->u_getnextuser;
}

struct _ds_storage_record *
_ds_get_nexttoken (DSPAM_CTX * CTX)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct _ds_storage_record *st;
  char query[128];
  MYSQL_ROW row;
  struct passwd *p;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_get_nexttoken: invalid database handle (NULL)");
    return NULL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_get_nexttoken: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
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
              "select token, spam_hits, innocent_hits, unix_timestamp(last_hit) from dspam_token_data where uid = %d",
              (int) p->pw_uid);
    if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
      free(st);
      return NULL;
    }

    s->iter_token = mysql_use_result (s->dbt->dbh_read);
    if (s->iter_token == NULL) {
      free(st);
      return NULL;
    }
  }

  row = mysql_fetch_row (s->iter_token);
  if (row == NULL)
  {
    mysql_free_result (s->iter_token);
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
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct _ds_storage_signature *st;
  unsigned long *lengths;
  char query[128];
  MYSQL_ROW row;
  struct passwd *p;
  char *mem;

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_get_nextsignature: invalid database handle (NULL)");
    return NULL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_get_nextsignature: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
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
              "select data, signature, length, unix_timestamp(created_on) from dspam_signature_data where uid = %d",
              (int) p->pw_uid);
    if (mysql_real_query (s->dbt->dbh_read, query, strlen (query)))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
      free(st);
      return NULL;
    }

    s->iter_sig = mysql_use_result (s->dbt->dbh_read);
    if (s->iter_sig == NULL) {
      free(st); 
      return NULL;
    }
  }

  row = mysql_fetch_row (s->iter_sig);
  if (row == NULL)
  {
    mysql_free_result (s->iter_sig);
    s->iter_sig = NULL;
    free(st);
    return NULL;
  }

  lengths = mysql_fetch_lengths (s->iter_sig);
  if (lengths == NULL || lengths[0] == 0)
  {
    mysql_free_result (s->iter_sig);
    free(st);
    return NULL;
  }

  mem = malloc (lengths[0]);
  if (mem == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    mysql_free_result (s->iter_sig);
    free(st);
    return NULL;
  }

  memcpy (mem, row[0], lengths[0]);
  st->data = mem;
  strlcpy (st->signature, row[1], sizeof (st->signature));
  st->length = strtol (row[2], NULL, 0);
  st->created_on = (time_t) strtol (row[3], NULL, 0);

  return st;
}

struct passwd *
_mysql_drv_getpwnam (DSPAM_CTX * CTX, const char *name)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;

#ifndef VIRTUAL_USERS
  struct passwd *q;
#if defined(_REENTRANT) && defined(HAVE_GETPWNAM_R)
  struct passwd pwbuf;
  char buf[1024];
#endif

  if (name == NULL)
      return NULL;

  if (s->p_getpwnam.pw_name != NULL)
  {
    /* cache the last name queried */
    if (name != NULL && !strcmp (s->p_getpwnam.pw_name, name))
      return &s->p_getpwnam;

    free (s->p_getpwnam.pw_name);
    s->p_getpwnam.pw_name = NULL;
    s->p_getpwnam.pw_uid = 0;
  }

#if defined(_REENTRANT) && defined(HAVE_GETPWNAM_R)
  if (getpwnam_r(name, &pwbuf, buf, sizeof(buf), &q))
    q = NULL;
#else
  q = getpwnam (name);
#endif
                                                                                
  if (q == NULL)
    return NULL;
  s->p_getpwnam.pw_uid = q->pw_uid;
  s->p_getpwnam.pw_name = strdup (q->pw_name);

  return &s->p_getpwnam;
#else
  char query[256];
  MYSQL_RES *result;
  MYSQL_ROW row;
  char *virtual_table, *virtual_uid, *virtual_username;
  char *sql_username;

  if ((virtual_table
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualTable"))==NULL)
  { virtual_table = "dspam_virtual_uids"; }

  if ((virtual_uid
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualUIDField"))==NULL)
  { virtual_uid = "uid"; }

  if ((virtual_username = _ds_read_attribute(CTX->config->attributes,
    "MySQLVirtualUsernameField")) ==NULL)
  { virtual_username = "username"; }

  if (s->p_getpwnam.pw_name != NULL)
  {
    /* cache the last name queried */
    if (name != NULL && !strcmp (s->p_getpwnam.pw_name, name))
      return &s->p_getpwnam;

    free (s->p_getpwnam.pw_name);
    s->p_getpwnam.pw_name = NULL;
  }

  if (name == NULL)
      return NULL;

  sql_username = malloc ((2 * strlen(name)) + 1);
  if (sql_username == NULL)
  {
    return NULL;
  }

  mysql_real_escape_string (s->dbt->dbh_read, sql_username, name,
                            strlen(name));

  snprintf (query, sizeof (query),
            "select %s from %s where %s = '%s'", 
            virtual_uid, virtual_table, virtual_username, sql_username);

  free (sql_username);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    return NULL;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL) {
    if (CTX->source == DSS_ERROR || CTX->operating_mode != DSM_PROCESS)
      return NULL;
    return _mysql_drv_setpwnam (CTX, name);
  }

  row = mysql_fetch_row (result);
  if (row == NULL)
  {
    mysql_free_result (result);
    if (CTX->source == DSS_ERROR || CTX->operating_mode != DSM_PROCESS)
      return NULL;
    return _mysql_drv_setpwnam (CTX, name);
  }

  if (row[0] == NULL)
  {
    mysql_free_result (result);
    if (CTX->source == DSS_ERROR || CTX->operating_mode != DSM_PROCESS)
      return NULL;
    return _mysql_drv_setpwnam (CTX, name);
  }

  s->p_getpwnam.pw_uid = strtol (row[0], NULL, 0);
  if (name == NULL)
    s->p_getpwnam.pw_name = strdup("");
  else
    s->p_getpwnam.pw_name = strdup (name);

  mysql_free_result (result);
  return &s->p_getpwnam;

#endif
}

struct passwd *
_mysql_drv_getpwuid (DSPAM_CTX * CTX, uid_t uid)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
#ifndef VIRTUAL_USERS
  struct passwd *q;
#if defined(_REENTRANT) && defined(HAVE_GETPWUID_R)
  struct passwd pwbuf;
  char buf[1024];
#endif

  if (s->p_getpwuid.pw_name != NULL)
  {
    /* cache the last uid queried */
    if (s->p_getpwuid.pw_uid == uid)
    {
      return &s->p_getpwuid;
    }
    free(s->p_getpwuid.pw_name);
    s->p_getpwuid.pw_name = NULL;
  }

#if defined(_REENTRANT) && defined(HAVE_GETPWUID_R)
  if (getpwuid_r(uid, &pwbuf, buf, sizeof(buf), &q))
    q = NULL;
#else
  q = getpwuid (uid);
#endif

  if (q == NULL) 
   return NULL;

  if (s->p_getpwuid.pw_name)
    free(s->p_getpwuid.pw_name);

  memcpy (&s->p_getpwuid, q, sizeof (struct passwd));
  s->p_getpwuid.pw_name = strdup(q->pw_name);

  return &s->p_getpwuid;
#else
  char query[256];
  MYSQL_RES *result;
  MYSQL_ROW row;
  char *virtual_table, *virtual_uid, *virtual_username;

  if ((virtual_table
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualTable"))==NULL)
  { virtual_table = "dspam_virtual_uids"; }

  if ((virtual_uid
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualUIDField"))==NULL)
  { virtual_uid = "uid"; }

  if ((virtual_username = _ds_read_attribute(CTX->config->attributes,
    "MySQLVirtualUsernameField")) ==NULL)
  { virtual_username = "username"; }

  if (s->p_getpwuid.pw_name != NULL)
  {
    /* cache the last uid queried */
    if (s->p_getpwuid.pw_uid == uid)
      return &s->p_getpwuid;
    free (s->p_getpwuid.pw_name);
    s->p_getpwuid.pw_name = NULL;
  }

  snprintf (query, sizeof (query),
            "select %s from %s where %s = '%d'", 
            virtual_username, virtual_table, virtual_uid, (int) uid);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    return NULL;
  }

  result = mysql_use_result (s->dbt->dbh_read);
  if (result == NULL)
    return NULL;

  row = mysql_fetch_row (result);
  if (row == NULL)
  {
    mysql_free_result (result);
    return NULL;
  }

  if (row[0] == NULL)
  {
    mysql_free_result (result);
    return NULL;
  }

  s->p_getpwuid.pw_name = strdup (row[0]);
  s->p_getpwuid.pw_uid = uid;

  mysql_free_result (result);
  return &s->p_getpwuid;
#endif
}

void
_mysql_drv_query_error (const char *error, const char *query)
{
  FILE *file;
  char fn[MAX_FILENAME_LENGTH];
  char buf[128];

  LOG (LOG_WARNING, "query error: %s: see sql.errors for more details",
       error);

  snprintf (fn, sizeof (fn), "%s/sql.errors", LOGDIR);

  file = fopen (fn, "a");

  if (file == NULL)
  {
    LOG(LOG_ERR, ERR_IO_FILE_WRITE, fn, strerror (errno));
    return;
  }

  fprintf (file, "[%s] %d: %s: %s\n", format_date_r(buf), (int) getpid (), 
    error, query); fclose (file);
  return;
}

#ifdef VIRTUAL_USERS
struct passwd *
_mysql_drv_setpwnam (DSPAM_CTX * CTX, const char *name)
{
  char query[256];
  char *virtual_table, *virtual_uid, *virtual_username;
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  char *sql_username;

  if ((virtual_table
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualTable"))==NULL)
  { virtual_table = "dspam_virtual_uids"; }

  if ((virtual_uid
    = _ds_read_attribute(CTX->config->attributes, "MySQLVirtualUIDField"))==NULL)
  { virtual_uid = "uid"; }

  if ((virtual_username = _ds_read_attribute(CTX->config->attributes,
    "MySQLVirtualUsernameField")) ==NULL)
  { virtual_username = "username"; }

#ifdef USE_LDAP
  if (_ds_match_attribute(CTX->config->attributes, "LDAPMode", "verify") &&
      ldap_verify(CTX, name)<=0) 
  {
    LOGDEBUG("LDAP verification of %s failed: not adding user", name);
    return NULL;
  }
#endif

  sql_username = malloc (strlen(name) * 2 + 1);
  if (sql_username == NULL)
  {
    return NULL;
  }

  mysql_real_escape_string (s->dbt->dbh_write, sql_username, name,
                            strlen(name));

  snprintf (query, sizeof (query),
            "insert into %s (%s, %s) values(NULL, '%s')",
            virtual_table, virtual_uid, virtual_username, sql_username);

  free (sql_username);

  /* we need to fail, to prevent a potential loop - even if it was inserted
   * by another process */
  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    return NULL;
  }

  return _mysql_drv_getpwnam (CTX, name);

}
#endif

int
_ds_del_spamrecord (DSPAM_CTX * CTX, unsigned long long token)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  struct passwd *p;
  char query[128];

  if (s->dbt == NULL)
  {
    LOGDEBUG ("_ds_delete_signature: invalid database handle (NULL)");
    return EINVAL;
  }
                                                                                
  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_delete_token: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
    return EINVAL;
  }
  if (_ds_match_attribute(CTX->config->attributes, "MySQLSupressQuote", "on"))
    snprintf (query, sizeof (query),
            "delete from dspam_token_data where uid = %d and token = %llu",
            (int) p->pw_uid, token);
  else
    snprintf (query, sizeof (query),
            "delete from dspam_token_data where uid = %d and token = \"%llu\"",
            (int) p->pw_uid, token);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    return EFAILURE;
  }
                                                                                
  return 0;
}

int _ds_delall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  struct _mysql_drv_storage *s = (struct _mysql_drv_storage *) CTX->storage;
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  buffer *query;
  char scratch[1024];
  char queryhead[1024];
  struct passwd *p;
  int writes = 0;

  if (diction->items < 1)
    return 0;

  if (s->dbt->dbh_write == NULL)
  {
    LOGDEBUG ("_ds_delall_spamrecords: invalid database handle (NULL)");
    return EINVAL;
  }

  if (!CTX->group || CTX->flags & DSF_MERGED)
    p = _mysql_drv_getpwnam (CTX, CTX->username);
  else
    p = _mysql_drv_getpwnam (CTX, CTX->group);

  if (p == NULL)
  {
    LOGDEBUG ("_ds_delall_spamrecords: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
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
            "where uid = %d and token in(",
            (int) p->pw_uid);

  buffer_cat (query, queryhead);

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    if (_ds_match_attribute(CTX->config->attributes, "MySQLSupressQuote", "on"))
      snprintf (scratch, sizeof (scratch), "%llu", ds_term->key);
    else
      snprintf (scratch, sizeof (scratch), "'%llu'", ds_term->key);
    buffer_cat (query, scratch);
    ds_term = ds_diction_next(ds_c);
   
    if (writes > 2500 || !ds_term) {
      buffer_cat (query, ")");

      if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query->data))
      {
        _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query->data);
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

    if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query->data))
    {
      _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query->data);
      buffer_destroy(query);
      return EFAILURE;
    }
  }

  buffer_destroy (query);
  return 0;
}

#ifdef PREFERENCES_EXTENSION
DSPAM_CTX *_mysql_drv_init_tools(
 const char *home,
 config_t config,
 void *dbt, 
 int mode)
{
  DSPAM_CTX *CTX;
  struct _mysql_drv_storage *s;
  int dbh_attached = (dbt) ? 1 : 0;

  CTX = dspam_create (NULL, NULL, home, mode, 0);

  if (CTX == NULL)
    return NULL;

  _mysql_drv_set_attributes(CTX, config);

  if (!dbt)
    dbt = _ds_connect(CTX);

  if (!dbt)
    goto BAIL;

  if (dspam_attach(CTX, dbt))
    goto BAIL;

  s = CTX->storage;
  s->dbh_attached = dbh_attached;
  return CTX;

BAIL:
  dspam_destroy(CTX);
  return NULL;
}

agent_pref_t _ds_pref_load(
  config_t config,
  const char *username, 
  const char *home,
  void *dbt)
{
  struct _mysql_drv_storage *s;
  struct passwd *p;
  char query[128];
  MYSQL_RES *result;
  MYSQL_ROW row; 
  DSPAM_CTX *CTX;
  agent_pref_t PTX;
  agent_attrib_t pref;
  int uid, i = 0;

  CTX = _mysql_drv_init_tools(home, config, dbt, DSM_TOOLS);
  if (CTX == NULL)
  {
    LOG (LOG_WARNING, "unable to initialize tools context");
    return NULL;
  }

  s = (struct _mysql_drv_storage *) CTX->storage;

  if (username != NULL) {
    p = _mysql_drv_getpwnam (CTX, username);

    if (p == NULL)
    {
      LOGDEBUG ("_ds_pref_load: unable to _mysql_drv_getpwnam(%s)",
              username);
      dspam_destroy(CTX);
      return NULL;
    } else {
      uid = p->pw_uid;
    }
  } else {
    uid = 0; /* Default Preferences */
  }

  LOGDEBUG("Loading preferences for uid %d", uid);

  snprintf(query, sizeof(query), "select preference, value "
    "from dspam_preferences where uid = %d", uid);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_read, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_read), query);
    dspam_destroy(CTX);
    return NULL;
  }

  result = mysql_store_result (s->dbt->dbh_read);
  if (result == NULL) {
    dspam_destroy(CTX);
    return NULL;
  }

  PTX = malloc(sizeof(agent_attrib_t )*(mysql_num_rows(result)+1));
  if (PTX == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC); 
    dspam_destroy(CTX);
    return NULL;
  }

  PTX[0] = NULL;

  row = mysql_fetch_row (result);
  if (row == NULL) {
    dspam_destroy(CTX);
    mysql_free_result(result);
    _ds_pref_free(PTX);
    free(PTX);
    return NULL;
  }

  while(row != NULL) {
    char *p = row[0];
    char *q = row[1];

    pref = malloc(sizeof(struct _ds_agent_attribute));
    if (pref == NULL) {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      dspam_destroy(CTX);
      return PTX;
    }
                                                                                
    pref->attribute = strdup(p);
    pref->value = strdup(q);
    PTX[i] = pref;
    PTX[i+1] = NULL;
    i++;

    row = mysql_fetch_row (result);
  }

  mysql_free_result(result);
  dspam_destroy(CTX);
  return PTX;
}

int _ds_pref_set (
 config_t config,
 const char *username, 
 const char *home,
 const char *preference,
 const char *value,
 void *dbt) 
{
  struct _mysql_drv_storage *s;
  struct passwd *p;
  char query[128];
  DSPAM_CTX *CTX;
  int uid;
  char *m1, *m2;

  CTX = _mysql_drv_init_tools(home, config, dbt, DSM_PROCESS);
  if (CTX == NULL) {
    LOG (LOG_WARNING, "unable to initialize tools context");
    return EUNKNOWN;
  }
                                                                                
  s = (struct _mysql_drv_storage *) CTX->storage;
                                                                                
  if (username != NULL) {
    p = _mysql_drv_getpwnam (CTX, username);
                                                                                
    if (p == NULL)
    {
      LOGDEBUG ("_ds_pref_set: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
      dspam_destroy(CTX);
      return EUNKNOWN; 
    } else {
      uid = p->pw_uid;
    }
  } else {
    uid = 0; /* Default Preferences */
  }

  m1 = calloc(1, strlen(preference)*2+1);
  m2 = calloc(1, strlen(value)*2+1);
  if (m1 == NULL || m2 == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    dspam_destroy(CTX);
    free(m1);
    free(m2);
    return EUNKNOWN;
  }
                                                                                
  mysql_real_escape_string (s->dbt->dbh_write, m1, preference, strlen(preference));   
  mysql_real_escape_string (s->dbt->dbh_write, m2, value, strlen(value)); 

  snprintf(query, sizeof(query), "delete from dspam_preferences "
    "where uid = %d and preference = '%s'", uid, m1);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    goto FAIL;
  }

  snprintf(query, sizeof(query), "insert into dspam_preferences "
    "(uid, preference, value) values(%d, '%s', '%s')", uid, m1, m2);

  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    goto FAIL;
  }

  dspam_destroy(CTX);
  free(m1);
  free(m2);
  return 0;

FAIL:
  free(m1);
  free(m2);
  dspam_destroy(CTX);
  LOGDEBUG("_ds_pref_set() failed");
  return EFAILURE;
}

int _ds_pref_del (
 config_t config,
 const char *username,
 const char *home,
 const char *preference,
 void *dbt)
{
  struct _mysql_drv_storage *s;
  struct passwd *p;
  char query[128];
  DSPAM_CTX *CTX;
  int uid;
  char *m1;
                                                                                
  CTX = _mysql_drv_init_tools(home, config, dbt, DSM_TOOLS);
  if (CTX == NULL) {
    LOG (LOG_WARNING, "unable to initialize tools context");
    return EUNKNOWN;
  }

  s = (struct _mysql_drv_storage *) CTX->storage;
                                                                                
  if (username != NULL) {
    p = _mysql_drv_getpwnam (CTX, username);
                                                                                
    if (p == NULL)
    {
      LOGDEBUG ("_ds_pref_del: unable to _mysql_drv_getpwnam(%s)",
              CTX->username);
      dspam_destroy(CTX);
      return EUNKNOWN;
    } else {
      uid = p->pw_uid;
    }
  } else {
    uid = 0; /* Default Preferences */
  }
                                                                                
  m1 = calloc(1, strlen(preference)*2+1);
  if (m1 == NULL) 
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    dspam_destroy(CTX);
    free(m1);
    return EUNKNOWN;
  }
                                                                                
  mysql_real_escape_string (s->dbt->dbh_write, m1, preference, strlen(preference));

  snprintf(query, sizeof(query), "delete from dspam_preferences "
    "where uid = %d and preference = '%s'", uid, m1);
                                                                                
  if (MYSQL_RUN_QUERY (s->dbt->dbh_write, query))
  {
    _mysql_drv_query_error (mysql_error (s->dbt->dbh_write), query);
    goto FAIL;
  }

  dspam_destroy(CTX);
  free(m1);
  return 0;
                                                                                
FAIL:
  free(m1);
  dspam_destroy(CTX);
  LOGDEBUG("_ds_pref_del() failed");
  return EFAILURE;

}

int _mysql_drv_set_attributes(DSPAM_CTX *CTX, config_t config) {
  int i, ret = 0;
  attribute_t t;
  char *profile;

  profile = _ds_read_attribute(config, "DefaultProfile");

  for(i=0;config[i];i++) {
    t = config[i];

    while(t) {

      if (!strncasecmp(t->key, "MySQL", 5))
      {
        if (profile == NULL || profile[0] == 0)
        {
          ret += dspam_addattribute(CTX, t->key, t->value);
        }
        else if (strchr(t->key, '.'))
        {
          if (!strcasecmp((strchr(t->key, '.')+1), profile)) {
            char *x = strdup(t->key);
            char *y = strchr(x, '.');
            y[0] = 0;

            ret += dspam_addattribute(CTX, x, t->value);
            free(x);
          }
        }
      }
      t = t->next;
    }
  }

  return 0;
}

#else
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
#endif

void *_ds_connect (DSPAM_CTX *CTX)
{
  _mysql_drv_dbh_t dbt = calloc(1, sizeof(struct _mysql_drv_dbh));
  dbt->dbh_read = _mysql_drv_connect(CTX, "MySQL");
  if (!dbt->dbh_read) { 
    free(dbt);
    return NULL;
  }
  if (_ds_read_attribute(CTX->config->attributes, "MySQLWriteServer"))
    dbt->dbh_write = _mysql_drv_connect(CTX, "MySQLWrite");
  else
    dbt->dbh_write = dbt->dbh_read;
  return (void *) dbt;
}

MYSQL *_mysql_drv_connect (DSPAM_CTX *CTX, const char *prefix) 
{
  MYSQL *dbh;
  FILE *file;
  char filename[MAX_FILENAME_LENGTH];
  char buffer[128];
  char hostname[128] = { 0 };
  char user[64] = { 0 };
  char password[64] = { 0 };
  char db[64] = { 0 };
  int port = 3306, i = 0, real_connect_flag = 0;
  char *p;
  char attrib[128];

  if (!prefix) 
    prefix = "MySQL";

  /* Read storage attributes */
  snprintf(attrib, sizeof(attrib), "%sServer", prefix);
  if ((p = _ds_read_attribute(CTX->config->attributes, attrib))) {

    strlcpy(hostname, p, sizeof(hostname));
    if (strlen(p) >= sizeof(hostname))
    {
      LOG(LOG_WARNING, "Truncating MySQLServer to %d characters.",
          sizeof(hostname)-1);
    }

    snprintf(attrib, sizeof(attrib), "%sPort", prefix);
    if (_ds_read_attribute(CTX->config->attributes, attrib))
      port = atoi(_ds_read_attribute(CTX->config->attributes, attrib));
    else
      port = 0;

    snprintf(attrib, sizeof(attrib), "%sUser", prefix);
    if ((p = _ds_read_attribute(CTX->config->attributes, attrib)))
    {
      strlcpy(user, p, sizeof(user));
      if (strlen(p) >= sizeof(user))
      {
        LOG(LOG_WARNING, "Truncating MySQLUser to %d characters.",
            sizeof(user)-1);
      }
    }
    snprintf(attrib, sizeof(attrib), "%sPass", prefix);
    if ((p = _ds_read_attribute(CTX->config->attributes, attrib)))
    {
      strlcpy(password, p, sizeof(password));
      if (strlen(p) >= sizeof(password))
      {
        LOG(LOG_WARNING, "Truncating MySQLPass to %d characters.",
            sizeof(password)-1);
      }
    }
    snprintf(attrib, sizeof(attrib), "%sDb", prefix);
    if ((p = _ds_read_attribute(CTX->config->attributes, attrib)))
    {
      strlcpy(db, p, sizeof(db)); 
      if (strlen(p) >= sizeof(db))
      {
        LOG(LOG_WARNING, "Truncating MySQLDb to %d characters.",
            sizeof(db)-1);
      }
    }

    snprintf(attrib, sizeof(attrib), "%sCompress", prefix);
    if (_ds_match_attribute(CTX->config->attributes, attrib, "true"))
      real_connect_flag = CLIENT_COMPRESS;

  } else {
    if (!CTX->home) {
      LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
      goto FAILURE;
    }
    snprintf (filename, MAX_FILENAME_LENGTH, "%s/mysql.data", CTX->home);
    file = fopen (filename, "r");
    if (file == NULL)
    {
      LOG (LOG_WARNING, "unable to locate mysql configuration");
      goto FAILURE;
    }
  
    db[0] = 0;
  
    while (fgets (buffer, sizeof (buffer), file) != NULL)
    {
      chomp (buffer);
      if (!i)
        strlcpy (hostname, buffer, sizeof (hostname));
      else if (i == 1)
        port = atoi (buffer);
      else if (i == 2)
        strlcpy (user, buffer, sizeof (user));
      else if (i == 3)
        strlcpy (password, buffer, sizeof (password));
      else if (i == 4)
        strlcpy (db, buffer, sizeof (db));
      i++;
    }
    fclose (file);
  }

  if (db[0] == 0)
  {
    LOG (LOG_WARNING, "file %s: incomplete mysql connect data", filename);
    goto FAILURE;
  }

  dbh = mysql_init(NULL);
  if (dbh == NULL)
  {
    LOGDEBUG
      ("_ds_init_storage: mysql_init: unable to initialize handle to database");
    goto FAILURE;
  }

  if (hostname[0] == '/')
  {
    if (!mysql_real_connect (dbh, NULL, user, password, db, 0, hostname, 
                        real_connect_flag))
    {
      LOG (LOG_WARNING, "%s", mysql_error (dbh));
      mysql_close(dbh);
      goto FAILURE;
    }
  }
  else
  {
    if (!mysql_real_connect (dbh, hostname, user, password, db, port, NULL,
                        real_connect_flag))
    {
      LOG (LOG_WARNING, "%s", mysql_error (dbh));
      mysql_close(dbh);
      goto FAILURE;
    }
  }

  return dbh;

FAILURE:
  LOGDEBUG("_ds_init_storage() failed");
  return NULL;
}

MYSQL * _mysql_drv_sig_write_handle(
    DSPAM_CTX *CTX,
    struct _mysql_drv_storage *s)
{
  if (_ds_match_attribute(CTX->config->attributes, "MySQLReadSignaturesFromWriteDb", "on"))
    return s->dbt->dbh_write;
  else
    return s->dbt->dbh_read;
}

