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
 * libdspam.c - DSPAM core analytical engine
 *
 * DESCRIPTION
 *   libdspam is at the core of the decision making process and is called
 *   by the agent to perform all tasks related to message classification.
 *   The libdspam API functions are documented in libdspam(1).
 */

#ifndef STATIC_DRIVER
void *_drv_handle;
#endif

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>

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

#include "config.h"
#include "libdspam_objects.h"
#include "libdspam.h"
#include "nodetree.h"
#include "config.h"
#include "base64.h"
#include "bnr.h"
#include "util.h"
#include "storage_driver.h"
#include "buffer.h"
#include "heap.h"
#include "error.h"
#include "decode.h"
#include "language.h"
#ifdef NCORE
#include <ncore/ncore.h>
#include <ncore/types.h>
#include "ncore_adp.h"
#endif

#define CHI_S   0.1     /* Chi-Sq Strength */
#define CHI_X   0.5000  /* Chi-Sq Assumed Probability */

#define	C1	16	/* Markov C1 */
#define C2	1	/* Markov C2 */

#ifdef DEBUG
int DO_DEBUG = 0;
#endif

#ifdef NCORE
nc_dev_t g_ncDevice;

NC_STREAM_CTX	g_ncDelimiters;
#endif

/*
 * dspam_init()
 *
 * DESCRIPTION
 *   The  dspam_init() function creates and initializes a new classification
 *   context and attaches the context to whatever backend  storage  facility
 *   was  configured. The user and group arguments provided are used to read
 *   and write information stored for the user and group specified. The home
 *   argument is used to configure libdspam's storage around the base direc-
 *   tory specified. The mode specifies the operating mode to initialize the
 *   classification context with and may be one of:
 *
 *    DSM_PROCESS   Process the message and return a result
 *    DSM_CLASSIFY  Classify message only, no learning
 *    DSM_TOOLS     No processing, attach to storage only
 * 
 *   The  flags  provided further tune the classification context for a spe-
 *   cific function. Multiple flags may be OR'd together.
 * 
 *    DSF_SIGNATURE A binary signature is requested/provided
 *    DSF_NOISE     Apply Bayesian Noise Reduction logic
 *    DSF_WHITELIST Use automatic whitelisting logic
 *    DSF_MERGED    Merge group metadata with user's in memory
 * 
 * RETURN VALUES
 *   Upon successful completion, dspam_init() will return a pointer to a new
 *   classification context structure containing a copy of the configuration
 *   passed into dspam_init(), a connected storage driver handle, and a  set
 *   of preliminary user control data read from storage.
 */

DSPAM_CTX * dspam_init (
  const char *username,
  const char *group,
  const char *home,
  int operating_mode,
  u_int32_t flags)
{
  DSPAM_CTX *CTX = dspam_create(username, group, home, operating_mode, flags);

  if (CTX == NULL)
    return NULL;

  if (!dspam_attach(CTX, NULL))
    return CTX;

  dspam_destroy(CTX);

  return NULL;
}

/* dspam_create()
 *
 * DESCRIPTION
 *   The  dspam_create() function performs in exactly the same manner as the
 *   dspam_init() function, but does not attach  to  storage.  Instead,  the
 *   caller  must  also  call dspam_attach() after setting any storage- spe-
 *   cific attributes using dspam_addattribute(). This is useful  for  cases
 *   where  the  implementor  would  prefer  to configure storage internally
 *   rather than having libdspam read a configuration from a file.
 *
 * RETURN VALUES
 *   Upon successful completion, dspam_create() will return a pointer to a new
 *   classification context structure containing a copy of the configuration
 *   passed into dspam_create(). At this point, dspam_attach() must be called
 *   for further processing.
 */

DSPAM_CTX * dspam_create (
  const char *username,
  const char *group,
  const char *home,
  int operating_mode,
  u_int32_t flags)
{
  DSPAM_CTX *CTX;

  CTX = calloc (1, sizeof (DSPAM_CTX));
  if (CTX == NULL)
    return NULL;

  CTX->config = calloc(1, sizeof(struct _ds_config));
  if (CTX->config == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    goto bail;
  }

  CTX->config->size = 128;
  CTX->config->attributes = calloc(1, sizeof(attribute_t)*128);
  if (CTX->config->attributes == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    goto bail;
  }

  if (home != NULL && home[0] != 0)
    CTX->home = strdup (home);
  else {
#ifdef DSPAM_HOME
    CTX->home = strdup(DSPAM_HOME);
#else
    CTX->home = NULL;
#endif
  }

  if (username != NULL && username[0] != 0)
    CTX->username = strdup (username);
  else
    CTX->username = NULL;

  if (group != NULL && group[0] != 0)
    CTX->group = strdup (group);
  else
    CTX->group = NULL;

  CTX->probability     = DSP_UNCALCULATED;
  CTX->operating_mode  = operating_mode;
  CTX->flags           = flags;
  CTX->message         = NULL;
  CTX->confidence      = 0;
  CTX->training_mode   = DST_TEFT;
  CTX->wh_threshold    = 10;
  CTX->training_buffer = 0;
  CTX->classification  = DSR_NONE;
  CTX->source          = DSS_NONE;
  CTX->_sig_provided   = 0;
  CTX->factors         = NULL;
  CTX->algorithms      = 0;
  CTX->tokenizer       = DSZ_WORD;

  return CTX;

bail:
  if (CTX->config)
    _ds_destroy_config(CTX->config->attributes);
  free(CTX->config);
  free(CTX->username);
  free(CTX->group);
  free(CTX);
  return NULL;
}

/*
 * dspam_clearattributes()
 *
 * DESCRIPTION
 *  The dspam_clearattributes() function is called to clear any attributes
 *  previously set using dspam_addattribute()  within  the  classification
 *  context.  It is necessary to call this function prior to replacing any
 *  attributes already written.
 *
 * RETURN VALUES
 *  returns 0 on success, standard errors on failure
 *
 */ 

int dspam_clearattributes (DSPAM_CTX * CTX) {

  if (CTX->config) {
    _ds_destroy_config(CTX->config->attributes);
    free(CTX->config);
  } else {
    return EFAILURE;
  }

  CTX->config = calloc(1, sizeof(struct _ds_config));
  if (CTX->config == NULL)
    goto bail;
  CTX->config->size = 128;
  CTX->config->attributes = calloc(1, sizeof(attribute_t)*128);
  if (CTX->config->attributes == NULL)
    goto bail;

  return 0;

bail:
  free(CTX->config);
  CTX->config = NULL;
  LOG(LOG_CRIT, ERR_MEM_ALLOC);
  return EUNKNOWN;
}

/*
 * dspam_addattribute()
 *
 * DESCRIPTION
 *   The dspam_addattribute() function is called to  set  attributes  within
 *   the  classification  context.  Some  storage drivers support the use of
 *   passing specific attributes such as  server  connect  information.  The
 *   driver-independent attributes supported by DSPAM include:
 * 
 *    IgnoreHeader   Specify a specific header to ignore
 *    LocalMX        Specify a local mail exchanger to assist in
 *                   correct results from dspam_getsource().
 *
 *   Only  driver-dependent  attributes  need  be  set  prior  to  a call to
 *   dspam_attach(). Driver-independent attributes may be  set  both  before
 *   and after storage has been attached.
 *
 * RETURN VALUES
 *   returns 0 on success, standard errors on failure
 */                                                                                 
int dspam_addattribute (DSPAM_CTX * CTX, const char *key, const char *value) {
  int i, j = 0;
                                                                                
  if (_ds_find_attribute(CTX->config->attributes, key))
    return _ds_add_attribute(CTX->config->attributes, key, value);
                                                                                
  for(i=0;CTX->config->attributes[i];i++)
    j++;
                                                                                
  if (j >= CTX->config->size) {
    config_t ptr;
    CTX->config->size *= 2;
    ptr = realloc(CTX->config->attributes,
                  1+(sizeof(attribute_t)*CTX->config->size));
    if (ptr) {
      CTX->config->attributes = ptr;
    } else {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      return EFAILURE; 
    } 
  }
                                                                                
  return _ds_add_attribute(CTX->config->attributes, key, value);
}

/*
 * dspam_attach()
 *
 * DESCRIPTION
 *   The dspam_attach() function attaches the storage interface to the clas-
 *   sification context and alternatively established an initial  connection
 *   with  storage  if dbh is NULL. Some storage drivers support only a NULL
 *   value  for  dbh,  while  others  (such  as  mysql_drv,  pgsql_drv,  and
 *   sqlite_drv) allow an open database handle to be attached. This function
 *   should only be called after  an  initial  call  to  dspam_create()  and
 *   should  never  be called if using dspam_init(), as storage is automati-
 *   cally attached by a call to dspam_init().
 *
 * RETURN VALUES
 *   returns 0 on success, standard errors on failure
 */

int dspam_attach (DSPAM_CTX *CTX, void *dbh) {
  if (!_ds_init_storage (CTX, dbh))
    return 0;
                                                                                
  return EFAILURE;
}

/*
 * dspam_detach()
 *
 * DESCRIPTION
 *     The dspam_detach() function can be called when a detachment from  stor-
 *     age  is desired, but the context is still needed. The storage driver is
 *     closed, leaving the classification context in place. Once  the  context
 *     is no longer needed, another call to dspam_destroy() should be made. If
 *     you are closing storage and destroying the context at the same time, it
 *     is   not  necessary  to  call  this  function.  Instead  you  may  call
 *     dspam_destroy() directly.
 *
 * RETURN VALUES
 *   returns 0 on success, standard errors on failure
 */

int
dspam_detach (DSPAM_CTX * CTX)
{
  if (CTX->storage != NULL) {
                                                                                
    /* Sanity check totals before our shutdown call writes them */

    if (CTX->totals.spam_learned < 0)
      CTX->totals.spam_learned = 0;
    if (CTX->totals.innocent_learned < 0)
      CTX->totals.innocent_learned = 0;
    if (CTX->totals.spam_misclassified < 0)
      CTX->totals.spam_misclassified = 0;
    if (CTX->totals.innocent_misclassified < 0)
      CTX->totals.innocent_misclassified = 0;
    if (CTX->totals.spam_classified < 0)
      CTX->totals.spam_classified = 0;
    if (CTX->totals.innocent_classified < 0)
      CTX->totals.innocent_classified = 0;

    _ds_shutdown_storage (CTX);
    free(CTX->storage);
    CTX->storage = NULL;
  }

  return 0;
}

