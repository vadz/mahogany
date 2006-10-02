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
 * agent_shared.c - shared agent-based components
 *
 * DESCRIPTION
 *   agent-based components shared between the full dspam agent (dspam) 
 *   and the lightweight client agent (dspamc)
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
#include "util.h"
#include "read_config.h"
#ifdef DAEMON
#include "daemon.h"
#include "dspam.h"
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

#include "agent_shared.h"
#include "language.h"
#include "buffer.h"

char * __pw_name = NULL;
uid_t  __pw_uid;

/*
 * initialize_atx(AGENT_CTX *)
 *
 * DESCRIPTION
 *  initializes an existing agent context
 *
 * INPUT ARGUMENTS
 *  ATX    agent context to initialize
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int initialize_atx(AGENT_CTX *ATX) {
  memset(ATX, 0, sizeof(AGENT_CTX));
  ATX->training_mode   = DST_DEFAULT;
  ATX->training_buffer = 0;
  ATX->train_pristine  = 0;
  ATX->classification  = DSR_NONE;
  ATX->source          = DSS_NONE;
  ATX->operating_mode  = DSM_PROCESS;
  ATX->users           = nt_create (NT_CHAR);

  if (ATX->users == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

#ifdef TRUSTED_USER_SECURITY
  if (!__pw_name) {
    LOG(LOG_ERR, ERR_AGENT_RUNTIME_USER);
    exit(EXIT_FAILURE);
  }

  LOGDEBUG("checking trusted user list for %s(%d)", __pw_name, __pw_uid);

  if (__pw_uid == 0)
    ATX->trusted = 1;
  else
    ATX->trusted = _ds_match_attribute(agent_config, "Trust", __pw_name);

  if (!ATX->trusted)
    nt_add (ATX->users, __pw_name);
#endif

  return 0;
}

/*
 * process_arguments(AGENT_CTX *, int argc, char *argv[])
 *
 * DESCRIPTION
 *   master commandline argument process loop
 *
 * INPUT ARGUMENTS
 *      ATX     agent context
 *      argc    number of arguments provided
 *      argv    array of arguments
 *
 * RETURN VALUES
 *  returns 0 on success, EINVAL when invalid options specified
 */

