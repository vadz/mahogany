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
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <string.h>

#include "config_shared.h"
#include "read_config.h"
#include "config.h"
#include "error.h"
#include "language.h"
#include "libdspam.h"
#include "pref.h"
#include "util.h"

static char *next_normal_token(char **p)
{
  char *start = *p;
  while (**p) {
    if (**p == ' ' || **p == '\t') {
      **p = 0;
      *p += 1;
      break;
    }
    *p += 1;
  }
  return start;
}

static char *next_quoted_token(char **p)
{
  char *start = *p;
  while (**p) {
    if (**p == '\"') {
      **p = 0;
      *p += 1;
      break;
    }
    *p += 1;
  }
  return start;
}

static char *tokenize(char *text, char **next)
{
  /* Initialize */
  if (text)
    *next = text;

  while (**next) {
    /* Skip leading whitespace */
    if (**next == ' ' || **next == '\t') {
      *next += 1;
      continue;
    }

    /* Strip off one token */
    if (**next == '\"') {
      *next += 1;
      return next_quoted_token(next);
    } else
      return next_normal_token(next);
  }

  return NULL;
}

config_t read_config(const char *path) {
  config_t attrib, ptr;
  FILE *file;
  long attrib_size = 128, num_root = 0;
  char buffer[1024];
  char *a, *c, *v, *bufptr = buffer;

  attrib = calloc(1, attrib_size*sizeof(attribute_t));
  if (attrib == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  if (path == NULL)
    file = fopen(CONFIG_DEFAULT, "r");
  else
    file = fopen(path, "r");

  if (file == NULL) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, CONFIG_DEFAULT, strerror(errno));
    free(attrib);
    return NULL;
  }

  while(fgets(buffer, sizeof(buffer), file)!=NULL) {

    chomp(buffer);

    /* Remove comments */
    if ((c = strchr(buffer, '#')) || (c = strchr(buffer, ';')))
      *c = 0;

    /* Parse attribute name */
    if (!(a = tokenize(buffer, &bufptr)))
      continue; /* Ignore whitespace-only lines */

    while ((v = tokenize(NULL, &bufptr)) != NULL) {
      if (_ds_find_attribute(attrib, a)!=NULL) { 
        _ds_add_attribute(attrib, a, v);
      }
      else {
        num_root++;
        if (num_root >= attrib_size) {
          attrib_size *=2;
          ptr = realloc(attrib, attrib_size*sizeof(attribute_t)); 
          if (ptr) 
            attrib = ptr;
          else
            LOG(LOG_CRIT, ERR_MEM_ALLOC);
        } 
        _ds_add_attribute(attrib, a, v);
      }
    }
  }

  fclose(file);

  ptr = realloc(attrib, ((num_root+1)*sizeof(attribute_t))+1);
  if (ptr)
    return ptr;
  LOG(LOG_CRIT, ERR_MEM_ALLOC);
  return attrib;
}

int configure_algorithms(DSPAM_CTX *CTX) {
  if (_ds_read_attribute(agent_config, "Algorithm"))
    CTX->algorithms = 0;
                                                                                
  if (_ds_match_attribute(agent_config, "Algorithm", "graham"))
    CTX->algorithms |= DSA_GRAHAM;
                                                                                
  if (_ds_match_attribute(agent_config, "Algorithm", "burton"))
    CTX->algorithms |= DSA_BURTON;
                                                                                
  if (_ds_match_attribute(agent_config, "Algorithm", "robinson"))
    CTX->algorithms |= DSA_ROBINSON;

  if (_ds_match_attribute(agent_config, "Algorithm", "naive"))
    CTX->algorithms |= DSA_NAIVE;
                                                                                
  if (_ds_match_attribute(agent_config, "PValue", "robinson"))
    CTX->algorithms |= DSP_ROBINSON;
  else if (_ds_match_attribute(agent_config, "PValue", "markov"))
    CTX->algorithms |= DSP_MARKOV;
  else
    CTX->algorithms |= DSP_GRAHAM;

  if (_ds_match_attribute(agent_config, "Tokenizer", "word")) 
    CTX->tokenizer = DSZ_WORD;
  else if (_ds_match_attribute(agent_config, "Tokenizer", "chain") ||
           _ds_match_attribute(agent_config, "Tokenizer", "chained"))
    CTX->tokenizer = DSZ_CHAIN;
  else if (_ds_match_attribute(agent_config, "Tokenizer", "sbph"))
    CTX->tokenizer = DSZ_SBPH;
  else if (_ds_match_attribute(agent_config, "Tokenizer", "osb"))
    CTX->tokenizer = DSZ_OSB;
 
  if (_ds_match_attribute(agent_config, "Algorithm", "chi-square"))
  {
    if (CTX->algorithms != 0 && CTX->algorithms != DSP_ROBINSON) {
      LOG(LOG_WARNING, "Warning: Chi-Square algorithm enabled with other algorithms. False positives may ensue.");
    }
    CTX->algorithms |= DSA_CHI_SQUARE;
  }

  return 0;
}

agent_pref_t pref_config(void)
{
  agent_pref_t PTX = malloc(sizeof(agent_attrib_t)*PREF_MAX);
  agent_pref_t ptr;
  attribute_t attrib;
  char *p, *q;
  char *ptrptr;
  int i = 0;

  if (PTX == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
  PTX[0] = NULL;

  /* Apply default preferences from dspam.conf */
                                                                                
  attrib = _ds_find_attribute(agent_config, "Preference");
                                                                                
  LOGDEBUG("Loading preferences from dspam.conf");
                                                                                
  while(attrib != NULL) {
    char *pcopy = strdup(attrib->value);
                                                                              
    p = strtok_r(pcopy, "=", &ptrptr);
    if (p == NULL) {
      free(pcopy);
      continue;
    }
    q = p + strlen(p)+1;

    PTX[i] = _ds_pref_new(p, q);
    PTX[i+1] = NULL;

    i++;
    attrib = attrib->next;
    free(pcopy);
  }

  ptr = realloc(PTX, sizeof(agent_attrib_t)*(i+1));
  if (ptr)
    return ptr;
  
  LOG(LOG_CRIT, ERR_MEM_ALLOC);
  return PTX;
}
