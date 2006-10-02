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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#ifndef _NODETREE_H
#define _NODETREE_H

/* Nodetree: An ordered dynamic data collection structure
 *
 *  Nodetree is designed to provide an ordered collective dynamic data
 *  structure for arrays of unknown size, where it is impractical to
 *  overallocate space.  Nodetree allows the user to allocate memory for
 *  new items as needed, and provides simple management and iteration
 *  functionality.  It is a linked list on steroids.
 *
 *  Nodetree types:
 *
 *  NT_CHAR	Character Nodetree
 *		Treats the passed pointer as a const char * and creates
 * 		new storage space for each string
 *
 *  NT_PTR	Pointer Nodetree
 *		Does not perform string conversion, instead just stores
 *		the pointer's location in the nodetree.  Prior to adding
 *              an item, the user should malloc storage for the new
 *              structure and pass the pointer to the nodetree functions.
 *
 *  NT_INDEX	Pointer Index
 *              Same as Pointer Nodetree, only does not free() the ptr
 *              objects upon deletion
 */

#define NT_CHAR		0x00
#define NT_PTR		0x01
#define NT_INDEX	0x02

struct nt_node
{
  void *ptr;
  struct nt_node *next;
};

struct nt
{
  struct nt_node *first;
  struct nt_node *insert;	/* Next insertion point */
  int items;
  int nodetype;
};

struct nt_c
{
  struct nt_node *iter_index;
};

struct nt_node *	nt_add		(struct nt *nt, void *data);
struct nt_node *	c_nt_first	(struct nt *nt, struct nt_c *c);
struct nt_node *	c_nt_next	(struct nt *nt, struct nt_c *c);
struct nt *		nt_create	(int node_type);
void			nt_destroy	(struct nt *nt);
struct nt_node *	nt_node_create	(void *data);

#endif /* _NODETREE_H */