int process_arguments(AGENT_CTX *ATX, int argc, char **argv) {
  int flag_u = 0, flag_r = 0;
  int client = (_ds_read_attribute(agent_config, "ClientHost") != NULL);
  char *ptrptr;
  int i;

#ifdef DEBUG
  ATX->debug_args[0] = 0;
#endif
  ATX->client_args[0] = 0;

  for (i=0; i<argc; i++)
  {

#ifdef DEBUG
    strlcat (ATX->debug_args, argv[i], sizeof (ATX->debug_args));
    strlcat (ATX->debug_args, " ", sizeof (ATX->debug_args));
#endif

    /* Terminate user/rcpt lists */

    if ((flag_u || flag_r) &&
        (argv[i][0] == '-' || argv[i][0] == 0 || !strcmp(argv[i], "--")))
    {
       flag_u = flag_r = 0;
       if (!strcmp(argv[i], "--"))
         continue;
    }

    if (!strcmp (argv[i], "--user")) {
      flag_u = 1;
      continue;
    }

    if (!strcmp (argv[i], "--rcpt-to"))
    {
      if (!ATX->recipients) {
        ATX->recipients = nt_create(NT_CHAR);
        if (ATX->recipients == NULL) {
          LOG(LOG_CRIT, ERR_MEM_ALLOC);
          return EUNKNOWN;
        }
      }
      flag_r = 1;
      continue;
    }

    /* Build arg list to pass to server (when in client/server mode) */
 
    if (client && !flag_u && !flag_r && i>0) 
    {
      if (argv[i][0] == 0)
        strlcat(ATX->client_args, "\"", sizeof(ATX->client_args));
      strlcat (ATX->client_args, argv[i], sizeof(ATX->client_args));
      if (argv[i][0] == 0)
        strlcat(ATX->client_args, "\"", sizeof(ATX->client_args));
      strlcat (ATX->client_args, " ", sizeof(ATX->client_args));
    }

    if (!strcmp (argv[i], "--debug"))
    {
#ifdef DEBUG
      if (DO_DEBUG == 0)
        DO_DEBUG = 1;
#endif
      continue;
    }

#if defined(DAEMON) && !defined(_DSPAMC_H)

    if (!strcmp (argv[i], "--client")) {
      ATX->client_mode = 1;
      continue;
    }

#ifdef TRUSTED_USER_SECURITY
    if (!strcmp (argv[i], "--daemon") && ATX->trusted) 
#else
    if (!strcmp (argv[i], "--daemon")) 
#endif
    {
      ATX->operating_mode = DSM_DAEMON;
      continue;
    }
#endif
 
    if (!strncmp (argv[i], "--mode=", 7))
    {
      char *mode = strchr(argv[i], '=')+1;
      process_mode(ATX, mode);
      continue;
    }

    /* Build RCPT TO list */

    if (flag_r)
    {
      if (argv[i] != NULL && strlen (argv[i]) < MAX_USERNAME_LENGTH)
      {
        char user[MAX_USERNAME_LENGTH];

        if (_ds_match_attribute(agent_config, "Broken", "case"))
          lc(user, argv[i]);
        else
          strcpy(user, argv[i]);

#ifdef TRUSTED_USER_SECURITY
        if (!ATX->trusted && strcmp(user, __pw_name)) {
          LOG(LOG_ERR, ERR_TRUSTED_USER, __pw_uid, __pw_name);
          return EINVAL;
        }

        if (ATX->trusted)
#endif
          nt_add (ATX->recipients, user);
      }
      continue;
    }

    /* Build process user list */

    if (flag_u)
    {
      if (argv[i] != NULL && strlen (argv[i]) < MAX_USERNAME_LENGTH)
      {
        char user[MAX_USERNAME_LENGTH];

        if (_ds_match_attribute(agent_config, "Broken", "case")) 
          lc(user, argv[i]);
        else 
          strcpy(user, argv[i]);

#ifdef TRUSTED_USER_SECURITY
        if (!ATX->trusted && strcmp(user, __pw_name)) {
          LOG(LOG_ERR, ERR_TRUSTED_USER, __pw_uid, __pw_name);
          return EINVAL;
        }

        if (ATX->trusted)
#endif
          nt_add (ATX->users, user);
      }
      continue;
    }

    if (!strncmp (argv[i], "--mail-from=", 12))
    {
      strlcpy(ATX->mailfrom, strchr(argv[i], '=')+1, sizeof(ATX->mailfrom));
      LOGDEBUG("MAIL FROM: %s", ATX->mailfrom);
      continue;
    }  

    if (!strncmp (argv[i], "--profile=", 10))
    {
#ifdef TRUSTED_USER_SECURITY
      if (!ATX->trusted) {
        LOG(LOG_ERR, ERR_TRUSTED_PRIV, "--profile", 
            __pw_uid, __pw_name);
        return EINVAL;
      }
#endif
      if (!_ds_match_attribute(agent_config, "Profile", argv[i]+10)) {
        LOG(LOG_ERR,ERR_AGENT_NO_SUCH_PROFILE, argv[i]+10);
        return EINVAL;
      } else {
        _ds_overwrite_attribute(agent_config, "DefaultProfile", argv[i]+10);
      }
      continue;
    }

    if (!strncmp (argv[i], "--signature=", 12)) 
    {
      strlcpy(ATX->signature, strchr(argv[i], '=')+1, sizeof(ATX->signature));
      continue;
    }

    if (!strncmp (argv[i], "--class=", 8))
    {
      char *ptr = strchr(argv[i], '=')+1;
      char *spam = _ds_read_attribute(agent_config, "ClassAliasSpam");
      char *nonspam = _ds_read_attribute(agent_config, "ClassAliasNonspam");
      if (!strcmp(ptr, "spam") || (spam && !strcmp(ptr, spam)))
      {
        ATX->classification = DSR_ISSPAM;
      } else if (!strcmp(ptr, "innocent") || !strcmp(ptr, "nonspam") ||
                 (nonspam && !strcmp(ptr, nonspam)))
      {
        ATX->classification = DSR_ISINNOCENT;
      }
      else
      {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_CLASS, ptr);
        return EINVAL;
      }
      continue;
    }

    if (!strncmp (argv[i], "--source=", 9))
    {
      char *ptr = strchr(argv[i], '=')+1;

      if (!strcmp(ptr, "corpus"))
        ATX->source = DSS_CORPUS;
      else if (!strcmp(ptr, "inoculation"))
        ATX->source = DSS_INOCULATION;
      else if (!strcmp(ptr, "error"))
        ATX->source = DSS_ERROR;
      else
      {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_SOURCE, ptr);
        return EINVAL;
      }
      continue;
    }

    if (!strcmp (argv[i], "--classify"))
    {
      ATX->operating_mode = DSM_CLASSIFY;
      ATX->training_mode = DST_NOTRAIN;
      continue;
    }

    if (!strcmp (argv[i], "--process"))
    {
      ATX->operating_mode = DSM_PROCESS;
      continue;
    }

    if (!strncmp (argv[i], "--deliver=", 10))
    {
      char *dup = strdup(strchr(argv[i], '=')+1);
      char *ptr;
      if (dup == NULL) {
        LOG(LOG_CRIT, ERR_MEM_ALLOC);
        return EUNKNOWN;
      }

      ptr = strtok_r(dup, ",", &ptrptr);
      while(ptr != NULL) {
        if (!strcmp(ptr, "stdout")) {
          ATX->flags |= DAF_DELIVER_SPAM;
          ATX->flags |= DAF_DELIVER_INNOCENT;
          ATX->flags |= DAF_STDOUT; 
        }
        else if (!strcmp(ptr, "spam")) 
          ATX->flags |= DAF_DELIVER_SPAM;
        else if (!strcmp(ptr, "innocent") || !strcmp(ptr, "nonspam"))
          ATX->flags |= DAF_DELIVER_INNOCENT;
        else if (!strcmp(ptr, "summary"))
          ATX->flags |= DAF_SUMMARY;
        else
        {
          LOG(LOG_ERR, ERR_AGENT_NO_SUCH_DELIVER, ptr);
          free(dup);
          return EINVAL;
        }
      
        ptr = strtok_r(NULL, ",", &ptrptr);
      }
      free(dup);
      continue;
    }

    if (!strncmp (argv[i], "--feature=", 10))
    {
      ATX->feature = 1;
      process_features(ATX, strchr(argv[i], '=')+1);
      continue;
    }

    if (!strcmp (argv[i], "--stdout"))
    {
      ATX->flags |= DAF_STDOUT;
      continue;
    }

    if (!strcmp (argv[i], "--help"))
    {
      fprintf (stderr, "%s\n", SYNTAX);
      exit(EXIT_SUCCESS);
    }

    if (!strcmp (argv[i], "--version"))
    {
      printf ("\nDSPAM Anti-Spam Suite %s (agent/library)\n\n", VERSION);
      printf ("Copyright (c) 2002-2006 Jonathan A. Zdziarski\n");
      printf ("http://dspam.nuclearelephant.com\n\n");
      printf ("DSPAM may be copied only under the terms of the GNU "
              "General Public License,\n");
      printf ("a copy of which can be found with the DSPAM distribution "
              "kit.\n\n");
#ifdef TRUSTED_USER_SECURITY
      if (ATX->trusted) {
#endif
        printf("Configuration parameters: %s\n\n", CONFIGURE_ARGS);
#ifdef TRUSTED_USER_SECURITY
      }
#endif
      exit (EXIT_SUCCESS);
    }

    /* Append all unknown arguments as mailer args */

    if (i>0 
#ifdef TRUSTED_USER_SECURITY
        && ATX->trusted
#endif
    )
    {
      if (argv[i][0] == 0)
        strlcat (ATX->mailer_args, "\"\"", sizeof (ATX->mailer_args));
      else
        strlcat (ATX->mailer_args, argv[i], sizeof (ATX->mailer_args));
      strlcat (ATX->mailer_args, " ", sizeof (ATX->mailer_args));
    }
  }
 
  return 0;
}


