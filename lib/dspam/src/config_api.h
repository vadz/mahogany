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

#ifndef _CONFIG_API_H
#define _CONFIG_API_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include "libdspam.h"

int set_libdspam_attributes(DSPAM_CTX *CTX);
int attach_context         (DSPAM_CTX *CTX, void *dbh);

#endif /* _CONFIG_API_H */
