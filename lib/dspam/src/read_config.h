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

#ifndef _READ_CONFIG_H
#define _READ_CONFIG_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include "config_shared.h"
#include "libdspam.h"
#include "pref.h"

config_t read_config (const char *path);
int configure_algorithms (DSPAM_CTX * CTX);

agent_pref_t pref_config(void);
config_t agent_config;

#endif /* _READ_CONFIG_H */
