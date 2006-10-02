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

#include <stdlib.h>
#include <sys/types.h>
#ifndef _WIN32
#include <pwd.h>
#endif

#include "nodetree.h"
#include "util.h"
#include "error.h"
#include "libdspam_objects.h"
#include "language.h"

/* nt_node_create (used internally) to allocate space for a new node */
struct nt_node * nt_node_create (void *data) {
  struct nt_node *node;
  if ((node = (struct nt_node *) malloc (sizeof (struct nt_node))) == 0)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    exit (1);
  }
  node->ptr = data;
  node->next = (struct nt_node *) NULL;
  return (node);
}

/* nt_create allocates space for and initializes a nodetree */
struct nt *
nt_create (int nodetype)
{
  struct nt *nt = (struct nt *) malloc (sizeof (struct nt));
  if (nt == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
  nt->first = (struct nt_node *) NULL;
  nt->insert = (struct nt_node *) NULL;
  nt->items = 0;
  nt->nodetype = nodetype;
  return (nt);
}

/* nt_destroy methodically destroys a nodetree, freeing resources */
void
nt_destroy (struct nt *nt)
{
  struct nt_node *cur, *next;
  int i;
  if (!nt)
    return;

  cur = nt->first;
  for (i = 0; i < nt->items; i++)
  {
    next = cur->next;
    if (nt->nodetype != NT_INDEX)
      free (cur->ptr);
    free (cur);
    cur = next;
  }
  free (nt);
}

/* nt_add adds an item to the nodetree */
struct nt_node *
nt_add (struct nt *nt, void *data)
{
  struct nt_node *prev;
  struct nt_c c;
  struct nt_node *node = c_nt_first (nt, &c);
  void *vptr;

  if (nt->insert) {
    prev = nt->insert;
  } else {
    prev = 0;
    while (node)
    {
      prev = node;
      node = node->next;
    }
  }
  nt->items++;

  if (nt->nodetype == NT_CHAR)
  {
    long size = strlen ((char *) data) + 1;
    vptr = malloc (size);
    if (vptr == NULL)
    {
      LOG (LOG_CRIT, ERR_MEM_ALLOC);
      return NULL;
    }
    strlcpy (vptr, data, size);
  }
  else
  {
    vptr = data;
  }

  if (prev)
  {
    node = nt_node_create (vptr);
    prev->next = node;
    nt->insert = node;
    return (node);
  }
  else
  {
    node = nt_node_create (vptr);
    nt->first = node;
    nt->insert = node;
    return (node);
  }
}

/* c_nt_next returns the next item in a nodetree */
struct nt_node *
c_nt_next (struct nt *nt, struct nt_c *c)
{
  struct nt_node *node = c->iter_index;
  if (node)
  {
    c->iter_index = node->next;
    return (node->next);
  }
  else
  {
    if (nt->items > 0)
    {
      c->iter_index = nt->first;
      return nt->first;
    }
  }

  return ((struct nt_node *) NULL);
}

/* nt_first returns the first item in a nodetree */
struct nt_node *
c_nt_first (struct nt *nt, struct nt_c *c)
{
  c->iter_index = nt->first;
  return (nt->first);
}
