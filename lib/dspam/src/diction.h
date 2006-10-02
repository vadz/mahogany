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

#ifndef _DICTION_H
#	define _DICTION_H

#include "nodetree.h"
#include "libdspam_objects.h"

typedef struct _ds_diction
{
  unsigned long size;
  unsigned long items;
  struct _ds_term **tbl;

  unsigned long long whitelist_token;
  struct nt *order;
  struct nt *chained_order;
} *ds_diction_t;

typedef struct _ds_term
{
  unsigned long long key;
  struct _ds_term *next;

  int frequency;
  struct _ds_spam_stat s;
  char *name;
  char type;

} *ds_term_t;

typedef struct _ds_diction_c
{
  struct _ds_diction *diction;
  unsigned long iter_index;
  ds_term_t iter_next;
} *ds_cursor_t;

typedef unsigned long long ds_key_t;

ds_diction_t ds_diction_create(unsigned long size);
void ds_diction_destroy(ds_diction_t diction);

ds_term_t ds_diction_term_create(ds_key_t key, const char *name);
ds_term_t ds_diction_find(ds_diction_t diction, ds_key_t key);
ds_term_t ds_diction_touch(ds_diction_t diction, ds_key_t key, 
  const char *name, int flags);
void ds_diction_delete(ds_diction_t diction, ds_key_t key);
int ds_diction_setstat(ds_diction_t diction, ds_key_t key, ds_spam_stat_t s);
int ds_diction_addstat(ds_diction_t diction, ds_key_t key, ds_spam_stat_t s);
int ds_diction_getstat(ds_diction_t diction, ds_key_t key, ds_spam_stat_t s);

ds_cursor_t ds_diction_cursor(ds_diction_t diction);
ds_term_t ds_diction_next(ds_cursor_t cur);
void ds_diction_close(ds_cursor_t cur);

#define DSD_CHAINED	0x01
#define DSD_CONTEXT	0x02

#endif /* _DICTION_H */