/* 
 * process_features(AGENT_CTX *, const char *)
 *
 * DESCRIPTION
 *   convert --feature= stdin into agent context values
 *
 * INPUT ARGUMENTS
 *	ATX	agent context
 *	in	remainder of --feature= stdin
 *
 * RETURN VALUES
 *   returns 0 on success, EINVAL when invalid options specified
 *    
 */

int process_features(AGENT_CTX *ATX, const char *in) {
  char *ptr, *dup, *ptrptr;
  int ret = 0;

  if (!in || in[0]==0)
    return 0;

  dup = strdup(in);
  if (dup == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  ptr = strtok_r(dup, ",", &ptrptr);
  while(ptr != NULL) {
    if (!strncmp(ptr, "no",2))
      ATX->flags |= DAF_NOISE;
    else if (!strncmp(ptr, "wh", 2))
      ATX->flags |= DAF_WHITELIST;
    else if (!strncmp(ptr, "tb=", 3)) {
      ATX->training_buffer = atoi(strchr(ptr, '=')+1);

      if (ATX->training_buffer < 0 || ATX->training_buffer > 10) {
        LOG(LOG_ERR, ERR_AGENT_TB_INVALID);
        ret = EINVAL;
      }
    }
    else {
      LOG(LOG_ERR, ERR_AGENT_NO_SUCH_FEATURE, ptr);
      ret = EINVAL;
    }

    ptr = strtok_r(NULL, ",", &ptrptr);
  }
  free(dup);
  return ret;
}

/*
 * process_mode(AGENT_CTX *, const char *)
 *
 * DESCRIPTION
 *   convert --mode= stdin into training mode
 *
 * INPUT ARGUMENTS
 *	ATX	agent context
 *	mode	remainder of --mode= stdin
 *
 * RETURN VALUES
 *   returns 0 on success, EINVAL when invalid mode specified
 */

int process_mode(AGENT_CTX *ATX, const char *mode) {

  if (!mode)
    return EINVAL;

  if (!strcmp(mode, "toe"))
    ATX->training_mode = DST_TOE;
  else if (!strcmp(mode, "teft"))
    ATX->training_mode = DST_TEFT;
  else if (!strcmp(mode, "tum"))
    ATX->training_mode = DST_TUM;
  else if (!strcmp(mode, "notrain"))
    ATX->training_mode = DST_NOTRAIN;
  else if (!strcmp(mode, "unlearn")) {
    ATX->training_mode = DST_TEFT;
    ATX->flags |= DAF_UNLEARN;
  } else {
    LOG(LOG_ERR, ERR_AGENT_TR_MODE_INVALID, mode);
    return EINVAL;
  }

  return 0;
}

/*
 * apply_defaults(AGENT_CTX *)
 *
 * DESCRIPTION
 *   apply default values from dspam.conf in absence of other options
 *
 * INPUT ARGUMENTS
 *      ATX     agent context
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int apply_defaults(AGENT_CTX *ATX) {

  /* Training mode */

  if (ATX->training_mode == DST_DEFAULT) {
    char *v = _ds_read_attribute(agent_config, "TrainingMode");
    process_mode(ATX, v);
  }

  /* Default delivery agent */

  if ( ! (ATX->flags & DAF_STDOUT) 
    && ATX->operating_mode != DSM_CLASSIFY
    && (ATX->flags & DAF_DELIVER_INNOCENT || ATX->flags & DAF_DELIVER_SPAM)) 
  {
    char key[32];
#ifdef TRUSTED_USER_SECURITY
    if (!ATX->trusted) 
      strcpy(key, "UntrustedDeliveryAgent");
    else
#endif
      strcpy(key, "TrustedDeliveryAgent");

    if (_ds_read_attribute(agent_config, key)) {
      char fmt[sizeof(ATX->mailer_args)];
      snprintf(fmt, sizeof(fmt), "%s ", _ds_read_attribute(agent_config, key));
#ifdef TRUSTED_USER_SECURITY
      if (ATX->trusted)
#endif
        strlcat(fmt, ATX->mailer_args, sizeof(fmt));
      strcpy(ATX->mailer_args, fmt);
    } else if (!_ds_read_attribute(agent_config, "DeliveryHost")) {
      LOG(LOG_ERR, ERR_AGENT_NO_AGENT, key);
      return EINVAL;
    }
  }

  /* Default quarantine agent */

  if (_ds_read_attribute(agent_config, "QuarantineAgent")) {
    snprintf(ATX->spam_args, sizeof(ATX->spam_args), "%s ",
             _ds_read_attribute(agent_config, "QuarantineAgent"));
  } else {
    LOGDEBUG("No QuarantineAgent option found. Using standard quarantine.");
  }

  /* Features */

  if (!ATX->feature && _ds_find_attribute(agent_config, "Feature")) {
    attribute_t attrib = _ds_find_attribute(agent_config, "Feature");

    while(attrib != NULL) { 
      process_features(ATX, attrib->value);
      attrib = attrib->next;
    }
  }

  return 0;
}

