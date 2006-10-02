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

#include <sys/types.h>
#ifndef _WIN32
#include <pwd.h>
#endif
#include "libdspam.h"
#include "buffer.h"
#include "pref.h"
#include "read_config.h"
#include "daemon.h"

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include "agent_shared.h"

#ifndef _DSPAM_H
#  define _DSPAM_H

int deliver_message (AGENT_CTX *ATX, const char *message, 
    const char *mailer_args, const char *username, FILE *out, int result);
int process_message (AGENT_CTX *ATX, buffer *message, const char *username,
     char **result_string);
int inoculate_user  (AGENT_CTX *ATX, const char *username, 
    struct _ds_spam_signature *SIG, const char *message);
int user_classify   (AGENT_CTX *ATX, const char *username,
     struct _ds_spam_signature *SIG, const char *message);
int send_notice     (AGENT_CTX *ATX, const char *filename, 
    const char *mailer_args, const char *username);
int write_web_stats (AGENT_CTX *ATX, const char *username, const char *group, 
    struct _ds_spam_totals *totals);
int ensure_confident_result (DSPAM_CTX *CTX, AGENT_CTX *ATX, int result);
int quarantine_message      (AGENT_CTX *ATX, const char *message, 
    const char *username);
int log_events         (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int retrain_message    (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int tag_message        (AGENT_CTX *ATX, ds_message_t message);
int process_users      (AGENT_CTX *ATX, buffer *message);
int find_signature     (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int add_xdspam_headers (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int embed_signature    (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int embed_msgtag       (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int embed_signed       (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int tracksource        (DSPAM_CTX *CTX);
#ifdef CLAMAV
int has_virus          (buffer *message);
int feed_clam          (int port, buffer *message);
#endif
int is_blacklisted     (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int is_blocklisted     (DSPAM_CTX *CTX, AGENT_CTX *ATX);
int do_notifications   (DSPAM_CTX *CTX, AGENT_CTX *ATX);
DSPAM_CTX *ctx_init    (AGENT_CTX *ATX, const char *username);
buffer *read_stdin     (AGENT_CTX *ATX);
agent_pref_t load_aggregated_prefs (AGENT_CTX *ATX, const char *username);

#ifdef DAEMON
int daemon_start       (AGENT_CTX *ATX);
#endif

#define DSM_DAEMON	0xFE

typedef struct agent_result {
  int exitcode;
  int classification;
  char text[256];
} *agent_result_t; 

#define ERC_SUCCESS		0x00
#define ERC_PROCESS		-0x01
#define ERC_DELIVERY		-0x02
#define ERC_PERMANENT_DELIVERY	-0x03

#endif /* _DSPAM_H */

