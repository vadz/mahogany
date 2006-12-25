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
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "pref.h"
#include "config.h"
#include "util.h"
#include "language.h"
#include "read_config.h"

/*
 *  _ds_pref_aggregate: aggregate system preferences and user preferences
 *
 *  This function takes a set of system preferences and a set of user 
 *  preferences as input and returns an aggregated set of preferences based on 
 *  the system's override rules.
 */

agent_pref_t _ds_pref_aggregate(agent_pref_t STX, agent_pref_t UTX) {
  agent_pref_t PTX = calloc(1, PREF_MAX*sizeof(agent_attrib_t ));
  int i, j, size = 0;

  if (STX) {
    for(i=0;STX[i];i++) {
      PTX[i] = _ds_pref_new(STX[i]->attribute, STX[i]->value);
      PTX[i+1] = NULL;
      size++;
    }
  }

  if (UTX) {
    for(i=0;UTX[i];i++) {

      if (_ds_match_attribute(agent_config, "AllowOverride", UTX[i]->attribute))
      {
        int found = 0;
        for(j=0;PTX[j];j++) {
          if (!strcasecmp(PTX[j]->attribute, UTX[i]->attribute)) {
            found = 1;
            free(PTX[j]->value);
            PTX[j]->value = strdup(UTX[i]->value);
            break;
          }
        }
  
        if (!found) {
          PTX[size] = _ds_pref_new(UTX[i]->attribute, UTX[i]->value);
          PTX[size+1] = NULL;
          size++;
        }
      } else {
        LOG(LOG_ERR, ERR_AGENT_IGNORE_PREF, UTX[i]->attribute);
      }
    }
  }

  return PTX;
}

int _ds_pref_free(agent_pref_t PTX) {
  agent_attrib_t pref;
  int i;

  if (!PTX)
    return 0;

  for(i=0;PTX[i];i++) {
    pref = PTX[i];

    free(pref->attribute);
    free(pref->value);
    free(pref);
  }

  return 0;
}

/*
 *  _ds_pref_val: returns the value of an attribute within a preference set
 *
 *  To allow this function to work with string operations, "" will be returned 
 *  if the value isn't found, insttead of NULL
 */

const char *_ds_pref_val(
  agent_pref_t PTX,
  const char *attrib)
{
  agent_attrib_t pref;
  int i;

  if (PTX == NULL)
    return "";

  for(i=0;PTX[i];i++) {
    pref = PTX[i];
    if (!strcasecmp(pref->attribute, attrib))
      return pref->value;
  }

  return "";
}

