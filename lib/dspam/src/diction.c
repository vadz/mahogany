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
 *  diction.c - subset of lexical data
 *
 *  DESCRIPTION
 *    a diction is a subset of lexical data from a user's dictionary. in the
 *    context used within DSPAM, a diction is all of the matching lexical
 *    information from the current message being processed. the diction is
 *    loaded/stored by the storage driver and managed primarily by libdspam.
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "diction.h"

static unsigned long _ds_prime_list[] = {
  53ul, 97ul, 193ul, 389ul, 769ul,
  1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
  49157ul, 98317ul, 196613ul, 393241ul, 786433ul,
  1572869ul, 3145739ul, 6291469ul, 12582917ul, 25165843ul,
  50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
  1610612741ul, 3221225473ul, 4294967291ul
};

ds_diction_t
ds_diction_create (unsigned long size)
{
  ds_diction_t diction = (ds_diction_t) calloc(1, sizeof(struct _ds_diction));
  int i = 0;

  if (!diction) {
    perror("ds_diction_create: calloc() failed");
    return NULL;
  }

  while (_ds_prime_list[i] < size) 
    { i++; }

  diction->size = _ds_prime_list[i];
  diction->items = 0;
  diction->tbl =
    (struct _ds_term **) calloc(diction->size, sizeof (struct _ds_term *));
  if (!diction->tbl)
  {
    perror("ds_diction_create: calloc() failed");
    free(diction);
    return NULL;
  }

  diction->order = nt_create(NT_INDEX);
  diction->chained_order = nt_create(NT_INDEX);
  if (!diction->order || !diction->chained_order) {
    nt_destroy(diction->order);
    nt_destroy(diction->chained_order);
    free(diction->tbl);
    free(diction);
    return NULL;
  }

  return diction;
}

void
ds_diction_destroy (ds_diction_t diction)
{
  ds_term_t term, next;
  ds_cursor_t cur;

  if (!diction) return;

  cur = ds_diction_cursor(diction);
  if (!cur) {
    perror("ds_diction_destroy: ds_diction_cursor() failed");
    return;
  } 

  term = ds_diction_next(cur);
  while(term)
  {
    next = ds_diction_next(cur);
    ds_diction_delete(diction, term->key);
    term = next;
  }
  ds_diction_close(cur);

  nt_destroy(diction->order);
  nt_destroy(diction->chained_order);
  free(diction->tbl);
  free(diction); 
  return;
}

ds_term_t
ds_diction_term_create (ds_key_t key, const char *name)
{
  ds_term_t term = (ds_term_t) calloc(1, sizeof(struct _ds_term));

  if (!term) {
    perror("ds_diction_term_create: calloc() failed");
  } else {
    term->key = key;
    term->frequency = 1;
    term->type = 'D';
    if (name)
      term->name = strdup(name);
  }
  return term;
}

ds_term_t
ds_diction_find (ds_diction_t diction, ds_key_t key)
{
  ds_term_t term;

  term = diction->tbl[key % diction->size];
  while (term)
  {
    if (key == term->key)
      return term;
    term = term->next;
  }

  return NULL;
}

ds_term_t
ds_diction_touch(
  ds_diction_t diction, 
  ds_key_t key, 
  const char *name, 
  int flags)
{
  unsigned long long bucket = key % diction->size;
  ds_term_t parent = NULL;
  ds_term_t insert = NULL;
  ds_term_t term;

  term = diction->tbl[bucket];
  while (term) {
    if (key == term->key) {
      insert = term;
      break;
    }
    parent = term;
    term = term->next;
  }

  if (!insert) {
    insert = ds_diction_term_create(key, name);
    if (!insert) {
      perror("ds_diction_touch: ds_diction_term_create() failed");
      return NULL;
    }
    diction->items++;
    if (parent) 
      parent->next = insert;
    else
      diction->tbl[bucket] = insert;
  } else {
    if (!insert->name && name) 
      insert->name = strdup(name);
    insert->frequency++;
  }

  if (flags & DSD_CONTEXT) {
    if (flags & DSD_CHAINED)
      nt_add(diction->chained_order, insert);
    else
      nt_add(diction->order, insert);
  }

  return insert;
}

void
ds_diction_delete(ds_diction_t diction, ds_key_t key)
{
  unsigned long long bucket = key % diction->size;
  ds_term_t parent = NULL;
  ds_term_t delete = NULL;
  ds_term_t term;

  term = diction->tbl[bucket];

  while(term) {
    if (key == term->key) {
      delete = term;
      break;
    }
    parent = term;
    term = term->next;
  }

  if (delete) {
    if (parent)
      parent->next = delete->next;
    else
      diction->tbl[bucket] = delete->next;

    free(delete->name);
    free(delete);
    diction->items--;
  }
  return;
}

ds_cursor_t
ds_diction_cursor (ds_diction_t diction)
{
  ds_cursor_t cur = (ds_cursor_t) calloc(1, sizeof(struct _ds_diction_c));

  if (!cur) {
    perror("ds_diction_cursor: calloc() failed");
    return NULL;
  }
  cur->diction    = diction;
  cur->iter_index = 0;
  cur->iter_next  = NULL;
  return cur;
}

ds_term_t
ds_diction_next (ds_cursor_t cur)
{
  unsigned long bucket;
  ds_term_t term;
  ds_term_t tbl_term;

  if (!cur)
    return NULL;

  term = cur->iter_next;
  if (term) {
    cur->iter_next = term->next;
    return term;
  }

  while (cur->iter_index < cur->diction->size) {
    bucket = cur->iter_index;
    cur->iter_index++;
    tbl_term = cur->diction->tbl[bucket];
    if (tbl_term) {
      cur->iter_next = tbl_term->next;
      return (tbl_term);
    }
  }

  return NULL;
}

void
ds_diction_close (ds_cursor_t cur)
{
  free(cur);
  return;
}

int
ds_diction_setstat (ds_diction_t diction, ds_key_t key, ds_spam_stat_t s)
{
  ds_term_t term = ds_diction_find(diction, key);

  if (term) {
    term->s.probability = s->probability;
    term->s.spam_hits = s->spam_hits;
    term->s.innocent_hits = s->innocent_hits;
    term->s.status = s->status;
    term->s.offset = s->offset;
    return 0;
  }
  return -1;
}

int ds_diction_addstat (ds_diction_t diction, ds_key_t key, ds_spam_stat_t s)
{
  ds_term_t term = ds_diction_find(diction, key);

  if (term) {
    term->s.probability += s->probability;
    term->s.spam_hits += s->spam_hits;
    term->s.innocent_hits += s->innocent_hits;
    if (!term->s.offset)
      term->s.offset = s->offset;
    if (s->status & TST_DISK)
      term->s.status |= TST_DISK;
    if (s->status & TST_DIRTY)
      term->s.status |= TST_DIRTY;
    return 0;
  }
  return -1;
}

int
ds_diction_getstat  (ds_diction_t diction, ds_key_t key, ds_spam_stat_t s)
{
  ds_term_t term = ds_diction_find(diction, key);

  if (term) {
    s->probability = term->s.probability;
    s->spam_hits = term->s.spam_hits;
    s->innocent_hits = term->s.innocent_hits;
    s->status = term->s.status;
    s->offset = term->s.offset;
    return 0;
  }
  return -1;
}