/*
 * dspam_destroy()
 * 
 *     The dspam_destroy() function should be called when the  context  is  no
 *     longer  needed.  If a connection was established to storage internally,
 *     the connection is closed and all data is flushed and written. If a han-
 *     dle was attached, the handle will remain open.
 */

void
dspam_destroy (DSPAM_CTX * CTX)
{
  if (CTX->storage != NULL)
    dspam_detach(CTX);

  _ds_factor_destroy(CTX->factors);
  if (CTX->config && CTX->config->attributes)
    _ds_destroy_config (CTX->config->attributes);

  free (CTX->config);
  free (CTX->username);
  free (CTX->group);
  free (CTX->home);

  if (! CTX->_sig_provided && CTX->signature != NULL)
  {
    free (CTX->signature->data);
    free (CTX->signature);
  }

  if (CTX->message)
    _ds_destroy_message(CTX->message);
  free (CTX);
  return;
}

/*
 * dspam_process()
 *
 * DESCRIPTION
 *   The dspam_process() function performs analysis of  the  message  passed
 *   into  it  and will return zero on successful completion. If successful,
 *   CTX->result will be set to one of three classification results:
 * 
 *    DSR_ISSPAM        Message was classified as spam
 *    DSR_ISINNOCENT    Message was classified as nonspam
 * 
 * RETURN VALUES
 *   returns 0 on success
 *
 *   EINVAL    An invalid call or invalid parameter used.
 *   EUNKNOWN  Unexpected error, such as malloc() failure
 *   EFILE     Error opening or writing to a file or file handle
 *   ELOCK     Locking failure
 *   EFAILURE  The operation itself has failed
 */

int
dspam_process (DSPAM_CTX * CTX, const char *message)
{
#ifdef DEBUG
  struct timeval tp1, tp2;
  struct timezone tzp;
#endif
  buffer *header, *body;
  int spam_result = 0, is_toe = 0, is_undertrain = 0;

#ifdef DEBUG
  if (DO_DEBUG)
    gettimeofday(&tp1, &tzp);
#endif

  if (CTX->signature != NULL)
    CTX->_sig_provided = 1;

  /* Sanity check context behavior */

  if (CTX->operating_mode == DSM_CLASSIFY && CTX->classification != DSR_NONE)
  {
    LOG(LOG_WARNING, "DSM_CLASSIFY can't be used with a classification");
    return EINVAL;
  }

  if (CTX->algorithms == 0) 
  {
    LOG(LOG_WARNING, "No algorithms configured. Use CTX->algorithms and DSA_");
    return EINVAL;
  }

  if (CTX->classification != DSR_NONE && CTX->source == DSR_NONE) 
  {
    LOG(LOG_WARNING, "A classification requires a source be specified");
    return EINVAL;
  }

  if (CTX->classification == DSR_NONE && CTX->source != DSR_NONE)
  {
    LOG(LOG_WARNING, "A source requires a classification be specified");
    return EINVAL;
  }
 
  /* Set TOE mode pretrain option if we haven't seen many messages yet */
  if (CTX->training_mode == DST_TOE
  && (CTX->totals.innocent_learned <= 100 || CTX->totals.spam_learned <= 100)
  && (!(CTX->algorithms & DSP_MARKOV)))
  {
    is_undertrain = 1;
    CTX->training_mode = DST_TEFT;
  }

  /* Classify only for TOE / NOTRAIN mode setting if data is mature enough */
  if ( CTX->operating_mode == DSM_PROCESS 
    && CTX->classification == DSR_NONE 
    && (CTX->training_mode == DST_TOE || CTX->training_mode == DST_NOTRAIN))
  {
    CTX->operating_mode = DSM_CLASSIFY;
    is_toe = 1;
  }

  /* A signature has been presented for training; process it */
  /* Non-SPBH Signature */
  if (CTX->operating_mode == DSM_PROCESS 
   && CTX->classification != DSR_NONE 
   && CTX->flags & DSF_SIGNATURE 
   && (CTX->tokenizer != DSZ_SBPH))
  {
    int i = _ds_process_signature (CTX);
    if (is_toe)
      CTX->operating_mode = DSM_PROCESS;
    if (is_undertrain)
      CTX->training_mode = DST_TOE;
    return i;
  }

  header = buffer_create (NULL);
  body   = buffer_create (NULL);
  if (header == NULL || body == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    buffer_destroy (header);
    buffer_destroy (body);
    if (is_toe)
      CTX->operating_mode = DSM_PROCESS;
    if (is_undertrain)
      CTX->training_mode = DST_TOE;
    return EUNKNOWN;
  }

  /* Parse the message if it hasn't already been by the client app */
  if (!CTX->message && message)
    CTX->message = _ds_actualize_message (message);

  /* Analyze and filter (unless it's a signature based classification) */
  if (! (CTX->flags & DSF_SIGNATURE
     && CTX->operating_mode == DSM_CLASSIFY
      && CTX->signature != NULL))
  {
    _ds_degenerate_message(CTX, header, body);
  }

  /*** Perform statistical operations and get a classification result ***/

  /* Initialize */
  CTX->result = DSR_NONE;

  /* If SBPH reclassification, recall and operate on saved SBPH text */

  if ( CTX->tokenizer == DSZ_SBPH
    && CTX->operating_mode != DSM_CLASSIFY 
    && CTX->classification != DSR_NONE 
    && CTX->flags & DSF_SIGNATURE)
  {
    char *y, *h, *b;
    char *ptrptr;

    y = strdup((const char *) CTX->signature->data);
    h = strtok_r(y, "\001", &ptrptr);
    b = strtok_r(NULL, "\001", &ptrptr);
    spam_result = _ds_operate (CTX, h, b);

  /* Otherwise, operate on the input message */
 
  } else {
    spam_result = _ds_operate (CTX, header->data, body->data);
  }

  /* Force decision if a classification was specified */

  if (CTX->classification != DSR_NONE && spam_result >= 0) 
  {
    if (CTX->classification == DSR_ISINNOCENT)
      spam_result = DSR_ISINNOCENT;
    else if (CTX->classification == DSR_ISSPAM)
      spam_result = DSR_ISSPAM;
  }

  /* Apply results to context and clean up */
  CTX->result = spam_result;
  if (CTX->class[0] == 0) {
    if (CTX->result == DSR_ISSPAM)
      strcpy(CTX->class, LANG_CLASS_SPAM);
    else if (CTX->result == DSR_ISINNOCENT)
      strcpy(CTX->class, LANG_CLASS_INNOCENT);
  }
  buffer_destroy (header);
  buffer_destroy (body);

  if (is_toe)
    CTX->operating_mode = DSM_PROCESS;
  if (is_undertrain)
    CTX->training_mode = DST_TOE;

#ifdef DEBUG
  if (DO_DEBUG) {
    if (CTX->source == DSS_NONE) {
      gettimeofday(&tp2, &tzp);
      LOGDEBUG("total processing time: %01.5fs",
         (double) (tp2.tv_sec + (tp2.tv_usec / 1000000.0)) -
         (double) (tp1.tv_sec + (tp1.tv_usec / 1000000.0)));
    }
  }
#endif

  if (CTX->result == DSR_ISSPAM || CTX->result == DSR_ISINNOCENT) 
    return 0;
  else
  {
    LOG(LOG_WARNING, "received invalid result (! DSR_ISSPAM || DSR_INNOCENT) "
                     ": %d", CTX->result);
    return EUNKNOWN;
  }
}

/*
 * dspam_getsource()
 *
 * DESCRIPTION
 *
 *   The dspam_getsource() function extracts the source sender from the mes-
 *   sage  passed  in  during  a call to dspam_process() and writes not more
 *   than size bytes to buf.
 *
 * RETURN VALUES
 *   returns 0 on success, standard errors on failure
 */

int
dspam_getsource (
  DSPAM_CTX * CTX,
  char *buf,
  size_t size)
{
  ds_message_part_t current_block;
  ds_header_t current_heading = NULL;
  struct nt_node *node_nt;
  struct nt_c c;
  char qmailmode = 0;

  if (CTX->message == NULL)
    return EINVAL;

  node_nt = c_nt_first (CTX->message->components, &c);
  if (node_nt == NULL)
    return EINVAL;

  current_block = (ds_message_part_t) node_nt->ptr;

  node_nt = c_nt_first (current_block->headers, &c);
  while (node_nt != NULL)
  {
    current_heading = (ds_header_t) node_nt->ptr;
    if (!strcmp (current_heading->heading, "Received"))
    {
      char *data, *ptr, *tok;

      // detect and skip "Received: (qmail..." lines
      if (!strncmp(current_heading->data, "(qmail", 6))
      {
        qmailmode = 1;
        node_nt = c_nt_next (current_block->headers, &c);
        continue;
      }

      data = strdup (current_heading->data);
      ptr = strstr (data, "from");

      if (ptr != NULL)
      {
        if (strchr(data, '['))  // found a non-qmail header
        {
          qmailmode = 0;
        }

        // qmail puts the sending IP inside the last "()" pair of the line
        if (qmailmode)
        {
          tok = strrchr(data, ')');

          if (tok != NULL)
          {
              *tok = 0;
              tok = strrchr(data, '(');
              if (tok != NULL)
                tok++;
          }
        }
        else
        {
          char *ptrptr;
          tok = strtok_r (ptr, "[", &ptrptr);

          if (tok != NULL)
          {
            tok = strtok_r (NULL, "]", &ptrptr);
          }
        }
        if (tok != NULL)
        {
          int whitelisted = 0;
          if (!strncmp (tok, "127.",4) ||        // ignore localhost
              !strncmp (tok, "10.", 3) ||        // ignore RFC 1918 private addresses
              !strncmp (tok, "172.16.", 7) ||
              !strncmp (tok, "192.168.", 8) ||
              !strncmp (tok, "169.254.", 8))     // ignore local-link
            whitelisted = 1;

          if (_ds_match_attribute(CTX->config->attributes, "LocalMX", tok))
            whitelisted = 1;

          if (!whitelisted)
          {
            strlcpy (buf, tok, size);
            free (data);
            return 0;
          }
        }
      }
      free (data);
    }
    node_nt = c_nt_next (current_block->headers, &c);
  }

  return EFAILURE;
}

