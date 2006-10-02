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
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
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

#include "libdspam.h"
#include "language.h"
#include "read_config.h"
#include "config_api.h"
#include "pref.h"
#include "error.h"

DSPAM_CTX *open_ctx = NULL, *open_mtx = NULL;

int process_sigs   (DSPAM_CTX * CTX, int age);
int process_probs  (DSPAM_CTX * CTX, int age);
int process_unused (DSPAM_CTX * CTX, int any, int quota, int nospam, int onehit);
void dieout (int signal);

#define CLEANSYNTAX "dspam_clean [-s[age] -p[age] -u[any,hapax,nospam,onehit] -h] [user1 user2 ... userN]\n"

int
main (int argc, char *argv[])
{
  DSPAM_CTX *CTX = NULL, *CTX2;
  char *user;
  int do_sigs   = 0;
  int do_probs  = 0;
  int do_unused = 0;
  int age_sigs   = 14;
  int age_probs  = 30;
  int age_unused[4] = { 90, 30, 15, 15 };
  int i, help = 0;
  struct nt *users = NULL;
  struct nt_node *node = NULL;
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
    goto bail;
  }
                                                                                
  libdspam_init(_ds_read_attribute(agent_config, "StorageDriver"));

#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  if (!_ds_match_attribute(agent_config, "Trust", p->pw_name) && p->pw_uid) {
    fprintf(stderr, ERR_TRUSTED_MODE "\n");
    goto bail;
  }
#endif
#endif

  for(i=0;i<argc;i++) {
                                                                                
    if (!strncmp (argv[i], "--profile=", 10))
    {
      if (!_ds_match_attribute(agent_config, "Profile", argv[i]+10)) {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_PROFILE, argv[i]+10);
        goto bail;
      } else {
        _ds_overwrite_attribute(agent_config, "DefaultProfile", argv[i]+10);
      }
      break;
    }
  }

#ifdef DEBUG
  fprintf (stderr, "dspam_clean starting\n");