agent_attrib_t _ds_pref_new(const char *attribute, const char *value) {
  agent_attrib_t pref;
                                                                                
  pref = malloc(sizeof(struct _ds_agent_attribute));
                                                                                
  if (pref == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
                                                                                
  pref->attribute = strdup(attribute);
  pref->value = strdup(value);

  return pref;
}

/* flat-file preference extensions (defaulted to if storage driver extensions
   are not found) */

agent_pref_t _ds_ff_pref_load(
  config_t config,
  const char *user, 
  const char *home,
  void *ignore)
{
  char filename[MAX_FILENAME_LENGTH];
  agent_pref_t PTX = malloc(sizeof(agent_attrib_t )*PREF_MAX);
  char buff[258];
  FILE *file;
  char *p, *q, *bufptr;
  int i = 0;

  UNUSED(config);
  UNUSED(ignore);

  if (PTX == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
  PTX[0] = NULL;

  if (user == NULL) {
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/default.prefs", home);
  } else {
    _ds_userdir_path (filename, home, user, "prefs");
  }
  file = fopen(filename, "r");
 
  /* Apply default preferences from dspam.conf */
                                                                                
  if (file != NULL) {
    char *ptrptr;
    while(i<(PREF_MAX-1) && fgets(buff, sizeof(buff), file)!=NULL) {
      if (buff[0] == '#' || buff[0] == 0)
        continue;
      chomp(buff);

      bufptr = buff;

      p = strtok_r(buff, "=", &ptrptr);

      if (p == NULL)
        continue;
      q = p + strlen(p)+1;

      LOGDEBUG("Loading preference '%s' = '%s'", p, q);

      PTX[i] = _ds_pref_new(p, q);
      PTX[i+1] = NULL;
      i++;
    }
    fclose(file);
  } else {
    free(PTX);
    return NULL;
  }

  return PTX;
}

/*
 *  _ds_ff_pref_prepare_file: prepares a backup copy of a preference file
 *
 *  This operation prepares a backup copy of a given preference file, using a
 *  .bak extension and returns an open filehandle to it at the end of the file.
 *  This function also allows for an omission to be passed in, which is the name
 *  of a preference that should be omitted from the file (if that preference
 *  is to be overwritten or deleted. If nlines is provided, it will be set to
 *  the number of lines in the file.
 */
 
FILE *_ds_ff_pref_prepare_file (
  const char *filename,
  const char *omission,
  int *nlines)
{
  char line[1024], out_filename[MAX_FILENAME_LENGTH];
  int lineno = 0, omission_len;
  FILE *in_file, *out_file;
  char omission_pref[1024];

  snprintf(omission_pref, sizeof(omission_pref), "%s=", omission);
  omission_len = strlen(omission_pref);

  snprintf(out_filename, MAX_FILENAME_LENGTH, "%s.bak", filename);
  out_file = fopen(out_filename, "w");

  if (out_file == NULL) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, out_filename, strerror(errno));
    return NULL;
  }

  in_file = fopen(filename, "r");

  if (in_file) {
    while (fgets(line, sizeof(line), in_file)) {
      if (!strncmp(line, omission_pref, omission_len)) 
        continue;

      lineno++;
  
      if (fputs(line, out_file)<0) {
        LOG(LOG_ERR, ERR_IO_FILE_WRITE, out_filename, strerror(errno));
        fclose(in_file);
        fclose(out_file);
        unlink(out_filename);
        return NULL;
      }
    }
    fclose(in_file);
  }

  if (nlines != NULL)
    *nlines = lineno;
  return out_file;
}

/* _ds_pref_commit: close scratch copy and commit it as the new live copy */

int _ds_ff_pref_commit (
  const char *filename,
  FILE *out_file)
{
  char backup[MAX_FILENAME_LENGTH];

  snprintf(backup, sizeof(backup), "%s.bak", filename);
  if (fclose(out_file)) {
    LOG(LOG_ERR, ERR_IO_FILE_CLOSE, backup, strerror(errno));
    return EFAILURE;
  }

  if (rename(backup, filename)) {
    LOG(LOG_ERR, ERR_IO_FILE_RENAME, backup, strerror(errno));
    unlink(backup);
    return EFAILURE;
  }

  return 0;
}

int _ds_ff_pref_set (
  config_t config,
  const char *username,
  const char *home,
  const char *preference,
  const char *value,
  void *ignore)
{
  char filename[MAX_FILENAME_LENGTH];
  FILE *out_file;

  UNUSED(config);
  UNUSED(ignore);

  if (username == NULL) {
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/default.prefs", home);
  } else {
    _ds_userdir_path (filename, home, username, "prefs");
  }

  out_file = _ds_ff_pref_prepare_file(filename, preference, NULL);

  if (out_file == NULL)
    return EFAILURE;

  fprintf(out_file, "%s=%s\n", preference, value);

  return _ds_ff_pref_commit(filename, out_file);
}

int _ds_ff_pref_del (
  config_t config,
  const char *username,
  const char *home,
  const char *preference,
  void *ignore)
{
  char filename[MAX_FILENAME_LENGTH];
  FILE *out_file;
  int nlines; 

  UNUSED(config);
  UNUSED(ignore);

  if (username == NULL) {
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/default.prefs", home);
  } else {
    _ds_userdir_path (filename, home, username, "prefs");
  }

  out_file = _ds_ff_pref_prepare_file(filename, preference, &nlines);

  if (out_file == NULL)
    return EFAILURE;

  if (!nlines) {
    char backup[MAX_FILENAME_LENGTH];
    fclose(out_file);
    snprintf(backup, sizeof(backup), "%s.bak", filename);
    unlink(backup);
    return unlink(filename);
  }

  return _ds_ff_pref_commit(filename, out_file);
}