/*
 * _ds_operate() - operate on the message
 *
 * DESCRIPTION
 *    calculate the statistical probability the email is spam
 *    update tokens in dictionary according to result/mode
 *
 * INPUT ARGUMENTS
 *     DSPAM_CTX *CTX    pointer to context
 *     char *header      pointer to message header
 *     char *body        pointer to message body
 *
 * RETURN VALUES
 *   standard errors on failure
 *
 *     DSR_ISSPAM           message is spam
 *     DSR_ISINNOCENT       message is innocent
 */

int
_ds_operate (DSPAM_CTX * CTX, char *headers, char *body)
{
  int errcode = 0;
  int i;

  /* Create our diction (lexical data in message) and patterns */

  ds_diction_t diction = ds_diction_create(24593ul);
  ds_diction_t bnr_patterns = NULL;
  ds_term_t ds_term;
  ds_cursor_t ds_c;

  ds_heap_t heap_sort = NULL;    /* Heap sort for top N tokens */

#ifdef LIBBNR_DEBUG
  ds_heap_t heap_nobnr = NULL;
#endif

  unsigned long long whitelist_token = 0;
  int do_whitelist = 0;
  int result;

  if (CTX->algorithms & DSA_BURTON)
    heap_sort = ds_heap_create(BURTON_WINDOW_SIZE, HP_DELTA);
  else if (CTX->algorithms & DSA_ROBINSON)
    heap_sort = ds_heap_create(25, HP_DELTA);
  else
    heap_sort = ds_heap_create(15, HP_DELTA);

  /* Allocate SBPH signature (stored as message text) */

  if ( CTX->tokenizer == DSZ_SBPH
    && CTX->flags & DSF_SIGNATURE 
    && ( (  CTX->operating_mode != DSM_CLASSIFY 
         && CTX->classification == DSR_NONE)
       || ! (CTX->_sig_provided)) 
    && CTX->source != DSS_CORPUS)
  {
    CTX->signature = calloc (1, sizeof (struct _ds_spam_signature));
    if (CTX->signature == NULL)
    {
      LOG (LOG_CRIT, "memory allocation error");
      errcode = EUNKNOWN;
      goto bail;
    }
                                                                                
    CTX->signature->length = strlen(headers)+strlen(body)+2;
    CTX->signature->data = malloc(CTX->signature->length);

    if (CTX->signature->data == NULL)
    {
      LOG (LOG_CRIT, "memory allocation error");
      free (CTX->signature);
      CTX->signature = NULL;
      errcode = EUNKNOWN; 
      goto bail;
    }

    strcpy(CTX->signature->data, headers);
    strcat(CTX->signature->data, "\001");
    strcat(CTX->signature->data, body);
  }

  if (!diction)
  {
    ds_diction_destroy(diction);
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    errcode = EUNKNOWN;
    goto bail;
  }

#ifdef LIBBNR_DEBUG
  heap_nobnr = ds_heap_create (heap_sort->size, HP_DELTA);
  if (heap_nobnr == NULL) {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    errcode = EUNKNOWN;
    goto bail;
  }
#endif

  CTX->result = 
    (CTX->classification == DSR_ISSPAM) ? DSR_ISSPAM : DSR_ISINNOCENT;

  /* If we are classifying based on a signature, preprogram the tree */

  if (CTX->flags & DSF_SIGNATURE          &&
      CTX->operating_mode == DSM_CLASSIFY && 
      CTX->_sig_provided)
  {
    int num_tokens =
      CTX->signature->length / sizeof (struct _ds_signature_token);
    struct _ds_signature_token t;

    for (i = 0; i < num_tokens; i++)
    {
      char x[128];
      memcpy (&t,
              (char *) CTX->signature->data +
              (i * sizeof (struct _ds_signature_token)),
              sizeof (struct _ds_signature_token));
      snprintf (x, sizeof (x), "E: %" LLU_FMT_SPEC, t.token);
      ds_term = ds_diction_touch(diction, t.token, x, 0);
      if (ds_term)
        ds_term->frequency = t.frequency;
    }
  }

  /* Otherwise, tokenize the message and propagate the tree */

  else
  {
    if (_ds_tokenize(CTX, headers, body, diction)) {
      LOG(LOG_CRIT, "tokenizer failed");
    }
    whitelist_token = diction->whitelist_token;
  }


  /* Load all token statistics */
  if (_ds_getall_spamrecords (CTX, diction))
  {
    LOGDEBUG ("_ds_getall_spamrecords() failed");
    errcode = EUNKNOWN;
    goto bail;
  }

  /* Apply Bayesian Noise Reduction */
  if (CTX->flags & DSF_NOISE)
  {
    ds_diction_t p = _ds_apply_bnr(CTX, diction);
    if (p) 
      ds_diction_destroy(p);
  }

  if (CTX->flags & DSF_WHITELIST)
  {
    LOGDEBUG("Whitelist threshold: %d", CTX->wh_threshold);
  }

  /* Create a heap sort based on the token's delta from .5 */
  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {

    if (ds_term->key == CONTROL_TOKEN) {
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    if (ds_term->s.probability == 0.00000 || CTX->classification != DSR_NONE)
      _ds_calc_stat (CTX, ds_term, &ds_term->s, DTT_DEFAULT, NULL);

    if (CTX->flags & DSF_WHITELIST) {
      if (ds_term->key == whitelist_token              && 
          ds_term->s.spam_hits <= (ds_term->s.innocent_hits / 15) && 
          ds_term->s.innocent_hits > CTX->wh_threshold && 
          CTX->classification == DSR_NONE)
      {
        do_whitelist = 1;
      }
    }

    if (ds_term->frequency > 0 && ds_term->type == 'D')
    {
      ds_heap_insert (heap_sort, ds_term->s.probability, ds_term->key,
             ds_term->frequency, _ds_compute_complexity(ds_term->name));
    }

#ifdef LIBBNR_DEBUG
    if (ds_term->type == 'D')
    {
      ds_heap_insert (heap_nobnr, ds_term->s.probability, ds_term->key,
             ds_term->frequency, _ds_compute_complexity(ds_term->name));
    }
#endif

#ifdef VERBOSE
    LOGDEBUG ("Token: %s [%f] SH %ld IH %ld", ds_term->name, ds_term->s.probability, ds_term->s.spam_hits, ds_term->s.innocent_hits);
#endif

    ds_term = ds_diction_next(ds_c);
  }
  ds_diction_close(ds_c);

  /* Take the 15 most interesting tokens and generate a score */

  if (heap_sort->items == 0)
  {
    LOGDEBUG ("no tokens found in message");
    errcode = EINVAL; 
    goto bail;
  }

  /* Initialize Non-SBPH signature, if requested */

  if ( CTX->tokenizer != DSZ_SBPH
    && CTX->flags & DSF_SIGNATURE 
    && (CTX->operating_mode != DSM_CLASSIFY || ! CTX->_sig_provided))
  {
    CTX->signature = calloc (1, sizeof (struct _ds_spam_signature));
    if (CTX->signature == NULL)
    {
      LOG (LOG_CRIT, "memory allocation error");
      errcode = EUNKNOWN;
      goto bail;
    }

    CTX->signature->length =
      sizeof (struct _ds_signature_token) * diction->items;
    CTX->signature->data = malloc (CTX->signature->length);
    if (CTX->signature->data == NULL)
    {
      LOG (LOG_CRIT, "memory allocation error");
      free (CTX->signature);
      CTX->signature = NULL;
      errcode = EUNKNOWN;
      goto bail;
    }
  }

#ifdef LIBBNR_DEBUG
  {
    int x = CTX->result;
    int nobnr_result = 0;

    if (CTX->flags & DSF_NOISE) {
      nobnr_result = _ds_calc_result(CTX, heap_nobnr, diction);

      if (CTX->factors) 
        _ds_factor_destroy(CTX->factors);
      CTX->result = x;
      CTX->probability = DSP_UNCALCULATED;
    }
#endif

    result = _ds_calc_result(CTX, heap_sort, diction);

#ifdef LIBBNR_DEBUG
    if (CTX->flags & DSF_NOISE) {
      if (nobnr_result == result) {
        LOGDEBUG("BNR Decision Concurs");
      } else {
        LOGDEBUG("BNR Decision Conflicts: %d (BNR) / %d (No BNR)", result, nobnr_result);
      }
    }
  }
#endif

  if (CTX->flags & DSF_WHITELIST && do_whitelist) {
    LOGDEBUG("auto-whitelisting this message");
    CTX->result = DSR_ISINNOCENT;
    strcpy(CTX->class, LANG_CLASS_WHITELISTED);
  }

  /* Update Totals */

  /* SPAM */
  if (CTX->result == DSR_ISSPAM && CTX->operating_mode != DSM_CLASSIFY)
  {
    if (!(CTX->flags & DSF_UNLEARN)) {
      CTX->totals.spam_learned++;
      CTX->learned = 1;
    }

    if (CTX->classification == DSR_ISSPAM) 
    {
      if (CTX->flags & DSF_UNLEARN) {
        CTX->totals.spam_learned -= (CTX->totals.spam_learned > 0) ? 1 : 0;
      } else if (CTX->source == DSS_CORPUS || CTX->source == DSS_INOCULATION) {
        CTX->totals.spam_corpusfed++;
      }
      else if (SPAM_MISS(CTX))
      {
        CTX->totals.spam_misclassified++;
        if (CTX->training_mode != DST_TOE && CTX->training_mode != DST_NOTRAIN)
        {
          CTX->totals.innocent_learned -=
            (CTX->totals.innocent_learned > 0) ? 1 : 0;
        }
      }
    }

    /* INNOCENT */
  }
  else if ((CTX->result == DSR_ISINNOCENT) && 
            CTX->operating_mode != DSM_CLASSIFY)
  {
    if (!(CTX->flags & DSF_UNLEARN)) {
      CTX->totals.innocent_learned++;
      CTX->learned = 1;
    }

    if (CTX->source == DSS_CORPUS || CTX->source == DSS_INOCULATION)
    {
      CTX->totals.innocent_corpusfed++;
    }
    else if (FALSE_POSITIVE(CTX))
    {
      if (CTX->flags & DSF_UNLEARN) {
        CTX->totals.innocent_learned -= (CTX->totals.innocent_learned >0) ? 1:0;
      } else {
        CTX->totals.innocent_misclassified++;
        if (CTX->training_mode != DST_TOE && CTX->training_mode != DST_NOTRAIN)
        {
          CTX->totals.spam_learned -= (CTX->totals.spam_learned > 0) ? 1 : 0;
        }
      }
    }
  }

  /* TOE mode increments 'classified' totals */
  if (CTX->training_mode == DST_TOE && CTX->operating_mode == DSM_CLASSIFY) {
    if (CTX->result == DSR_ISSPAM) 
      CTX->totals.spam_classified++;
    else if (CTX->result == DSR_ISINNOCENT)
      CTX->totals.innocent_classified++;
  }

  _ds_increment_tokens(CTX, diction);

  /* Store all tokens */
  if (CTX->training_mode != DST_NOTRAIN) {
    if (_ds_setall_spamrecords (CTX, diction))
    {
      LOGDEBUG ("_ds_setall_spamrecords() failed");
      errcode = EUNKNOWN;
      goto bail;
    }
  }

  ds_diction_destroy (diction);
  ds_heap_destroy (heap_sort);
#ifdef LIBBNR_DEBUG
  ds_heap_destroy (heap_nobnr);
#endif

  /* One final sanity check */

  if (CTX->classification == DSR_ISINNOCENT)
  {
    CTX->probability = 0.0;
    CTX->result = DSR_ISINNOCENT;
  }
  else if (CTX->classification == DSR_ISSPAM)
  {
    CTX->probability = 1.0;
    CTX->result = DSR_ISSPAM;
  }

  return CTX->result;

bail:
  LOG(LOG_ERR, "bailing on error %d", errcode);
  ds_heap_destroy (heap_sort);
#ifdef LIBBNR_DEBUG
  ds_heap_destroy (heap_nobnr);
#endif
  ds_diction_destroy(diction);
  ds_diction_destroy(bnr_patterns);
  return errcode;
}

/*
 * _ds_process_signature()
 *
 * DESCRIPTION
 *   process an erroneously classified message processing based on signature
 *
 * INPUT ARGUMENTS
 *   parameters: DSPAM_CTX *CTX		Pointer to context containing signature
 */

int
_ds_process_signature (DSPAM_CTX * CTX)
{
  struct _ds_signature_token t;
  int num_tokens, i;
  ds_diction_t diction = ds_diction_create(24593ul);
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  int occurrence = _ds_match_attribute(CTX->config->attributes, 
     "ProcessorWordFrequency", "occurrence");

  if (diction == NULL) {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  if (CTX->signature == NULL) {
    LOG(LOG_WARNING, "DSF_SIGNATURE specified, but no signature provided.");
    ds_diction_destroy(diction);
    return EINVAL;
  }

  LOGDEBUG ("processing signature.  length: %ld", CTX->signature->length);

  CTX->result = DSS_NONE;

  if (!(CTX->flags & DSF_UNLEARN)) 
    CTX->learned = 1;

  /* INNOCENT */
  if (CTX->classification == DSR_ISINNOCENT && 
      CTX->operating_mode != DSM_CLASSIFY)
  {
    if (CTX->flags & DSF_UNLEARN) {
      CTX->totals.innocent_learned -= (CTX->totals.innocent_learned) > 0 ? 1:0;
    } else {
      if (CTX->source == DSS_ERROR) {
        CTX->totals.innocent_misclassified++;
        if (CTX->training_mode != DST_TOE && CTX->training_mode != DST_NOTRAIN)
        {
          CTX->totals.spam_learned -= (CTX->totals.spam_learned > 0) ? 1:0;
        }
      } else {
        CTX->totals.innocent_corpusfed++;
      }

      CTX->totals.innocent_learned++;
    }
  }

  /* SPAM */
  else if (CTX->classification == DSR_ISSPAM &&
           CTX->operating_mode != DSM_CLASSIFY)
  {
    if (CTX->flags & DSF_UNLEARN) {
      CTX->totals.spam_learned -= (CTX->totals.spam_learned > 0) ? 1 : 0;
    } else {
      if (CTX->source == DSS_ERROR) {
        CTX->totals.spam_misclassified++;
        if (CTX->training_mode != DST_TOE && CTX->training_mode != DST_NOTRAIN)
        {
          CTX->totals.innocent_learned -= (CTX->totals.innocent_learned > 0) ? 1:0;
        }
      } else {
        CTX->totals.spam_corpusfed++;
      }
      CTX->totals.spam_learned++;
    }
  }

  num_tokens = CTX->signature->length / sizeof (struct _ds_signature_token);

  if (CTX->class[0] == 0) {
    if (CTX->classification == DSR_ISSPAM)
      strcpy(CTX->class, LANG_CLASS_SPAM);
    else if (CTX->classification == DSR_ISINNOCENT)
      strcpy(CTX->class, LANG_CLASS_INNOCENT);
  }

  LOGDEBUG ("reversing %d tokens", num_tokens);
  for (i = 0; i < num_tokens; i++)
  {
    memcpy (&t,
            (char *) CTX->signature->data +
            (i * sizeof (struct _ds_signature_token)),
            sizeof (struct _ds_signature_token));
    ds_term = ds_diction_touch (diction, t.token, "-", 0);
    if (ds_term)
      ds_term->frequency = t.frequency;
  }

  if (_ds_getall_spamrecords (CTX, diction)) {
    ds_diction_destroy(diction);
    return EUNKNOWN;
  }

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    /* INNOCENT */
    if (CTX->classification == DSR_ISINNOCENT)
    {
      if (CTX->flags & DSF_UNLEARN) {
        if (CTX->classification == DSR_ISSPAM)
        {
          if (occurrence)
          {
            ds_term->s.innocent_hits -= ds_term->frequency;
            if (ds_term->s.innocent_hits < 0)
              ds_term->s.innocent_hits = 0;
          } else {
            ds_term->s.innocent_hits -= (ds_term->s.innocent_hits>0) ? 1:0;
          }
        }

      } else {
        if (occurrence)
        {
          ds_term->s.innocent_hits += ds_term->frequency;
        } else {
          ds_term->s.innocent_hits++;
        }

        if (CTX->source == DSS_ERROR          && 
            CTX->training_mode != DST_NOTRAIN && 
            CTX->training_mode != DST_TOE)
        {
          if (occurrence)
          {
            ds_term->s.spam_hits -= ds_term->frequency;
            if (ds_term->s.spam_hits < 0)
              ds_term->s.spam_hits = 0;
          } else {
            ds_term->s.spam_hits -= (ds_term->s.spam_hits>0) ? 1:0;
          }
        }
      }
    }


    /* SPAM */
    else if (CTX->classification == DSR_ISSPAM)
    {
      if (CTX->flags & DSF_UNLEARN) {
        if (CTX->classification == DSR_ISSPAM)
        {
          if (occurrence)
          {
            ds_term->s.spam_hits -= ds_term->frequency;
            if (ds_term->s.spam_hits < 0)
              ds_term->s.spam_hits = 0;
          } else {
            ds_term->s.spam_hits -= (ds_term->s.innocent_hits>0) ? 1:0;
          }
        }

      } else {
       if (CTX->source == DSS_ERROR          && 
           CTX->training_mode != DST_NOTRAIN && 
           CTX->training_mode != DST_TOE)
       {
          if (occurrence)
          {
            ds_term->s.innocent_hits -= ds_term->frequency;
            if (ds_term->s.innocent_hits < 0)
              ds_term->s.innocent_hits = 0;
          } else {
            ds_term->s.innocent_hits -= (ds_term->s.innocent_hits>0) ? 1:0;
          }

       }

        if (CTX->source == DSS_INOCULATION)
        {
          if (ds_term->s.innocent_hits < 2 && ds_term->s.spam_hits < 5)
            ds_term->s.spam_hits += 5;
          else
            ds_term->s.spam_hits += 2;
        } else /* ERROR or CORPUS */
        {
            if (occurrence)
            {
              ds_term->s.spam_hits += ds_term->frequency;
            } else {
              ds_term->s.spam_hits++;
            }

        }
      }
    }

    ds_term->s.status |= TST_DIRTY;
    ds_term = ds_diction_next(ds_c);
  }
  ds_diction_close(ds_c);

  if (CTX->training_mode != DST_NOTRAIN) {
    if (_ds_setall_spamrecords (CTX, diction)) {
      ds_diction_destroy(diction);
      return EUNKNOWN;
    }
  }

  if (CTX->classification == DSR_ISSPAM)
  {
    CTX->probability = 1.0;
    CTX->result = DSR_ISSPAM;
  }
  else
  {
    CTX->probability = 0.0;
    CTX->result = DSR_ISINNOCENT;
  }

  ds_diction_destroy(diction);
  return 0;
}

/*
 *  _ds_calc_stat() - Calculate the probability of a token
 *
 * DESCRIPTION
 *
 *  Calculates the probability of an individual token based on  the
 *  pvalue algorithm chosen. The resulting value largely depends on
 *  the total  amount of ham/spam in the user's corpus. The result
 *  is written to s.
 *
 * INPUT ARGUMENTS
 *      CTX           DSPAM context
 *      term          ds_term_t 
 *      token_type    DTT_ value specifying token type
 *      bnr_tot       BNR totals structure
 */

int
_ds_calc_stat (
  DSPAM_CTX * CTX,
  ds_term_t term,
  struct _ds_spam_stat *s,
  int token_type,
  struct _ds_spam_stat *bnr_tot)
{
  int min_hits, sed_hits = 0;
  long ti, ts;

  if (token_type == DTT_BNR) {
    min_hits = 25; /* Bayesian Noise Reduction patterns */

  } else {
    min_hits = 5; /* "Standard" token threshold */
  }

  /*  Statistical Sedation: Adjust hapaxial threshold to compensate for a 
   *  spam corpus imbalance 
   */

  ti = CTX->totals.innocent_learned + CTX->totals.innocent_classified;
  ts = CTX->totals.spam_learned + CTX->totals.spam_classified;
  if (CTX->training_buffer>0) {
    if (ti < 1000 && ti < ts)
    {
      sed_hits = min_hits+(CTX->training_buffer/2)+
                   (CTX->training_buffer*((ts-ti)/200));
    }

    if (ti < 2500 && ti >=1000 && ts > ti)
    {
      float spams = (ts * 1.0 / (ts * 1.0 + ti * 1.0)) * 100;
      sed_hits = min_hits+(CTX->training_buffer/2)+
                   (CTX->training_buffer*(spams/20));
    }
  } else if (! CTX->training_buffer) {
    min_hits = 5;
  }

  if (token_type != DTT_DEFAULT || sed_hits > min_hits) 
    min_hits = sed_hits;

  /*  TUM mode training only records up to 20 hits so we need to make sure we
   *  don't require more than that.
   */

  if (CTX->training_mode == DST_TUM && min_hits > 20) 
    min_hits = 20;

  if (CTX->classification == DSR_ISSPAM)
    s->probability = .7;
  else 
    s->probability = (CTX->algorithms & DSP_MARKOV) ? .5 : .4;

  /* Markovian Weighting */

  if (CTX->algorithms & DSP_MARKOV) {
    unsigned int weight;
    long num, den;
 
    /*  some utilities don't provide the token name, and so we can't compute
     *  a probability. just return something neutral.
     */
    if (term == NULL) {
      s->probability = .5;
      return 0;
    }

    weight = _ds_compute_weight(term->name);

    if (CTX->flags & DSF_BIAS) {
      num = weight * (s->spam_hits - (s->innocent_hits*2));
      den = C1 * (s->spam_hits + (s->innocent_hits*2) + C2) * 256;
    } else {
      num = (s->spam_hits - s->innocent_hits) * weight;
      den = C1 * (s->spam_hits + s->innocent_hits + C2) * 256;
    }

    if (CTX->flags & DSF_BIAS) {
      s->probability = 0.49 + ((double) num / (double) den);
    } else {
      s->probability = 0.5 + ((double) num / (double) den); 
    }

  /* Graham and Robinson Start Here */

  } else {
    int ih = 1;
    if (CTX->flags & DSF_BIAS)
      ih = 2;

    if (CTX->totals.spam_learned > 0 && CTX->totals.innocent_learned > 0)
    {
      if (token_type == DTT_BNR) {
        s->probability =
          (s->spam_hits * 1.0 / bnr_tot->spam_hits * 1.0) /
          ((s->spam_hits * 1.0 / bnr_tot->spam_hits * 1.0) +
           (s->innocent_hits * 1.0 / bnr_tot->innocent_hits * 1.0));
      } else {
        s->probability =
          (s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0) /
          ((s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0) +
           (s->innocent_hits * ih * 1.0 / CTX->totals.innocent_learned * 1.0));
      }
    }

    if (s->spam_hits == 0 && s->innocent_hits > 0) {
      s->probability = 0.01;
      if (CTX->totals.spam_learned > 0 && CTX->totals.innocent_learned > 0)
      {
        if ((1.0 / CTX->totals.spam_learned * 1.0) /
           ((1.0 / CTX->totals.spam_learned * 1.0) +
           (s->innocent_hits * ih * 1.0 / CTX->totals.innocent_learned * 1.0))
          < 0.01) 
        {
          s->probability = (1.0 / CTX->totals.spam_learned * 1.0) /
           ((1.0 / CTX->totals.spam_learned * 1.0) +
            (s->innocent_hits * ih *1.0 / CTX->totals.innocent_learned * 1.0));
        }
      }
    }
    else if (s->spam_hits > 0 && s->innocent_hits == 0) {
      s->probability = 0.99;
      if (CTX->totals.spam_learned > 0 && CTX->totals.innocent_learned > 0)
      {
        if ((s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0) /
           ((s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0) +
           (ih * 1.0 / CTX->totals.innocent_learned * 1.0))
          > 0.99)
        {
          s->probability = (s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0)
           / ((s->spam_hits * 1.0 / CTX->totals.spam_learned * 1.0) 
           + (ih * 1.0 / CTX->totals.innocent_learned * 1.0));
        }
      }
    }

    if (  (CTX->flags & DSF_BIAS && 
          (s->spam_hits + (2 * s->innocent_hits) < min_hits))
       || (!(CTX->flags & DSF_BIAS) && 
          (s->spam_hits + s->innocent_hits < min_hits)))
    {
      s->probability = (CTX->algorithms & DSP_MARKOV) ? .5000 : .4;
    }
  }

  if (s->probability < 0.0001)
    s->probability = 0.0001;

  if (s->probability > 0.9999)
    s->probability = 0.9999;

  /* Finish off Robinson */

  if (token_type != DTT_BNR && CTX->algorithms & DSP_ROBINSON)
  {
    int n = s->spam_hits + s->innocent_hits;
    double fw = ((CHI_S * CHI_X) + (n * s->probability))/(CHI_S + n);
    s->probability = fw;
  }

  return 0;
}

/*
 *  _ds_calc_result()
 *
 * DESCRIPTION
 *   Perform statistical combination of the token index
 *
 *    Passed in an index of tokens, this function is responsible for choosing
 *    and combining  the most  relevant characteristics  (based on the  algo-
 *    rithms  configured)  and calculating  libdspam's  decision  about  the 
 *    provided message sample.
 */

int
_ds_calc_result(DSPAM_CTX *CTX, ds_heap_t heap_sort, ds_diction_t diction)
{
  struct _ds_spam_stat stat;
  ds_heap_element_t node_heap;
  ds_heap_element_t heap_list[heap_sort->items];

  /* Naive-Bayesian */
  float nbay_top = 0.0;
  float nbay_bot = 0.0;
  float nbay_result = -1;
  long nbay_used = 0;            /* Total tokens used in naive bayes */
  struct nt *factor_nbayes = nt_create(NT_PTR);

  /* Graham-Bayesian */
  float bay_top = 0.0; 
  float bay_bot = 0.0;
  float bay_result = -1;
  long bay_used = 0;            /* Total tokens used in bayes */
  struct nt *factor_bayes = nt_create(NT_PTR);

  /* Burton-Bayesian */
  double abay_top = 0.0; 
  double abay_bot = 0.0;
  double abay_result = -1;
  long abay_used = 0;           /* Total tokens used in altbayes */
  struct nt *factor_altbayes = nt_create(NT_PTR);

  /* Robinson's Geometric Mean, used to calculate confidence */
  float rob_top = 0.0;      		/* Robinson's Geometric Mean */
  float rob_bot = 0.0;
  float rob_result = -1;
  double p = 0.0, q = 0.0, s = 0.0;     /* Robinson PQS Calculations */
  long rob_used = 0;            	/* Total tokens used in Robinson's GM */
  struct nt *factor_rob = nt_create(NT_PTR);
                                                                                
  /* Fisher-Robinson's Chi-Square */
  float chi_result = -1;
  long chi_used  = 0, chi_sx = 0, chi_hx = 0;
  double chi_s = 1.0, chi_h = 1.0;
  struct nt *factor_chi = nt_create(NT_PTR);
  int i;

  /* Invert the heap */
  node_heap = heap_sort->root;
  for(i=0;i<heap_sort->items;i++) {
    heap_list[(heap_sort->items-i)-1] = node_heap;
    node_heap = node_heap->next;
  }

  node_heap = heap_sort->root;

  /* BEGIN Combine Token Values */
  for(i=0;i<heap_sort->items;i++)
  {
    char *token_name;
    ds_term_t ds_term;

    node_heap = heap_list[i];
    ds_term = ds_diction_find(diction, node_heap->token);

    if (!ds_term) 
      continue;

    token_name = ds_term->name;

    if (ds_diction_getstat(diction, node_heap->token, &stat) || !token_name)
      continue;

    /* Skip BNR patterns */
    if (ds_term->type == 'B')
      continue;

    /* Set the probability if we've provided a classification */
    if (CTX->classification == DSR_ISSPAM)
      stat.probability = 1.00;
    else if (CTX->classification == DSR_ISINNOCENT)
      stat.probability = 0.00;

    /* Graham-Bayesian */
    if (CTX->algorithms & DSA_GRAHAM && bay_used < 15)
    {
        LOGDEBUG ("[graham] [%2.6f] %s (%dfrq, %lds, %ldi)",
                  stat.probability, token_name, ds_term->frequency,
                  stat.spam_hits, stat.innocent_hits);

      _ds_factor(factor_bayes, token_name, stat.probability);

      if (bay_used == 0)
      {
        bay_top = stat.probability;
        bay_bot = 1 - stat.probability;
      }
      else
      {
        bay_top *= stat.probability;
        bay_bot *= (1 - stat.probability);
      }

      bay_used++;
    }

    /* Burton Bayesian */
    if (CTX->algorithms & DSA_BURTON && abay_used < BURTON_WINDOW_SIZE)
    {
        LOGDEBUG ("[burton] [%2.6f] %s (%dfrq, %lds, %ldi)",
                  stat.probability, token_name, ds_term->frequency,
                  stat.spam_hits, stat.innocent_hits);

      _ds_factor(factor_altbayes, token_name, stat.probability);

      if (abay_used == 0)
      {
        abay_top = stat.probability;
        abay_bot = (1 - stat.probability);
      }
      else
      {
        abay_top *= stat.probability;
        abay_bot *= (1 - stat.probability);
      }

      abay_used++;

      if (abay_used < BURTON_WINDOW_SIZE && ds_term->frequency > 1 )
      {
          LOGDEBUG ("[burton] [%2.6f] %s (%dfrq, %lds, %ldi)",
                    stat.probability, token_name, ds_term->frequency,
                    stat.spam_hits, stat.innocent_hits);

        _ds_factor(factor_altbayes, token_name, stat.probability);

        abay_used++;
        abay_top *= stat.probability;
        abay_bot *= (1 - stat.probability);
      }

    }

    /* Robinson's Geometric Mean Definitions */

//#define ROB_S	0.010           /* Sensitivity */
//#define ROB_X	0.415           /* Value to use when N = 0 */
//#define ROB_CUTOFF	0.54


#define ROB_S   0.010           /* Sensitivity */
#define ROB_X   0.500           /* Value to use when N = 0 */
#define ROB_CUTOFF      0.50


    if (rob_used < 25)
    {
      float probability;
      long n = (heap_sort->items > 25) ? 25 : heap_sort->items;

      probability = ((ROB_S * ROB_X) + (n * stat.probability)) / (ROB_S + n);

#ifdef ROBINSON
#ifndef VERBOSE
      if (CTX->operating_mode != DSM_CLASSIFY)
      {
#endif
        LOGDEBUG ("[rob] [%2.6f] %s (%dfrq, %lds, %ldi)",
                  stat.probability, token_name, ds_term->frequency,
                  stat.spam_hits, stat.innocent_hits);
#ifndef VERBOSE
      }
#endif
#endif

      _ds_factor(factor_rob, token_name, stat.probability);

      if (probability < 0.3 || probability > 0.7)
      {

        if (rob_used == 0)
        {
          rob_top = probability;
          rob_bot = (1 - probability);
        }
        else
        {
          rob_top *= probability;
          rob_bot *= (1 - probability);
        }

        rob_used++;

        if (rob_used < 25 && ds_term->frequency > 1)
        {
#ifdef ROBINSON
#ifndef VERBOSE
          if (CTX->operating_mode != DSM_CLASSIFY)
          {
#endif
            LOGDEBUG ("[rob] [%2.6f] %s (%dfrq, %lds, %ldi)",
                      stat.probability, token_name, ds_term->frequency,
                      stat.spam_hits, stat.innocent_hits);

#ifndef VERBOSE
          }
#endif
#endif

          _ds_factor(factor_rob, token_name, stat.probability);

          rob_used++;
          rob_top *= probability;
          rob_bot *= (1 - probability);
        }
      }
    }
  }

  /* END Combine Token Values */

  /* Fisher-Robinson's Inverse Chi-Square */
#define CHI_CUTOFF	0.5010	/* Ham/Spam Cutoff */
#define CHI_EXCR	0.4500	/* Exclusionary Radius */
#define LN2		0.69314718055994530942 /* log e2 */

  if (CTX->algorithms & DSA_CHI_SQUARE || CTX->algorithms & DSA_NAIVE)
  {
    ds_term_t ds_term;
    ds_cursor_t ds_c;
    double fw;
    int n, exp;

    ds_c = ds_diction_cursor(diction);
    ds_term = ds_diction_next(ds_c);
    while(ds_term) {

      if (ds_term->key == CONTROL_TOKEN) {
        ds_term = ds_diction_next(ds_c);
        continue;
      }

      /* Naive-Bayesian */
      if (CTX->algorithms & DSA_NAIVE)
      {
          LOGDEBUG ("[naive] [%2.6f] %s (%dfrq, %lds, %ldi)",
                    ds_term->s.probability, ds_term->name, ds_term->frequency,
                    ds_term->s.spam_hits, ds_term->s.innocent_hits);
  
        _ds_factor(factor_nbayes, ds_term->name, stat.probability);
  
        if (nbay_used == 0)
        {
          nbay_top = stat.probability;
          nbay_bot = 1 - stat.probability;
        }
        else
        {
          nbay_top *= stat.probability;
          nbay_bot *= (1 - stat.probability);
        }
  
        nbay_used++;
      }

      if (CTX->algorithms & DSA_CHI_SQUARE) {

        /* Skip BNR Tokens */
        if (ds_term->type == 'B')
          goto CHI_NEXT;

        /* Convert the p-value */

      if (CTX->algorithms & DSP_ROBINSON) {
          fw = ds_term->s.probability;
        } else {
          n = ds_term->s.spam_hits + ds_term->s.innocent_hits;
          fw = ((CHI_S * CHI_X) + (n * ds_term->s.probability))/(CHI_S + n);
        }

        if (fabs(0.5-fw)>CHI_EXCR) {
          int iter = _ds_compute_complexity(ds_term->name);

          iter = 1;
          while(iter>0) {
            iter --;

#ifndef VERBOSE
            if (CTX->operating_mode != DSM_CLASSIFY)
            {
#endif
              LOGDEBUG ("[chi-sq] [%2.6f] %s (%dfrq, %lds, %ldi)",
                        fw, ds_term->name, ds_term->frequency,
                        ds_term->s.spam_hits, ds_term->s.innocent_hits);
#ifndef VERBOSE
            }
#endif

            _ds_factor(factor_chi, ds_term->name, ds_term->s.probability);
  
            chi_used++;
            chi_s *= (1.0 - fw);
            chi_h *= fw;
            if (chi_s < 1e-200) {
              chi_s = frexp(chi_s, &exp);
              chi_sx += exp;
            }
            if (chi_h < 1e-200) {
              chi_h = frexp(chi_h, &exp);
              chi_hx += exp; 
            }
          }
        }
      }

CHI_NEXT:
      ds_term = ds_diction_next(ds_c);
    }
    ds_diction_close(ds_c);
  }

  /* BEGIN Calculate Individual Probabilities */

  if (CTX->algorithms & DSA_NAIVE) {
    nbay_result = (nbay_top) / (nbay_top + nbay_bot);
    LOGDEBUG ("Naive-Bayesian Probability: %f Samples: %ld", nbay_result,
              nbay_used);
  }

  if (CTX->algorithms & DSA_GRAHAM) {
    bay_result = (bay_top) / (bay_top + bay_bot);
    LOGDEBUG ("Graham-Bayesian Probability: %f Samples: %ld", bay_result,
              bay_used);
  }

  if (CTX->algorithms & DSA_BURTON) {
    abay_result = (abay_top) / (abay_top + abay_bot);
    LOGDEBUG ("Burton-Bayesian Probability: %f Samples: %ld", abay_result,
              abay_used);
  }

  /* Robinson's */
  if (rob_used == 0)
  {
    p = q = s = 0;
  }
  else
  {
    p = 1.0 - pow (rob_bot, 1.0 / rob_used);
    q = 1.0 - pow (rob_top, 1.0 / rob_used);
    s = (p - q) / (p + q);
    s = (s + 1.0) / 2.0;
  }

  rob_result = s;

  if (CTX->algorithms & DSA_ROBINSON) { 
    LOGDEBUG("Robinson's Geometric Confidence: %f (Spamminess: %f, "
      "Non-Spamminess: %f, Samples: %ld)", rob_result, p, q, rob_used);
  }

  if (CTX->algorithms & DSA_CHI_SQUARE) {
    chi_s = log(chi_s) + chi_sx * LN2;
    chi_h = log(chi_h) + chi_hx * LN2;

    if (chi_used) {
      chi_s = 1.0 - chi2Q(-2.0 * chi_s, 2 * chi_used);
      chi_h = 1.0 - chi2Q(-2.0 * chi_h, 2 * chi_used);

      chi_result = ((chi_s-chi_h)+1.0) / 2.0;
    } else {
      chi_result = (float)(CHI_CUTOFF-0.1);
    }  

    LOGDEBUG("Chi-Square Confidence: %f", chi_result); 
  }

/* END Calculate Individual Probabilities */

/* BEGIN Determine Result */

  if (CTX->classification == DSR_ISSPAM) {
    CTX->result = DSR_ISSPAM;
    CTX->probability = 1.0;
  } else if (CTX->classification == DSR_ISINNOCENT) {
    CTX->result = DSR_ISINNOCENT;
    CTX->probability = 0.0;
  } else {
    struct nt *factor = NULL;

    if (CTX->algorithms & DSA_NAIVE) {
      factor = factor_nbayes;
      if ((CTX->algorithms & DSP_MARKOV && nbay_result > 0.5000) ||
          (!(CTX->algorithms & DSP_MARKOV) && nbay_result >= 0.9))
      {
        CTX->result = DSR_ISSPAM;
        CTX->probability = nbay_result;
        CTX->factors = factor;
        LOGDEBUG("using Naive-Bayes factors");
      }
    }

    if (CTX->algorithms & DSA_GRAHAM) {
      factor = factor_bayes;
      if ((CTX->algorithms & DSP_MARKOV && bay_result > 0.5000) ||
          (!(CTX->algorithms & DSP_MARKOV) && bay_result >= 0.9))
      {
        CTX->result = DSR_ISSPAM;
        CTX->probability = bay_result;
        CTX->factors = factor;
        LOGDEBUG("using Graham factors");
      }
    }

    if (CTX->algorithms & DSA_BURTON) {
      factor = factor_altbayes;
      if ((CTX->algorithms & DSP_MARKOV && abay_result > 0.5000) ||
          (!(CTX->algorithms & DSP_MARKOV) && abay_result >= 0.9))
      {
        CTX->result = DSR_ISSPAM;
        CTX->probability = abay_result;
        if (!CTX->factors) {
          CTX->factors = factor;
          LOGDEBUG("using Burton factors");
        }
      }
    }

    if (CTX->algorithms & DSA_ROBINSON) {
      factor = factor_rob;
      if ((CTX->algorithms & DSP_MARKOV && rob_result > 0.5000) ||
          (!(CTX->algorithms & DSP_MARKOV) && rob_result >= ROB_CUTOFF))
      {
        CTX->result = DSR_ISSPAM;
        if (CTX->probability < 0)
          CTX->probability = rob_result;
        if (!CTX->factors) {
          CTX->factors = factor;
          LOGDEBUG("using Robinson-Geom factors");
        }
      }
    }

    if (CTX->algorithms & DSA_CHI_SQUARE) {
     factor = factor_chi;
     if ((CTX->algorithms & DSP_MARKOV && chi_result > 0.5000) ||
         (!(CTX->algorithms & DSP_MARKOV) && chi_result >= CHI_CUTOFF))
     {
       CTX->result = DSR_ISSPAM;
       if (CTX->probability < 0)
         CTX->probability = chi_result;
       if (!CTX->factors) {
         CTX->factors = factor;
         LOGDEBUG("using Chi-Square factors");
       }
      }
    }

    if (!CTX->factors) {
      CTX->factors = factor;
      LOGDEBUG("no factors specified; using default");
    }
  }

  if (CTX->factors != factor_nbayes)
    _ds_factor_destroy(factor_nbayes);
  if (CTX->factors != factor_bayes)
    _ds_factor_destroy(factor_bayes);
  if (CTX->factors != factor_altbayes)
    _ds_factor_destroy(factor_altbayes);
  if (CTX->factors != factor_rob)
    _ds_factor_destroy(factor_rob);
  if (CTX->factors != factor_chi)
    _ds_factor_destroy(factor_chi);

  /* If somehow we haven't yet assigned a probability, assign one */
  if (CTX->probability == DSP_UNCALCULATED)
  {
    if (CTX->algorithms & DSA_GRAHAM)
      CTX->probability = bay_result;

    if (CTX->algorithms & DSA_NAIVE)
      CTX->probability = nbay_result;

    if (CTX->probability < 0 && CTX->algorithms & DSA_BURTON)
      CTX->probability = abay_result;

    if (CTX->probability < 0 && CTX->algorithms & DSA_ROBINSON)
      CTX->probability = rob_result;

    if (CTX->probability < 0 && CTX->algorithms & DSA_CHI_SQUARE)
      CTX->probability = chi_result;
  }

#ifdef VERBOSE
  if (DO_DEBUG && (!(CTX->algorithms & DSP_MARKOV))) {
    if (abay_result >= 0.9 && bay_result < 0.9)
    {
      LOGDEBUG ("CATCH: Burton Bayesian");
    }
    else if (abay_result < 0.9 && bay_result >= 0.9)
    {
      LOGDEBUG ("MISS: Burton Bayesian");
    }
                                                                                
    if (rob_result >= ROB_CUTOFF && bay_result < 0.9)
    {
      LOGDEBUG ("CATCH: Robinson's");
    }
    else if (rob_result < ROB_CUTOFF && bay_result >= 0.9)
    {
      LOGDEBUG ("MISS: Robinson's");
    }
                                                                                
    if (chi_result >= CHI_CUTOFF && bay_result < 0.9)
    {
      LOGDEBUG("CATCH: Chi-Square");
    }
    else if (chi_result < CHI_CUTOFF && bay_result >= 0.9)
    {
      LOGDEBUG("MISS: Chi-Square");
    }
  }
#endif

  /* Calculate Confidence */

  if (CTX->algorithms & DSP_MARKOV) {
    if (CTX->result == DSR_ISSPAM)
    {
      CTX->confidence = CTX->probability;
    }
    else
    {
      CTX->confidence = 1.0 - CTX->probability;
    }
  } else {
    if (CTX->result == DSR_ISSPAM)
    {
      CTX->confidence = rob_result;
    }
    else
    {
      CTX->confidence = 1.0 - rob_result;
    }
  }

  LOGDEBUG("Result Confidence: %1.2f", CTX->confidence);
  return CTX->result;
}

/*
 *  _ds_factor()
 *
 * DESCRIPTION
 *   Factors a token/value into a set
 *
 *    Adds a token/value pair to a factor set. The factor set of the dominant
 *    calculation  is provided to the  client in order to explain  libdspam's
 *    final decision about the message's classification.
 */
 
int _ds_factor(struct nt *set, char *token_name, float value) {
  struct dspam_factor *f;
  f = calloc(1, sizeof(struct dspam_factor));
  if (!f)
    return EUNKNOWN;
  f->token_name = strdup(token_name);
  f->value = value;
  nt_add(set, (void *) f);
  return 0;
}

/*
 *  _ds_factor_destroy - destroy a factor tree
 *
 */

void _ds_factor_destroy(struct nt *factors) {
  struct dspam_factor *f;
  struct nt_node *node;
  struct nt_c c;

  if (factors == NULL)
        return;
  
  node = c_nt_first(factors, &c);
  while(node != NULL) {
    f = (struct dspam_factor *) node->ptr;
    if (f)
        free(f->token_name);
    node = c_nt_next(factors, &c);
  }
  nt_destroy(factors);

  return;
} 

int libdspam_init(const char *driver) {

#ifndef STATIC_DRIVER
  if (driver) {
    if ((_drv_handle = dlopen(driver, RTLD_NOW))==NULL) {
      LOG(LOG_CRIT, "dlopen() failed: %s: %s", driver, dlerror()); 
      return EFAILURE;
    }
  }
#endif

#ifdef NCORE
  _ncsetup();
#endif

  return 0;
}

int libdspam_shutdown(void) {

#ifdef NCORE
  _nccleanup();
#endif

#ifndef STATIC_DRIVER
  if (_drv_handle) {
    int r;
    if ((r=dlclose(_drv_handle))) {
      LOG(LOG_CRIT, "dlclose() failed: %s", dlerror());
      return r;
    }
  }
#endif
   
  return 0;
}

int _ds_instantiate_bnr(
  DSPAM_CTX *CTX,
  ds_diction_t patterns,
  struct nt *stream,
  char identifier)
{
  float previous_bnr_probs[BNR_SIZE];
  ds_term_t ds_term, ds_touch;
  struct nt_node *node_nt;
  struct nt_c c_nt;
  unsigned long long crc;
  char bnr_token[64];
  int i;

  for(i=0;i<BNR_SIZE;i++)
    previous_bnr_probs[i] = 0.00000;

  node_nt = c_nt_first(stream, &c_nt);
  while(node_nt != NULL) {
    ds_term = node_nt->ptr;

    _ds_calc_stat (CTX, ds_term, &ds_term->s, DTT_DEFAULT, NULL);

    for(i=0;i<BNR_SIZE-1;i++) 
      previous_bnr_probs[i] = previous_bnr_probs[i+1];

    previous_bnr_probs[BNR_SIZE-1] = _ds_round(ds_term->s.probability);
    sprintf(bnr_token, "bnr.%c|", identifier);
    for(i=0;i<BNR_SIZE;i++) {
      char x[6];
      snprintf(x, 6, "%01.2f_", previous_bnr_probs[i]);
      strlcat(bnr_token, x, sizeof(bnr_token));
    }

    crc = _ds_getcrc64 (bnr_token);
#ifdef VERBOSE
    LOGDEBUG ("BNR pattern instantiated: '%s'", bnr_token);
#endif
    ds_touch = ds_diction_touch(patterns, crc, bnr_token, 0);
    ds_touch->type = 'B';
    node_nt = c_nt_next(stream, &c_nt);
  }
  return 0;
}

ds_diction_t _ds_apply_bnr (DSPAM_CTX *CTX, ds_diction_t diction) {

  /*
     Bayesian Noise Reduction - Contextual Symmetry Logic
     http://bnr.nuclearelephant.com
  */

  ds_diction_t bnr_patterns = ds_diction_create(3079);
  struct _ds_spam_stat bnr_tot;
  unsigned long long crc;
  BNR_CTX *BTX_S, *BTX_C;
#ifdef LIBBNR_DEBUG
  float snr;
#endif
  struct nt_node *node_nt;
  struct nt_c c_nt;
  ds_term_t ds_term, ds_touch;
  ds_cursor_t ds_c;

  if (!bnr_patterns)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  BTX_S = bnr_init(BNR_INDEX, 's');
  BTX_C = bnr_init(BNR_INDEX, 'c');

  if (!BTX_S || !BTX_C) {
    LOGDEBUG("bnr_init() failed");
    bnr_destroy(BTX_S);
    bnr_destroy(BTX_C);
    return NULL;
  }

  BTX_S->window_size = BNR_SIZE;
  BTX_C->window_size = BNR_SIZE;

  _ds_instantiate_bnr(CTX, bnr_patterns, diction->order, 's');
  _ds_instantiate_bnr(CTX, bnr_patterns, diction->chained_order, 'c');

  /* Add BNR totals to the list of load elements */
  memset(&bnr_tot, 0, sizeof(struct _ds_spam_stat));
  crc = _ds_getcrc64("bnr.t|");
  ds_touch = ds_diction_touch(bnr_patterns, crc, "bnr.t|", 0);
  ds_touch->type = 'B';

  /* Load BNR patterns */
  LOGDEBUG("Loading %ld BNR patterns", bnr_patterns->items);
  if (_ds_getall_spamrecords (CTX, bnr_patterns)) {
    LOGDEBUG ("_ds_getall_spamrecords() failed");
    return NULL;
  }

  /* Perform BNR Processing */

  if (CTX->classification == DSR_NONE	&&
      CTX->_sig_provided == 0		&&
      CTX->totals.innocent_learned + CTX->totals.innocent_classified > 2500)
  {
    int elim;
#ifdef LIBBNR_DEBUG
    char fn[MAX_FILENAME_LENGTH];
    FILE *file;
#endif

    node_nt = c_nt_first(diction->order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      bnr_add(BTX_S, ds_term->name, ds_term->s.probability);
      node_nt = c_nt_next(diction->order, &c_nt);
    }

    node_nt = c_nt_first(diction->chained_order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      bnr_add(BTX_C, ds_term->name, ds_term->s.probability);
      node_nt = c_nt_next(diction->chained_order, &c_nt);
    }

    bnr_instantiate(BTX_S);
    bnr_instantiate(BTX_C);

    /* Calculate pattern p-values */
    ds_diction_getstat(bnr_patterns, crc, &bnr_tot);
    ds_c = ds_diction_cursor(bnr_patterns);
    ds_term = ds_diction_next(ds_c);
    while(ds_term) {
      _ds_calc_stat(CTX, ds_term, &ds_term->s, DTT_BNR, &bnr_tot);
      if (ds_term->name[4] == 's')
        bnr_set_pattern(BTX_S, ds_term->name, ds_term->s.probability);
      else if (ds_term->name[4] == 'c')
        bnr_set_pattern(BTX_C, ds_term->name, ds_term->s.probability);
      ds_term = ds_diction_next(ds_c);
    } 
    ds_diction_close(ds_c);

    bnr_finalize(BTX_S);
    bnr_finalize(BTX_C);

    /* Propagate eliminations to DSPAM */

    node_nt = c_nt_first(diction->order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      bnr_get_token(BTX_S, &elim);
      if (elim) 
        ds_term->frequency--;
      node_nt = c_nt_next(diction->order, &c_nt);
    }

    node_nt = c_nt_first(diction->chained_order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      bnr_get_token(BTX_C, &elim);
      if (elim)
        ds_term->frequency--;
      node_nt = c_nt_next(diction->chained_order, &c_nt);
    }

#ifdef LIBBNR_DEBUG
    if (BTX_S->stream->items + BTX_C->stream->items +
        BTX_S->eliminations  + BTX_C->eliminations > 0)
    {
      snr = 100.0*((BTX_S->eliminations + BTX_C->eliminations + 0.0)/
            (BTX_S->stream->items + BTX_C->stream->items +
             BTX_S->eliminations  + BTX_C->eliminations)); 
    } else {
      snr = 0;
    }

    LOGDEBUG("bnr reported snr of %02.3f", snr);

#ifdef LIBBNR_GRAPH_OUTPUT
    printf("BEFORE\n\n");
    node_nt = c_nt_first(diction->order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      printf("%1.5f\n", ds_term->s.probability);
      node_nt = c_nt_next(diction->order, &c_nt);
    }

    printf("\n\nAFTER\n\n");
    node_nt = c_nt_first(diction->order, &c_nt);
    while(node_nt != NULL) {
      ds_term = node_nt->ptr;
      if (ds_term->frequency > 0)
        printf("%1.5f\n", ds_term->s.probability);
      node_nt = c_nt_next(diction->order, &c_nt);
    }
    printf("\n");
#endif
         

    snprintf(fn, sizeof(fn), "%s/bnr.log", LOGDIR);
    file = fopen(fn, "a");
    if (file != NULL) {
      fprintf(file, "-- BNR Filter Process Results --\n");
      fprintf(file, "Eliminations:\n");
      node_nt = c_nt_first(diction->order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency <= 0)
          fprintf(file, "%s ", ds_term->name);
        node_nt = c_nt_next(diction->order, &c_nt);
      }
      fprintf(file, "\n[");
      node_nt = c_nt_first(diction->order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency <= 0)
          fprintf(file, "%1.2f ", ds_term->s.probability);
        node_nt = c_nt_next(diction->order, &c_nt);
      }

      fprintf(file, "]\n\nRemaining:\n");
      node_nt = c_nt_first(diction->order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency > 0)
          fprintf(file, "%s ", ds_term->name);
        node_nt = c_nt_next(diction->order, &c_nt);
      }
      fprintf(file, "\n[");
      node_nt = c_nt_first(diction->order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency > 0)
          fprintf(file, "%1.2f ", ds_term->s.probability);
        node_nt = c_nt_next(diction->order, &c_nt);
      }

      fprintf(file, "]\nProcessed for: %s\n\n", CTX->username);

      fprintf(file, "-- Chained Tokens --\n");
      fprintf(file, "Eliminations:\n");
      node_nt = c_nt_first(diction->chained_order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency <= 0)
          fprintf(file, "%s ", ds_term->name);
        node_nt = c_nt_next(diction->chained_order, &c_nt);
      }
      fprintf(file, "\n[");
      node_nt = c_nt_first(diction->chained_order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency <= 0)
          fprintf(file, "%1.2f ", ds_term->s.probability);
        node_nt = c_nt_next(diction->chained_order, &c_nt);
      }

      fprintf(file, "]\n\nRemaining:\n");
      node_nt = c_nt_first(diction->chained_order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency > 0)
          fprintf(file, "%s ", ds_term->name);
        node_nt = c_nt_next(diction->chained_order, &c_nt);
      }
      fprintf(file, "\n[");
      node_nt = c_nt_first(diction->chained_order, &c_nt);
      while(node_nt != NULL) {
        ds_term = node_nt->ptr;
        if (ds_term->frequency > 0)
          fprintf(file, "%1.2f ", ds_term->s.probability);
        node_nt = c_nt_next(diction->chained_order, &c_nt);
      }


      fprintf(file, "]\nProcessed for: %s\n\n", CTX->username);
      fclose(file);
    }
