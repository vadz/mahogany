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
 * dspamc.c - lightweight dspam client
 *
 * DESCRIPTION
 *   The lightweight client build is designed to perform identical to the full
 *   agent, but without any linkage to libdspam. Instead, the client calls
 *   the client-based functions (client.c) to perform processing by connecting
 *   to a DSPAM server process (dspam in --daemon mode).
 *
 *   This code-base is the client-only codebase. See dspam.c for the full
 *   agent base.
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H_
#include <unistd.h>
#include <pwd.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#define WIDEXITED(x) 1
#define WEXITSTATUS(x) (x)
#include <windows.h>
#else
#include <sys/wait.h>
#include <sys/param.h>
#endif
#include "config.h"
#include "util.h"
#include "read_config.h"
#ifdef DAEMON
#include "daemon.h"
#include "client.h"
#endif

#ifdef TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   ifdef HAVE_SYS_TIME_H
#       include <sys/time.h>
#   else
#       include <time.h>
#   endif
#endif

#include "dspamc.h"
#include "agent_shared.h"
#include "pref.h"
#include "libdspam.h"
#include "language.h"
#include "buffer.h"
#include "pref.h"
#include "error.h"

#ifdef DEBUG
int DO_DEBUG;
char debug_text[1024];
#endif

int
main (int argc, char *argv[])
{
  AGENT_CTX ATX;
  int exitcode = EXIT_SUCCESS;
  buffer *message = NULL;       /* input Message */
  int agent_init = 0;		/* agent is initialized */

  setbuf (stdout, NULL);	/* unbuffered output */
#ifdef DEBUG
  DO_DEBUG = 0;
#endif

  srand ((long) time << (long) getpid());
  umask (006);

#ifndef DAEMON
  LOG(LOG_ERR, ERR_DAEMON_NO_SUPPORT);
  exit(EXIT_FAILURE);
#endif

  /* Read dspam.conf into global config structure (ds_config_t) */

  agent_config = read_config(NULL);
  if (!agent_config) {
    LOG(LOG_ERR, ERR_AGENT_READ_CONFIG);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  if (!_ds_read_attribute(agent_config, "Home")) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  /* Set up agent context to define behavior of processor */

  if (initialize_atx(&ATX)) {
    LOG(LOG_ERR, ERR_AGENT_INIT_ATX);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  } else {
    agent_init = 1;
  }

  if (process_arguments(&ATX, argc, argv)) {
    LOG(LOG_ERR, ERR_AGENT_INIT_ATX);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  if (apply_defaults(&ATX)) {
    LOG(LOG_ERR, ERR_AGENT_INIT_ATX);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  if (check_configuration(&ATX)) {
    LOG(LOG_ERR, ERR_AGENT_MISCONFIGURED);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  /* Read the message in and apply ParseTo services */

  message = read_stdin(&ATX);
  if (message == NULL) {
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  if (ATX.users->items == 0)
  {
    LOG (LOG_ERR, ERR_AGENT_USER_UNDEFINED);
    fprintf (stderr, "%s\n", SYNTAX);
    exitcode = EXIT_FAILURE;
    goto BAIL;
  }

  /* Perform client-based processing */

#ifdef DAEMON
  if (_ds_read_attribute(agent_config, "ClientIdent") &&
      (_ds_read_attribute(agent_config, "ClientHost") ||
       _ds_read_attribute(agent_config, "ServerDomainSocketPath")))
  {
    exitcode = client_process(&ATX, message);
  } else {
    LOG(LOG_ERR, ERR_CLIENT_INVALID_CONFIG);
    exitcode = EINVAL;
  }
#endif

BAIL:

  if (message)
    buffer_destroy(message);

  if (agent_init)
    nt_destroy(ATX.users);

  if (agent_config)
    _ds_destroy_config(agent_config);

  exit (exitcode);
}