#endif

  if (_ds_read_attribute(agent_config, "PurgeSignatures") &&
      !_ds_match_attribute(agent_config, "PurgeSignatures", "off"))  
  {
    do_sigs = 1;
    age_sigs = atoi(_ds_read_attribute(agent_config, "PurgeSignatures"));
  }
  
  if (_ds_read_attribute(agent_config, "PurgeNeutral") &&
      !_ds_match_attribute(agent_config, "PurgeNeutral", "off"))
  {
    do_probs = 1;
    age_probs = atoi(_ds_read_attribute(agent_config, "PurgeNeutral"));
  }

  if (_ds_read_attribute(agent_config, "PurgeUnused") &&
      !_ds_match_attribute(agent_config, "PurgeUnused", "off"))
  {
    int i;

    do_unused = 1;
    age_unused[0] = atoi(_ds_read_attribute(agent_config, "PurgeUnused"));
    age_unused[1] = atoi(_ds_read_attribute(agent_config, "PurgeHapaxes"));
    age_unused[2] = atoi(_ds_read_attribute(agent_config, "PurgeHits1S"));
    age_unused[3] = atoi(_ds_read_attribute(agent_config, "PurgeHits1I"));

    for(i=0;i<4;i++) 
      if (age_unused[i]==0)
        do_unused = 0;
  }

  users = nt_create(NT_CHAR);
  if (users == NULL) {
    fprintf(stderr, "%s", ERR_MEM_ALLOC);
    goto bail;
  }

  for(i=0;i<argc;i++) {
    if (!strncmp(argv[i], "-p", 2)) {
      do_probs = 1;
      if (strlen(argv[i])>2)
        age_probs = atoi(argv[i]+2);
    }
    else if (!strncmp(argv[i], "-s", 2)) {
      do_sigs = 1;
      if (strlen(argv[i])>2)
        age_sigs = atoi(argv[i]+2);
    } else if (!strncmp(argv[i], "-u", 2)) {
      do_unused = 1;
      if (strlen(argv[i])>2) {
        char *c = strdup(argv[i]+2);
        char *d = strtok(c, ",");
        int j = 0;
        while(d != NULL && j<4) {
          age_unused[j] = atoi(d);
          j++;
          d = strtok(NULL, ",");
        }
        free(c);
      }
    }
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
      help = 1;
    else if (i>0) 
      nt_add(users, argv[i]);
  }

  if (help || (!do_probs && !do_sigs && !do_unused)) {
    fprintf(stderr, "%s", CLEANSYNTAX);
    _ds_destroy_config(agent_config);
    nt_destroy(users);
    libdspam_shutdown();
    if (help) {
      exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
  }
      
  open_ctx = open_mtx = NULL;

  signal (SIGINT, dieout);
  signal (SIGPIPE, dieout);
  signal (SIGTERM, dieout);

  dspam_init_driver (NULL);

  if (users->items == 0) {
    CTX = dspam_create (NULL, NULL, _ds_read_attribute(agent_config, "Home"), DSM_TOOLS, 0);
    open_ctx = CTX;
    if (CTX == NULL)
    {
      fprintf (stderr, "Could not initialize context: %s\n", strerror (errno));
      dspam_shutdown_driver (NULL);
      goto bail;
    }

    set_libdspam_attributes(CTX);
    if (dspam_attach(CTX, NULL)) {
      LOG (LOG_WARNING, "unable to attach dspam context");
      goto bail;
    }

    user = _ds_get_nextuser (CTX);
  } else {
    node = users->first;
    if (node != NULL)
      user = node->ptr;
    else 
      goto bail;
  }

  while (user != NULL)
  {
#ifdef DEBUG
    printf ("PROCESSING USER: %s\n", user);
#endif
    CTX2 = dspam_create (user, NULL,  _ds_read_attribute(agent_config, "Home"), DSM_TOOLS, 0);
    open_mtx = CTX2;

    if (CTX2 == NULL)
    {
      fprintf (stderr, "Could not initialize context: %s\n",
               strerror (errno));
      return EUNKNOWN;
    }

    set_libdspam_attributes(CTX2);
    if (dspam_attach(CTX2, NULL)) {
      LOG (LOG_WARNING, "unable to attach dspam context");
      goto bail;
    }

    if (do_sigs)
      process_sigs(CTX2, age_sigs);
    if (do_probs)
      process_probs(CTX2, age_probs);
    if (do_unused)
      process_unused(CTX2, age_unused[0], age_unused[1], age_unused[2], age_unused[3]);
    dspam_destroy (CTX2);
    open_mtx = NULL;

    if (users->items == 0) {
      user = _ds_get_nextuser (CTX);
    } else {
      node = node->next;
      if (node == NULL)
        user = NULL;
      else
        user = node->ptr;
    }
  }

  if (users->items == 0) {
    dspam_destroy (CTX);
    open_ctx = NULL;
  }

  dspam_shutdown_driver (NULL);
  _ds_destroy_config(agent_config);
  nt_destroy(users);
  libdspam_shutdown();
  exit (EXIT_SUCCESS);

bail:

  if (open_ctx)
    dspam_destroy(open_ctx);
  if (open_mtx)
    dspam_destroy(open_mtx);
  _ds_destroy_config(agent_config);
  nt_destroy(users);
  libdspam_shutdown();
  exit(EXIT_FAILURE);
}

int
process_sigs (DSPAM_CTX * CTX, int age)
{
  struct _ds_storage_signature *ss;
  struct nt *del;
  struct nt_node *node;
  int delta;

  del = nt_create(NT_CHAR);
  if (del == NULL)
    return -1;

#ifdef DEBUG
    printf ("Processing sigs; age: %d\n", age);
#endif

  ss = _ds_get_nextsignature (CTX);
  while (ss != NULL)
  {
#ifdef DEBUG
    printf ("Signature: %s\n    Created: %s\n", ss->signature,
            ctime (&ss->created_on));
#endif
    delta = (((time (NULL) - ss->created_on) / 60) / 60) / 24;
    if (age == 0 || delta > age)
    {
#ifdef DEBUG
      printf ("    DELETED!\n");
#endif
      nt_add(del, ss->signature);
    }
    free(ss->data);
    free(ss);
    ss = _ds_get_nextsignature (CTX);
  }

  node = del->first;
  while(node != NULL) {
    _ds_delete_signature (CTX, node->ptr);
    node = node->next; 
  }
  nt_destroy(del);

  return 0;
}

