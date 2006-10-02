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

#ifndef _LANGUAGE_H
#  define _LANGUAGE_H

#define LANGUAGE	"en"

/* General purpose error codes */

#define ERR_MEM_ALLOC		"Memory allocation failed"
#define ERR_IO_FILE_OPEN	"Unable to open file for reading: %s: %s"
#define ERR_IO_FILE_WRITE	"Unable to open file for writing: %s: %s"
#define ERR_IO_FILE_CLOSE	"Unable to close file: %s: %s"
#define ERR_IO_FILE_RENAME	"Unable to rename file: %s: %s"
#define ERR_IO_DIR_CREATE	"Unable to create directory: %s: %s"
#define ERR_IO_LOCK             "Failed to lock file %s: %d: %s"
#define ERR_IO_LOCK_FREE	"Failed to free lock file %s: %d: %s"

/* LDAP related error codes */

#define ERR_LDAP_INIT_FAIL	"LDAP initialization failed"
#define ERR_LDAP_PROTO_VER_FAIL	"LDAP failure: unable to set protocol version"
#define ERR_LDAP_SEARCH_FAIL	"LDAP search failure"
#define ERR_LDAP_MISCONFIGURED	"LDAP misconfigured"

/* Agent error codes */

#define ERR_AGENT_USER_UNDEFINED  "Unable to determine the destination user"
#define ERR_AGENT_READ_CONFIG     "Unable to read dspam.conf"
#define ERR_AGENT_DSPAM_HOME	  "No DSPAM home specified"
#define ERR_AGENT_RUNTIME_USER    "Unable to determine the runtime user"
#define ERR_AGENT_NO_SUCH_PROFILE "No such profile '%s'"
#define ERR_AGENT_NO_SUCH_CLASS   "No such class '%s'"
#define ERR_AGENT_NO_SUCH_SOURCE  "No such source '%s'"
#define ERR_AGENT_NO_SUCH_DELIVER "No such delivery option '%s'"
#define ERR_AGENT_NO_SUCH_FEATURE "No such feature '%s'"
#define ERR_AGENT_NO_AGENT        "No %s configured, and --stdout not specified"
#define ERR_AGENT_NO_SOURCE       "No source was specified for this class"
#define ERR_AGENT_NO_CLASS        "No class was specified for this source"
#define ERR_AGENT_CLASSIFY_CLASS  "Classify mode may not specify a class"
#define ERR_AGENT_NO_OP_MODE      "No operating mode was specified"
#define ERR_AGENT_NO_TR_MODE      "No training mode was specified"
#define ERR_AGENT_TB_INVALID      "Training buffer level must be 0-10"
#define ERR_AGENT_TR_MODE_INVALID "Invalid training mode specified"
#define ERR_AGENT_IGNORE_PREF     "Ignoring disallowed preference '%s'"
#define ERR_AGENT_INIT_ATX        "Unable to initialize agent context"
#define ERR_AGENT_MISCONFIGURED   "DSPAM agent misconfigured: aborting"
#define ERR_AGENT_FAILOVER        "Failing over to storage profile '%s'"
#define ERR_AGENT_FAILOVER_OUT    "Could not fail over: out of failover servers"
#define ERR_AGENT_CLEAR_ATTRIB    "Unable to clear attributes list"
#define ERR_AGENT_PARSER_FAILED   "Message parser failed. Unable to continue."
#define ERR_AGENT_SIG_RET_FAILED  "Signature retrieval for '%s' failed"
#define ERR_AGENT_NO_VALID_SIG    "Unable to find a valid signature. Aborting."
#define ERR_AGENT_OPTIN_DIR       "The opt-in file %s should be a directory"

/* Local delivery agent error codes */

#define ERR_LDA_EXIT      "Delivery agent returned exit code %d: %s"
#define ERR_LDA_SIGNAL    "Delivery agent terminated by signal %d: %s"
#define ERR_LDA_OPEN      "Error opening pipe to delivery agent: %s: %s"
#define ERR_LDA_CLOSE     "Unexpected return code %d when closing pipe"
#define ERR_LDA_STATUS    "Error getting exit status of delivery agent: %s: %s"

/* Client error codes */

#define ERR_CLIENT_EXIT           "Client exited with error %d"

/* Trusted-user related error codes */

#define ERR_TRUSTED_USER \
"Option --user requires special privileges when user does not match current \
user, e.g.. root or Trusted User [uid=%d(%s)]"

