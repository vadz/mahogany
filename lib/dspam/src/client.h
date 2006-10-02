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

#ifndef _CLIENT_H
#  define _CLIENT_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#ifdef DAEMON

#include <sys/types.h>
#ifndef _WIN32
#include <pwd.h>
#endif

#include "dspam.h"
#include "buffer.h"

int client_process      (AGENT_CTX *ATX, buffer *msg);
int client_connect      (AGENT_CTX *ATX, int flags);
int client_authenticate (THREAD_CTX *TTX, const char *mode);
int client_parsecode	(char *err);
int client_getcode      (THREAD_CTX *TTX, char *err, size_t len);
char * client_expect    (THREAD_CTX *TTX, int code, char *err, size_t len);
char * client_getline   (THREAD_CTX *TTX, int timeout);
int deliver_socket      (AGENT_CTX *ATX, const char *msg, int proto);

char *pop_buffer  (THREAD_CTX *TTX);
int   send_socket (THREAD_CTX *TTX, const char *ptr);

#define CCF_PROCESS	0x00	/* use ClientHost */
#define CCF_DELIVERY	0x01	/* use DeliveryHost */

#define DDP_LMTP	0x00
#define DDP_SMTP	0x01

#endif /* _CLIENT_H */

#endif /* DAEMON */
