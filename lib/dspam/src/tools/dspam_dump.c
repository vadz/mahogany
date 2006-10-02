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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "util.h"

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

#include "libdspam.h"
#include "read_config.h"
#include "config_api.h"
#include "language.h"

DSPAM_CTX *open_ctx;

/* headers */
int dump_database (DSPAM_CTX * CTX, const char *token, int sql);
void dieout (int signal);

#define TSYNTAX	\
  "syntax: dspam_dump [--profile=Profile] [-d sqlite_drv] [username [token]]"

#define SQL_SQLITE_DRV	1

int
main (int argc, char **argv)
{
  DSPAM_CTX *CTX;
  int r, sql = 0, i;
  char *username = NULL, *token = NULL;
#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  struct passwd *p = getpwuid (getuid ());
#endif
#endif

  /* Read dspam.conf */
                                                                                
  agent_config = read_config(NULL);
  if (!agent_config) {
    LOG(LOG_ERR, ERR_AGENT_READ_CONFIG);
    exit(EXIT_FAILURE);
  }
                                                                                
  if (!_ds_read_attribute(agent_config, "Home")) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    exit(EXIT_FAILURE);
  }

  libdspam_init(_ds_read_attribute(agent_config, "StorageDriver"));

#ifndef WIN32
#ifdef TRUSTED_USER_SECURITY
  if (!_ds_match_attribute(agent_config, "Trust", p->pw_name) && p->pw_uid) {
    fprintf(stderr, ERR_TRUSTED_MODE "\n");
    goto BAIL;
  }
#endif
#endif

  open_ctx = NULL;
  signal (SIGINT, dieout);
  signal (SIGPIPE, dieout);
  signal (SIGTERM, dieout);

  if (argc < 2)
  {
    fprintf (stderr, "%s\n", TSYNTAX);
    goto BAIL;
  }

  for(i=1;i<argc;i++) {

    if (!strncmp (argv[i], "--profile=", 10))
    {
      if (!_ds_match_attribute(agent_config, "Profile", argv[i]+10)) {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_PROFILE, argv[i]+10);
        goto BAIL;
      } else {
        _ds_overwrite_attribute(agent_config, "DefaultProfile", argv[i]+10);
      }
      continue;
    }

    if (!strcmp(argv[i], "-d")) {
      if (i+1<argc && !strcmp(argv[i+1], "sqlite_drv")) {
        sql = SQL_SQLITE_DRV;
        i++;
      } else {
        fprintf(stderr, "invalid driver or no driver specified\n");
        goto BAIL;
      }
    } else {
      if (username == NULL)
        username = argv[i];
      else
        token = argv[i];
    }
  }

  dspam_init_driver (NULL);
  CTX = dspam_create (username, NULL, _ds_read_attribute(agent_config, "Home"), DSM_CLASSIFY, 0);
  open_ctx = CTX;
  if (CTX == NULL)
  {
    fprintf (stderr, "Could not init context: %s\n", strerror (errno));
    dspam_shutdown_driver (NULL);
    goto BAIL;
  }

  set_libdspam_attributes(CTX);
  if (dspam_attach(CTX, NULL)) {
    LOG (LOG_WARNING, "unable to attach dspam context");
    goto BAIL;
  }

  r = dump_database (CTX, token, sql);
  dspam_destroy (CTX);
  open_ctx = NULL;
  dspam_shutdown_driver (NULL);
  libdspam_shutdown();
  return (r) ? EXIT_FAILURE : EXIT_SUCCESS;

BAIL:
  libdspam_shutdown();
  exit(EXIT_FAILURE);
}

int
dump_database (DSPAM_CTX * CTX, const char *token, int sql)
{
  struct _ds_storage_record *sr;
  struct _ds_spam_stat s;
  unsigned long long crc = 0;

  if (token) 
    crc = _ds_getcrc64(token);

  if (sql == SQL_SQLITE_DRV) {
    printf("insert into dspam_stats (dspam_stat_id, "
           "spam_learned, innocent_learned, "
           "spam_misclassified, innocent_misclassified, "
           "spam_corpusfed, innocent_corpusfed, "
           "spam_classified, innocent_classified) values "
           "(0, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld)\n", 
           CTX->totals.spam_learned, CTX->totals.innocent_learned, 
           CTX->totals.spam_misclassified, CTX->totals.innocent_misclassified,
           CTX->totals.spam_corpusfed, CTX->totals.innocent_corpusfed,
           CTX->totals.spam_classified, CTX->totals.innocent_classified);
  }

  if (token == NULL) {
    sr = _ds_get_nexttoken (CTX);
    while (sr != NULL)
    {
      s.innocent_hits = sr->innocent_hits;
      s.spam_hits = sr->spam_hits;
      s.probability = 0.00000;
      _ds_calc_stat(CTX, NULL, &s, DTT_DEFAULT, NULL);
      if (!sql) {
        printf ("%-20"LLU_FMT_SPEC" S: %05ld  I: %05ld  P: %0.4f LH: %s", sr->token,
                sr->spam_hits, sr->innocent_hits, s.probability, 
                ctime (&sr->last_hit));
        free (sr);
      } else if (sql == SQL_SQLITE_DRV) {
        printf("insert into dspam_token_data(token, spam_hits, "
                 "innocent_hits, last_hit) values('%"LLU_FMT_SPEC"', %ld, %ld, "
                 "date('now'))\n", sr->token, sr->spam_hits, sr->innocent_hits);
      }
      sr = _ds_get_nexttoken (CTX);
    }
  } else {
    if (_ds_get_spamrecord (CTX, crc, &s)) {
      fprintf(stderr, "token not found\n");
      return -1;
     }
  
    _ds_calc_stat(CTX, NULL, &s, DTT_DEFAULT, NULL);
    printf ("%-20"LLU_FMT_SPEC" S: %05ld  I: %05ld  P: %0.4f\n", crc,
            s.spam_hits, s.innocent_hits, s.probability);

  }

  return 0;
}

void
dieout (int signal)
{
  fprintf (stderr, "terminated.\n");
  if (open_ctx != NULL)
    dspam_destroy (open_ctx);
  exit (EXIT_SUCCESS);
}