#endif

  }

  bnr_destroy(BTX_S);
  bnr_destroy(BTX_C);

  /* Add BNR pattern to token hash */
  if (CTX->totals.innocent_learned + CTX->totals.innocent_classified > 1000) {
    ds_c = ds_diction_cursor(bnr_patterns);
    ds_term = ds_diction_next(ds_c);
    while(ds_term) {
      ds_term_t t = ds_diction_touch(diction, ds_term->key, ds_term->name, 0);
      t->type = 'B';
      ds_diction_setstat(diction, ds_term->key, &ds_term->s);
      if (t)
        t->frequency = 1;
  
#ifdef LIBBNR_DEBUG
      if (fabs(0.5-ds_term->s.probability)>0.25) {
        LOGDEBUG("Interesting BNR Pattern: %s %01.5f %lds %ldi",
                 ds_term->name,
                 ds_term->s.probability,
                 ds_term->s.spam_hits,
                 ds_term->s.innocent_hits);
      }
#endif
  
      ds_term = ds_diction_next(ds_c);
    }
    ds_diction_close(ds_c);
  }

  return bnr_patterns;
}

int _ds_increment_tokens(DSPAM_CTX *CTX, ds_diction_t diction) {
  ds_cursor_t ds_c;
  ds_term_t ds_term;
  int i = 0;
  int occurrence = _ds_match_attribute(CTX->config->attributes,
     "ProcessorWordFrequency", "occurrence");

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term) {
    unsigned long long crc;

    crc = ds_term->key;

    /* Create a signature if we're processing a message */

    if (CTX->tokenizer != DSZ_SBPH
      && CTX->flags & DSF_SIGNATURE 
      && (CTX->operating_mode != DSM_CLASSIFY || !(CTX->_sig_provided)))
    {
      struct _ds_signature_token t;

      memset(&t, 0, sizeof(t));
      t.token = crc;
      t.frequency = ds_term->frequency;
      memcpy ((char *) CTX->signature->data +
              (i * sizeof (struct _ds_signature_token)), &t,
              sizeof (struct _ds_signature_token));
    }

    /* If classification was provided, force probabilities */
    if (CTX->classification == DSR_ISSPAM)
      ds_term->s.probability = 1.00;
    else if (CTX->classification == DSR_ISINNOCENT) 
      ds_term->s.probability = 0.00;

    if (ds_term->type == 'D' &&
        ( CTX->training_mode != DST_TUM  || 
          CTX->source == DSS_ERROR       ||
          CTX->source == DSS_INOCULATION ||
          ds_term->s.spam_hits + ds_term->s.innocent_hits < 50 ||
          ds_term->key == diction->whitelist_token             ||
          CTX->confidence < 0.70))
    {
        ds_term->s.status |= TST_DIRTY;
    }

    if (ds_term->type == 'B' &&
        CTX->totals.innocent_learned + CTX->totals.innocent_classified > 500 &&
        CTX->flags & DSF_NOISE &&
        CTX->_sig_provided == 0)
    {
        ds_term->s.status |= TST_DIRTY;
    }

    /* SPAM */
    if (CTX->result == DSR_ISSPAM)
    {
      /* Inoculations increase token count considerably */
      if (CTX->source == DSS_INOCULATION)
      {
        if (ds_term->s.innocent_hits < 2 && ds_term->s.spam_hits < 5)
          ds_term->s.spam_hits += 5;
        else
          ds_term->s.spam_hits += 2;
      }

      /* Standard increase */
      else
      {
        if (CTX->flags & DSF_UNLEARN) {
          if (CTX->classification == DSR_ISSPAM)
          {
            if (occurrence)
            {
              ds_term->s.spam_hits -= ds_term->frequency;
              if (ds_term->s.spam_hits < 0)
                ds_term->s.spam_hits = 0;
            } else {
              ds_term->s.spam_hits -= (ds_term->s.spam_hits>0) ? 1:0;
            }
          }
        } else {
          if (occurrence)
          {
            ds_term->s.spam_hits += ds_term->frequency;
          } else {
            ds_term->s.spam_hits++;
          }
        }
      }

      if (SPAM_MISS(CTX) && 
          !(CTX->flags & DSF_UNLEARN) && 
          CTX->training_mode != DST_TOE &&
          CTX->training_mode != DST_NOTRAIN) 
      { 
        if (occurrence)
        {
          ds_term->s.innocent_hits -= ds_term->frequency;
          if (ds_term->s.innocent_hits < 0)
            ds_term->s.innocent_hits = 0;
        } else {
          ds_term->s.innocent_hits -= (ds_term->s.innocent_hits>0) ? 1:0;
        }
      }
    }

    /* INNOCENT */
    else
    {
      if (CTX->flags & DSF_UNLEARN) { 
        if (CTX->classification == DSR_ISINNOCENT)
        {
          if (occurrence)
          {
            ds_term->s.innocent_hits -= ds_term->frequency;
            if (ds_term->s.innocent_hits < 0)
              ds_term->s.innocent_hits = 0;
          } else {
            ds_term->s.innocent_hits -= (ds_term->s.innocent_hits>0) ? 1:0;
          }
        }
      } else {
        if (occurrence)
        {
          ds_term->s.innocent_hits += ds_term->frequency;
        } else {
          ds_term->s.innocent_hits++;
        }
      }

      if (FALSE_POSITIVE(CTX)         && 
          !(CTX->flags & DSF_UNLEARN) && 
          CTX->training_mode != DST_TOE &&
          CTX->training_mode != DST_NOTRAIN)
      {

        if (occurrence)
        {
          ds_term->s.spam_hits -= ds_term->frequency;
          if (ds_term->s.spam_hits < 0)
            ds_term->s.spam_hits = 0;
        } else {
          ds_term->s.spam_hits -= (ds_term->s.spam_hits>0) ? 1:0;
        }

      }
    }

    ds_term = ds_diction_next(ds_c);
    i++;
  }
  ds_diction_close(ds_c);
  return 0;
}
