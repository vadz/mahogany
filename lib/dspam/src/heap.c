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
 * heap.c - fast heap-based sorting algorithm with maximum window size
 *
 * DESCRIPTION
 *   This sorting algorithm is designed to perform very efficiently when there 
 *   is a small window-size of 'peak' values, such as the 15 bayes slots.
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include "heap.h"

ds_heap_t
ds_heap_create(int size, int type)
{
  ds_heap_t h;

  h = calloc(1, sizeof(struct _ds_heap));
  h->size = size;
  h->type = type;
  return h;
}

void
ds_heap_destroy(ds_heap_t h)
{
  ds_heap_element_t node, next;

  if (h) {
    node = h->root;
    while(node) {
      next = node->next;
      free(node);
      node = next;
    }
    free(h);
  }
  return;
}

ds_heap_element_t
ds_heap_element_create (double probability, 
                  unsigned long long token, 
                  unsigned long frequency,
                  int complexity)
{
  ds_heap_element_t element = calloc(1, sizeof(struct _ds_heap_element));

  if (!element)
    return NULL;

  element->delta       = fabs(0.5-probability);
  element->probability = probability;
  element->token       = token;
  element->frequency   = frequency;
  element->complexity  = complexity;

  return element;
}

ds_heap_element_t
ds_heap_insert (ds_heap_t h,
             double probability,
             unsigned long long token,
             unsigned long frequency,
             int complexity)
{
  ds_heap_element_t current = NULL;
  ds_heap_element_t insert = NULL;
  ds_heap_element_t node;
  float delta = fabs(0.5-probability);

  current = h->root;

  /* Determine if and where we should insert this item */
  if (h->type == HP_DELTA) {
    while(current) {
      if (delta > current->delta) 
        insert = current;
      else if (delta == current->delta) {
        if (frequency > current->frequency)
          insert = current;
        else if (frequency == current->frequency)
          if (complexity >= current->complexity)
            insert = current;
      }
      if (!insert)
        break;
      else
        current = current->next;
    }
  } else {
   while(current) {
      if (probability > current->probability)
        insert = current;
      if (!insert)
        break;
      else
        current = current->next;
    }
  }

  if (insert != NULL) {

    /* Insert item, throw out new least significant item if necessary */
    node = ds_heap_element_create( probability, token, frequency, complexity);
    node->next = insert->next;
    insert->next = node;
    h->items++;
    if (h->items > h->size) {
      node = h->root;
      h->root = node->next;
      free(node);
      h->items--;
    }
  } else {
    
    /* Item is least significant; throw it out or grow the heap */
    if (h->items == h->size)
      return NULL;

    /* Grow heap */
    node = ds_heap_element_create(probability, token, frequency, complexity);
    node->next = h->root;
    h->root = node;
    h->items++;
  }

  return node;
}

