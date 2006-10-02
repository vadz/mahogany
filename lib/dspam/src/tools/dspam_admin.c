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
#include <signal.h>
#include <unistd.h>
#include "config.h"

#include "libdspam.h"
#include "read_config.h"
#include "language.h"
#include "pref.h"

#define TSYNTAX	"syntax: dspam_admin [function] [arguments] [--profile=PROFILE]"

/* 
  PREFERENCE FUNCTIONS

  Add a Preference:	dspam_admin add preference [user] [attr] [value]
  Set a Preference:	dspam_admin change preference [user] [attr] [value]
  Delete a Preference:	dspam_admin delete preference [user] [attr]
  List Preferences:     dspam_admin list preferences [user]

*/

int set_preference_attribute(const char *, const char *, const char *);
int del_preference_attribute(const char *, const char *);
int list_preference_attributes(const char *);
int list_aggregate_preference_attributes(const char *);

void dieout (int signal);
void usage (void);
void min_args(int argc, int min);

int
main (int argc, char **argv)
{
#ifdef TRUSTED_USER_SECURITY
  struct passwd *p = getpwuid (getuid ());
#endif
  int i, valid = 0;

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

  signal (SIGINT, dieout);
  signal (SIGPIPE, dieout);
  signal (SIGTERM, dieout);

  dspam_init_driver (NULL);

  if (argc < 3 || !strcmp(argv[1], "help"))
  {
    usage();
  }

  /* PREFERENCE FUNCTIONS */
  if (!strncmp(argv[2], "pref", 4)) {
 
    /* Delete */
    if (!strncmp(argv[1], "d", 1)) {
      min_args(argc, 4);
      valid = 1;
      del_preference_attribute(argv[3], argv[4]);
    }

    /* Add, Change */
    if (!strncmp(argv[1], "ch", 2) || !strncmp(argv[1], "ad", 2)) {
      min_args(argc, 5);
      valid = 1; 
      set_preference_attribute(argv[3], argv[4], argv[5]); 
    }

    /* List */
    if (!strncmp(argv[1], "l", 1)) {
      min_args(argc, 3);
      valid = 1;
      list_preference_attributes(argv[3]);
    }

    /* Aggregate - Preference attr + AllowOverride attr + user prefs */
    if (!strncmp(argv[1], "ag", 2)) {
      min_args(argc, 3);
      valid = 1;
      list_aggregate_preference_attributes(argv[3]);
    }
  }

  if (!valid) 
    usage();

  dspam_shutdown_driver (NULL);
  _ds_destroy_config(agent_config);
  libdspam_shutdown();
  exit (EXIT_SUCCESS);

BAIL:
  libdspam_shutdown();
  exit(EXIT_FAILURE);
}

void
dieout (int signal)
{
  fprintf (stderr, "terminated.\n");
  _ds_destroy_config(agent_config);
  exit (EXIT_SUCCESS);
}

void
usage (void)
{
  fprintf(stderr, "%s\n", TSYNTAX);

  fprintf(stderr, "\tadd preference [user] [attrib] [value]\n");
  fprintf(stderr, "\tchange preference [user] [attrib] [value]\n");
  fprintf(stderr, "\tdelete preference [user] [attrib] [value]\n");
  fprintf(stderr, "\tlist preference [user] [attrib] [value]\n");
  fprintf(stderr, "\taggregate preference [user]\n");
  _ds_destroy_config(agent_config);
  exit(EXIT_FAILURE);
}

void min_args(int argc, int min) {
  if (argc<(min+1)) {
    fprintf(stderr, "invalid command syntax.\n");
    usage();
    _ds_destroy_config(agent_config);
    exit(EXIT_FAILURE);
  }
  return;
}

int set_preference_attribute(
  const char *username, 
  const char *attr, 
  const char *value)
{
  int i;

  if (username[0] == 0 || !strcmp(username, "default"))
    i = _ds_pref_set(agent_config, NULL, _ds_read_attribute(agent_config, "Home"), attr, value, NULL);
  else
    i = _ds_pref_set(agent_config, username, _ds_read_attribute(agent_config, "Home"), attr, value, NULL);

  if (!i) 
    printf("operation successful.\n");
  else
    printf("operation failed with error %d.\n", i);

  return i;
}

int del_preference_attribute(
  const char *username,
  const char *attr)
{
  int i;

  if (username[0] == 0 || !strncmp(username, "default", strlen(username)))
    i = _ds_pref_del(agent_config, NULL, _ds_read_attribute(agent_config, "Home"), attr, NULL);
  else
    i = _ds_pref_del(agent_config, username, _ds_read_attribute(agent_config, "Home"), attr, NULL);

  if (!i)
    printf("operation successful.\n");
  else
    printf("operation failed with error %d.\n", i);

  return i;
}

int list_preference_attributes(const char *username)
{
  agent_pref_t PTX;
  agent_attrib_t pref;
  int i;
  
  if (username[0] == 0 || !strncmp(username, "default", strlen(username)))
    PTX = _ds_pref_load(agent_config, NULL, _ds_read_attribute(agent_config, "Home"), NULL);
  else
    PTX = _ds_pref_load(agent_config, username,  _ds_read_attribute(agent_config, "Home"), NULL);

  if (PTX == NULL) 
    return 0;

  for(i=0;PTX[i];i++) {
    pref = PTX[i];
    printf("%s=%s\n", pref->attribute, pref->value);
  }

  _ds_pref_free(PTX);
  free(PTX);
  return 0;
}

int list_aggregate_preference_attributes(const char *username)
{
  agent_pref_t PTX = NULL;
  agent_pref_t STX = NULL;
  agent_pref_t UTX = NULL;
  agent_attrib_t pref;
  int i;
  
  STX =  _ds_pref_load(agent_config,
                       NULL,
                       _ds_read_attribute(agent_config, "Home"), NULL);

  if (STX == NULL || STX[0] == 0) {
    if (STX) {
      _ds_pref_free(STX);
    }
    LOGDEBUG("default preferences empty. reverting to dspam.conf preferences.");
    STX = pref_config();
  } else {
    LOGDEBUG("loaded default preferences externally");
  }

  if (username[0] == 0 || !strncmp(username, "default", strlen(username)))
    UTX = _ds_pref_load(agent_config, NULL, _ds_read_attribute(agent_config, "Home"), NULL);
  else {
    UTX = _ds_pref_load(agent_config, username,  _ds_read_attribute(agent_config, "Home"), NULL);
  }

  PTX = _ds_pref_aggregate(STX, UTX);
  _ds_pref_free(STX);
  free(STX);
  if (UTX != NULL) {
    _ds_pref_free(UTX);
    free(UTX);
  }

  for(i=0;PTX[i];i++) {
    pref = PTX[i];
    printf("%s=%s\n", pref->attribute, pref->value);
  }

  _ds_pref_free(PTX);
  free(PTX);
  return 0;
}
