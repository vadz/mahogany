/* $Id$ */

/*
  Bayesian Noise Reduction - Contextual Symmetry Logic
  http://bnr.nuclearelephant.com
  Copyright (c) 2004 Jonathan A. Zdziarski

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

#ifndef _BNR_HASH_H
#define _BNR_HASH_H

enum
{ bnr_hash_num_primes = 28 };

/* bnr_hash root */
struct bnr_hash
{
  unsigned long size;
  unsigned long items;
  struct bnr_hash_node **tbl;
};

/* bnr_hash node */
struct bnr_hash_node
{
  struct bnr_hash_node *next;

  char *name;
  float value;
};

/* bnr_hash cursor */
struct bnr_hash_c
{
  unsigned long iter_index;
  struct bnr_hash_node *iter_next;
};

/* constructor and destructor */
struct bnr_hash *	bnr_hash_create (unsigned long size);
int		bnr_hash_destroy (struct bnr_hash *hash);

int bnr_hash_set	(struct bnr_hash *hash, const char *name, float value);
int bnr_hash_hit	(struct bnr_hash *hash, const char *name);
int bnr_hash_delete	(struct bnr_hash *hash, const char *name);
float bnr_hash_value(struct bnr_hash *hash, const char *name);

struct bnr_hash_node *bnr_hash_node_create (const char *name);
long bnr_hash_hashcode(struct bnr_hash *hash, const char *name);

/* iteration functions */
struct bnr_hash_node *c_bnr_hash_first	(struct bnr_hash *hash, struct bnr_hash_c *c);
struct bnr_hash_node *c_bnr_hash_next	(struct bnr_hash *hash, struct bnr_hash_c *c);

#endif /* _BNR_HASH_H */
