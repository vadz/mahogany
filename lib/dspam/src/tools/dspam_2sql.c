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
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>

#include "libdspam.h"
#include "util.h"
#include "read_config.h"
#include "config_api.h"
#include "language.h"

DSPAM_CTX *open_ctx, *open_mtx;

int dump_user (const char *username);
int process_all_users (void);
void dieout (int signal);

int
main (int argc, char **argv)
{
#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  struct passwd *p = getpwuid (getuid ());
#endif
#endif
  int i, ret;

 /* Read dspam.conf */
                                                                                
  agent_config = read_config(NULL);
  if (!agent_config) {
    LOG(LOG_ERR, ERR_AGENT_READ_CONFIG);
    exit(EXIT_FAILURE);
  }
                                                                                
  if (!_ds_read_attribute(agent_config, "Home")) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    _ds_destroy_config(agent_config);
    exit(EXIT_FAILURE);
  }

  libdspam_init(_ds_read_attribute(agent_config, "StorageDriver"));
                                                                                
#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  if (!_ds_match_attribute(agent_config, "Trust", p->pw_name) && p->pw_uid) {
    fprintf(stderr, ERR_TRUSTED_MODE "\n");
    _ds_destroy_config(agent_config);
    goto BAIL;
  }
#endif
#endif

  for(i=0;i<argc;i++) {
                                                                                
    if (!strncmp (argv[i], "--profile=", 10))
    {
      if (!_ds_match_attribute(agent_config, "Profile", argv[i]+10)) {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_PROFILE, argv[i]+10);
        _ds_destroy_config(agent_config);
        goto BAIL;
      } else {
        _ds_overwrite_attribute(agent_config, "DefaultProfile", argv[i]+10);
      }
      break;
    }
  }

  open_ctx = open_mtx = NULL;

  signal (SIGINT, dieout);
  signal (SIGPIPE, dieout);
  signal (SIGTERM, dieout);

  dspam_init_driver (NULL);
  ret = process_all_users();
  dspam_shutdown_driver (NULL);
  _ds_destroy_config(agent_config);
  libdspam_shutdown();
  exit((ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE);

BAIL:
  libdspam_shutdown();
  exit(EXIT_FAILURE);
}

int
process_all_users (void)
{
  DSPAM_CTX *CTX;
  char *user;

  CTX = dspam_create (NULL, NULL, _ds_read_attribute(agent_config, "Home"), DSM_TOOLS, 0);
  open_ctx = CTX;
  if (CTX == NULL)
  {
    fprintf (stderr, "Could not initialize context: %s\n", strerror (errno));
    return EFAILURE;
  }

  set_libdspam_attributes(CTX);
  if (dspam_attach(CTX, NULL)) {
    LOG (LOG_WARNING, "unable to attach dspam context");
    return EFAILURE;
  }


  user = _ds_get_nextuser (CTX);
  while (user != NULL)
  {
    dump_user (user);
    user = _ds_get_nextuser (CTX);
  }

  dspam_destroy (CTX);
  open_ctx = NULL;
  return 0;
}

int
dump_user (const char *username)
{
  struct passwd *p;
  struct _ds_storage_record *record;
  DSPAM_CTX *CTX;

  p = getpwnam (username);
  if (p == NULL)
  {
    fprintf (stderr, "Unable to obtain uid for user %s\n", username);
    return EUNKNOWN;
  }

  CTX = dspam_create (username, NULL, _ds_read_attribute(agent_config, "Home"), DSM_CLASSIFY, 0);
  open_mtx = CTX;
  if (CTX == NULL)
  {
    fprintf (stderr, "Could not init context: %s\n", strerror (errno));
    return EUNKNOWN;
  }

  set_libdspam_attributes(CTX);
  if (dspam_attach(CTX, NULL)) {
    LOG (LOG_WARNING, "unable to attach dspam context");
    return EFAILURE;
  }

  printf
    ("insert into dspam_stats (uid, spam_learned, innocent_learned, spam_misclassified, innocent_misclassified, spam_corpusfed, innocent_corpusfed, spam_classified, innocent_classified) values(%d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld);\n",
     (int) p->pw_uid, CTX->totals.spam_learned, CTX->totals.innocent_learned,
     CTX->totals.spam_misclassified, CTX->totals.innocent_misclassified,
     CTX->totals.spam_corpusfed, CTX->totals.innocent_corpusfed,
     CTX->totals.spam_classified, CTX->totals.innocent_classified);

  record = _ds_get_nexttoken (CTX);
  while (record != NULL)
  {
    printf
      ("insert into dspam_token_data (uid, token, spam_hits, innocent_hits, last_hit) values(%d, \"%"LLU_FMT_SPEC"\", %ld, %ld, %ld);\n",
       (int) p->pw_uid, record->token, record->spam_hits,
       record->innocent_hits, (long) record->last_hit);
    record = _ds_get_nexttoken (CTX);
  }

  dspam_destroy (CTX);
  open_mtx = NULL;
  return 0;
}

void
dieout (int signal)
{
  fprintf (stderr, "terminated.\n");
  if (open_ctx != NULL)
    dspam_destroy (open_ctx);
  if (open_mtx != NULL)
    dspam_destroy (open_mtx);
  _ds_destroy_config(agent_config);
  exit (EXIT_SUCCESS);
}
