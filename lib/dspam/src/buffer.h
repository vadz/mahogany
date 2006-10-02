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

#ifndef _BUFFER_H
#define _BUFFER_H

typedef struct
{
  long	size;
  long	used;
  char *data;
} buffer;

buffer *buffer_create	(const char *);
void	buffer_destroy	(buffer *);

int buffer_copy		(buffer *, const char *);
int buffer_cat		(buffer *, const char *);
int buffer_clear	(buffer *);

#endif /* _BUFFER_H */
