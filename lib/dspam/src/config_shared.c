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

/*
 * config_shared.c - attributes-based configuration functionsc
 *
 * DESCRIPTION
 *   Attribtues are used by the agent and libdspam to control configuration
 *   management. The included functions perform various operations on the
 *   configuration structures supplied.
 *
 *   Because these functions are used by libdspam, they are prefixed with _ds_
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "config_shared.h"
#include "error.h"
#include "language.h"
#include "libdspam.h"

attribute_t _ds_find_attribute(config_t config, const char *key) {
  int i;

  if (config == NULL) { return NULL; }

  for(i=0;config[i];i++) {
    attribute_t attr = config[i];
    if (!strcasecmp(attr->key, key)) {
      return attr;
    }
  } 

  return NULL;
}

int _ds_add_attribute(config_t config, const char *key, const char *val) {
  attribute_t attr;
  int i;

#ifdef VERBOSE
  LOGDEBUG("attribute %s = %s", key, val);
#endif

  attr = _ds_find_attribute(config, key);
  if (!attr) {
    for(i=0;config[i];i++) { }
    config[i+1] = 0;
    config[i] = malloc(sizeof(struct attribute));
    if (!config[i]) {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      return EUNKNOWN;
    }
    attr = config[i];
  } else {
    while(attr->next != NULL) {
      attr = attr->next;
    }
    attr->next = malloc(sizeof(struct attribute));
    if (!attr->next) {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      return EUNKNOWN;
    }
    attr = attr->next;
  }
  attr->key = strdup(key);
  attr->value = strdup(val);
  attr->next = NULL;

  return 0;
}

int _ds_overwrite_attribute(config_t config, const char *key, const char *val) 
{
  attribute_t attr;

#ifdef VERBOSE
  LOGDEBUG("overwriting attribute %s with value '%s'", key, val);
#endif

  attr = _ds_find_attribute(config, key);
  if (attr == NULL) {
    return _ds_add_attribute(config, key, val);
  } 

  free(attr->value);
  attr->value = strdup(val);

  return 0;
}

char *_ds_read_attribute(config_t config, const char *key) {
  attribute_t attr = _ds_find_attribute(config, key);

  if (!attr)
    return NULL;
  return attr->value;
}

int _ds_match_attribute(config_t config, const char *key, const char *val) {
  attribute_t attr;

  attr = _ds_find_attribute(config, key);
  if (!attr) 
    return 0;

  while(strcasecmp(attr->value, val) && attr->next != NULL) 
    attr = attr->next;

  if (!strcasecmp(attr->value, val))
    return 1;

  return 0;
}

void _ds_destroy_config(config_t config) {
  attribute_t x, y;
  int i;

  for(i=0;config[i];i++) {
    x = config[i];

    while(x) {
      y = x->next;
      free(x->key);
      free(x->value);
      free(x);
      x = y;
    }
  }
  free(config);
  return;
}