/*
 * check_configuration(AGENT_CTX *)
 *
 * DESCRIPTION
 *   sanity-check agent configuration
 *
 * INPUT ARGUMENTS
 *      ATX     agent context
 *
 * RETURN VALUES
 *   returns 0 on success, EINVAL on invalid configuration
*/

int check_configuration(AGENT_CTX *ATX) {

  if (ATX->classification != DSR_NONE && ATX->operating_mode == DSM_CLASSIFY)
  {
    LOG(LOG_ERR, ERR_AGENT_CLASSIFY_CLASS);
    return EINVAL;
  }

  if (ATX->classification != DSR_NONE && ATX->source == DSS_NONE && 
     !(ATX->flags & DAF_UNLEARN))
  {
    LOG(LOG_ERR, ERR_AGENT_NO_SOURCE);
    return EINVAL;
  }

  if (ATX->source != DSS_NONE && ATX->classification == DSR_NONE)
  {
    LOG(LOG_ERR, ERR_AGENT_NO_CLASS);
    return EINVAL;
  }

  if (ATX->operating_mode == DSM_NONE)
  {
    LOG(LOG_ERR, ERR_AGENT_NO_OP_MODE);
    return EINVAL;
  }

  if (ATX->training_mode == DST_DEFAULT)
  {
    LOG(LOG_ERR, ERR_AGENT_NO_TR_MODE);
    return EINVAL;
  }

  if (!_ds_match_attribute(agent_config, "ParseToHeaders", "on")) {

    if (ATX->users->items == 0)
    {
      LOG(LOG_ERR, ERR_AGENT_USER_UNDEFINED);
      return EINVAL;
    }
  }

  return 0;
}