#define ERR_TRUSTED_PRIV \
"Option %s requires special privileges; e.g.. root or Trusted User [uid=%d(%s)]"

#define ERR_TRUSTED_MODE \
"Program mode requires special privileges, e.g., root or Trusted User"

/* DSPAM core engine (libdspam) related error codes */

#define ERR_CORE_INIT             "Context initialization failed"
#define ERR_CORE_ATTACH           "Unable to attach DSPAM context"
#define ERR_CORE_REATTACH         "Unable to attach DSPAM context. Retrying."

/* Storage driver error codes */

#define ERR_DRV_NO_ATTACH	"Driver does not support dbh attach"
#define ERR_DRV_NO_MERGED	"Driver does not support merged groups"
#define ERR_DRV_INIT            "Unable to initialize storage driver"

/* Daemon-mode related info codes */

#define INFO_DAEMON_START	"Daemon process starting"
#define INFO_DAEMON_EXIT	"Daemon process exiting"
#define INFO_DAEMON_BIND        "Binding to :%d"
#define INFO_DAEMON_DOMAINSOCK  "Creating local domain socket %s"
#define INFO_DAEMON_RELOAD      "Reloading configuration"

/* Daemon-mode related error codes */

#define ERR_DAEMON_NO_SUPPORT   "DSPAM was not compiled with daemon support"
#define ERR_DAEMON_BIND         "Could not bind to :%d: %s"
#define ERR_DAEMON_DOMAINBIND	"Could not bind to %s: %s"
#define ERR_DAEMON_SOCKET	"Could not create socket: %s"
#define ERR_DAEMON_SOCKOPT	"Could not set sockopt %s: %s"
#define ERR_DAEMON_LISTEN	"Unable to listen: %s"
#define ERR_DAEMON_ACCEPT	"Unable to accept: %s"
#define ERR_DAEMON_THREAD	"Thread creation failed: %s"
#define ERR_DAEMON_FAIL         "Daemon mode failed to start"
#define ERR_DAEMON_TERMINATE    "Daemon terminating on signal %d"

/* LMTP (externally visible) info codes */

#define INFO_LMTP_DATA          "Enter mail, end with \".\" on a line by itself"

/* LMTP (externally visible) error codes */

#define ERR_LMTP_MSG_NULL	"Message is empty. Aborting."
#define ERR_LMTP_BAD_RCPT	"Invalid RCPT TO. Use RCPT TO: <recipient>"

/* Client-mode related info codes */

#define INFO_CLIENT_CONNECTED           "Connection established"
#define INFO_CLIENT_CONNECTING          "Establishing connection to %s:%d"

/* Client-mode related error codes */

#define ERR_CLIENT_INVALID_CONFIG	"Invalid client configuration"
#define ERR_CLIENT_CONNECT		"Unable to connect to server"
#define ERR_CLIENT_IDENT		"No ClientIdent provided in dspam.conf"
#define ERR_CLIENT_CONNECT_SOCKET	"Connection to socket %s failed: %s"
#define ERR_CLIENT_CONNECT_HOST	        "Connection to %s:%d failed: %s"
#define ERR_CLIENT_AUTH_FAILED	        "Unable to authenticate client"
#define ERR_CLIENT_AUTHENTICATE	        "Authentication rejected"
#define ERR_CLIENT_WHILE_AUTH		"Error while authenticating: %s"
#define ERR_CLIENT_ON_GREETING	        "Received error on greeting: %s"
#define ERR_CLIENT_SEND_FAILED		"Packet send failure"
#define ERR_CLIENT_INVALID_RESPONSE     "Received error in response to %s: %s"
#define ERR_CLIENT_DELIVERY_FAILED      "Delivery failed completely"
#define ERR_CLIENT_RESPONSE             "Got error %d in response to %s: %s"
#define ERR_CLIENT_RESPONSE_CODE        "Invalid data waiting for code %d: %s"

/* Classes */

#define LANG_CLASS_WHITELISTED		"Whitelisted"
#define LANG_CLASS_SPAM			"Spam"
#define LANG_CLASS_INNOCENT		"Innocent"
#define LANG_CLASS_VIRUS		"Virus"
#define LANG_CLASS_BLOCKLISTED		"Blocklisted"
#define LANG_CLASS_BLACKLISTED		"Blacklisted"

#endif /* _LANGUAGE_H */