int process_probs (DSPAM_CTX *CTX, int age) {
  struct _ds_storage_record *sr;
  struct _ds_spam_stat s;
  ds_diction_t del;
  int delta;

#ifdef DEBUG
  printf("Processing probabilities; age: %d\n", age);
#endif 

  del = ds_diction_create(196613);
  if (del == NULL)
    return -1;
  sr = _ds_get_nexttoken (CTX);
  while (sr != NULL)
  {
    s.innocent_hits = sr->innocent_hits;
    s.spam_hits = sr->spam_hits;
    s.probability = 0.00000;
    _ds_calc_stat(CTX, NULL, &s, DTT_DEFAULT, NULL);
    if (s.probability >= 0.3500 && s.probability <= 0.6500) {
      delta = (((time (NULL) - sr->last_hit) / 60) / 60) / 24;
      if (age == 0 || delta > age)
        ds_diction_touch(del, sr->token, "", 0);
    }
    free (sr);
    sr = _ds_get_nexttoken (CTX);
  }
 
  _ds_delall_spamrecords(CTX, del);
  ds_diction_destroy(del);

  return 0;
}

int process_unused (DSPAM_CTX *CTX, int any, int quota, int nospam, int onehit) {
  struct _ds_storage_record *sr;
  ds_diction_t del;
  time_t t = time(NULL);
  int delta, toe = 0, tum = 0;
  agent_pref_t PTX;
                                                                                
#ifdef DEBUG
  printf("Processing unused; any: %d quota: %d nospam: %d onehit: %d\n",
         any, quota, nospam, onehit);
#endif

  PTX = _ds_pref_load(agent_config, CTX->username, _ds_read_attribute(agent_config, "Home"), NULL);

  if (PTX == NULL || PTX[0] == 0) {
    if (PTX)
      _ds_pref_free(PTX);
    PTX = pref_config();
  }
                                                                                
  if (!strcasecmp(_ds_pref_val(PTX, "trainingMode"), "toe")) {
#ifdef DEBUG
    printf("Limiting unused token purges for user %s - TOE Training Mode Set\n", CTX->username);
#endif
    toe = 1;
  }

  if (!strcasecmp(_ds_pref_val(PTX, "trainingMode"), "tum")) {
#ifdef DEBUG
    printf("Limiting unused token purges for user %s - TUM Training Mode Set\n", CTX->username);
#endif
    tum = 1;
  }

  if (PTX)
    _ds_pref_free(PTX);

  del = ds_diction_create(196613);
  if (del == NULL)
    return -1;
  sr = _ds_get_nexttoken (CTX);
  while (sr != NULL)
  {
    delta = (((t - sr->last_hit) / 60) / 60) / 24;
    if (!toe && (any == 0 || delta > any))
    { 
      if (!tum || sr->innocent_hits + sr->spam_hits < 50)
        ds_diction_touch(del, sr->token, "", 0);
    }
    else if ((sr->innocent_hits*2) + sr->spam_hits < 5)
    { 
      if (quota == 0 || delta > quota)
      {
        ds_diction_touch(del, sr->token, "", 0);
      }
      else if (sr->innocent_hits == 0 && sr->spam_hits == 1 &&
          (nospam == 0 || delta > nospam))
      {
        ds_diction_touch(del, sr->token, "", 0);
      }
      else if (sr->innocent_hits == 1 && sr->spam_hits == 0 &&
          (onehit == 0 || delta > onehit))
      {
        ds_diction_touch(del, sr->token, "", 0);
      }
    }
 
    free (sr);
    sr = _ds_get_nexttoken (CTX);
  }
                                                                                
  _ds_delall_spamrecords(CTX, del);
  ds_diction_destroy(del);
                                                                                
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