/*
 * read_stdin(AGENT_CTX *)
 *
 * DESCRIPTION
 *   read message from stdin and perform any inline configuration
 *   (such as servicing 'ParseToHeaders' functions)
 *
 * INPUT ARGUMENTS
 *      ATX     agent context
 *
 * RETURN VALUES
 *   buffer structure containing the message
 */

buffer * read_stdin(AGENT_CTX *ATX) {
  int body = 0, line = 1;
  char buf[1024];
  buffer *msg;

  msg = buffer_create(NULL);
  if (msg == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  if (_ds_match_attribute(agent_config, "DataSource", "document")) {
    buffer_cat(msg, ": \n\n");
    body = 1;
  }

  /* Only read the message if no signature was provided on commandline */

  if (ATX->signature[0] == 0) {
    while ((fgets (buf, sizeof (buf), stdin)) != NULL)
    {
      /* Strip CR/LFs for admittedly broken mail servers */

      if (_ds_match_attribute(agent_config, "Broken", "lineStripping")) {
        size_t len = strlen(buf);
        while (len>1 && buf[len-2]==13) {
          buf[len-2] = buf[len-1];
          buf[len-1] = 0;
          len--;
        }
      }

      /* Quote message termination characters, that could truncate messages */
      if (buf[0] == '.' && buf[1] < 32) {
        char x[sizeof(buf)];
        snprintf(x, sizeof(x), ".%s", buf);
        strcpy(buf, x);
      }
  
      /*
       *  Don't include first line of message if it's a quarantine header added
       *  by dspam at time of quarantine 
       */

      if (line==1 && !strncmp(buf, "From QUARANTINE", 15))
        continue;

      /*
       *  Parse the "To" headers and adjust the operating mode and user when
       *  an email is sent to spam-* or notspam-* address. Behavior must be
       *  configured in dspam.conf
       */

      if (_ds_match_attribute(agent_config, "ParseToHeaders", "on")) {
        if (buf[0] == 0)
          body = 1;

        if (!body && !strncasecmp(buf, "To: ", 4))
          process_parseto(ATX, buf);
      }

      if (buffer_cat (msg, buf))
      {
        LOG (LOG_CRIT, ERR_MEM_ALLOC);
        goto bail;
      }
  
      /*
       *  Use the original user id if we are reversing a false positive
       *  (this is only necessary when using shared,managed groups 
       */

      if (!strncasecmp (buf, "X-DSPAM-User: ", 14) && 
          ATX->operating_mode == DSM_PROCESS    &&
          ATX->classification == DSR_ISINNOCENT &&
          ATX->source         == DSS_ERROR)
      {
        char user[MAX_USERNAME_LENGTH];
        strlcpy (user, buf + 14, sizeof (user));
        chomp (user);
        nt_destroy (ATX->users);
        ATX->users = nt_create (NT_CHAR);
        if (ATX->users == NULL) {
          LOG(LOG_CRIT, ERR_MEM_ALLOC);
          goto bail;
        }
        LOGDEBUG("found username %s in X-DSPAM-User header", user);
        nt_add (ATX->users, user);
      }
  
      line++;
    }
  }

  if (!msg->used)
  {
    if (ATX->signature[0] != 0) {
      buffer_cat(msg, "\n\n");
    }
    else { 
      LOG (LOG_INFO, "empty message (no data received)");
      goto bail;
    }
  }

  return msg;

bail:
  LOGDEBUG("read_stdin() failure");
  buffer_destroy(msg);
  return NULL;
}

/*
 * process_parseto(AGENT_CTX *, const char *)
 *
 * DESCRIPTION
 *   processes the To: line of a message to provide parseto services
 *
 * INPUT ARGUMENTS
 *      ATX     agent context
 *      buf	To: line
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int process_parseto(AGENT_CTX *ATX, const char *buf) {
  char *y = NULL;
  char *x;

  if (!buf) 
    return EINVAL;

  x = strstr(buf, "<spam-");
  if (!x)
    x = strstr(buf, " spam-");
  if (!x)
    x = strstr(buf, ":spam-");
  if (!x)
    x = strstr(buf, "<spam@");
  if (!x)
    x = strstr(buf, " spam@");
  if (!x)
    x = strstr(buf, ":spam@");

  if (x != NULL) {
    y = strdup(x+6);

    if (_ds_match_attribute(agent_config, "ChangeModeOnParse", "on"))
    {
      ATX->classification = DSR_ISSPAM;
      ATX->source = DSS_ERROR;
    }
  } else {

    x = strstr(buf, "<notspam-");
    if (!x)
      x = strstr(buf, " notspam-");
    if (!x)
      x = strstr(buf, ":notspam-");
    if (!x)
      x = strstr(buf, "<notspam@");
    if (!x)
      x = strstr(buf, " notspam@");
    if (!x)
      x = strstr(buf, ":notspam@");

    if (x && strlen(x) >= 9) {
      y = strdup(x+9);

      if (_ds_match_attribute(agent_config, "ChangeModeOnParse", "on"))
      {
        ATX->classification = DSR_ISINNOCENT;
        ATX->source = DSS_ERROR;
      }
    }
  }

  if (y && (_ds_match_attribute(agent_config,
                                "ChangeUserOnParse", "on") ||
            _ds_match_attribute(agent_config,
                               "ChangeUserOnParse", "full") ||
            _ds_match_attribute(agent_config,
                                "ChangeUserOnParse", "user"))) 
  {
    char *ptrptr;
    char *z;

    if (_ds_match_attribute(agent_config, 
                            "ChangeUserOnParse", "full"))
    {
      z = strtok_r(y, "> \n", &ptrptr);
    } else {
      if (!strstr(x, "spam@"))
        z = strtok_r(y, "@", &ptrptr);
      else
        z = NULL;
    }

    if (z) {
      nt_destroy(ATX->users);
      ATX->users = nt_create(NT_CHAR);
      if (!ATX->users) {
        LOG(LOG_CRIT, ERR_MEM_ALLOC);
        return EUNKNOWN;
      }
      nt_add (ATX->users, z);
    }
    free(y);
  }

  return 0;
}
