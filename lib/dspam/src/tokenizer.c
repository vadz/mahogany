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
 * tokenizer.c - tokenizer functions
 *
 * DESCRIPTION
 *   The tokenizer subroutines are responsible for decomposing a message into
 *   its colloquial components. All components are stored collectively in
 *   a diction object, passed into the function.
 *
 */

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
#include "tokenizer.h"
#include "util.h"
#include "libdspam.h"
#include "language.h"
#ifdef NCORE
#include <ncore/ncore.h>
#include <ncore/types.h>
#include "ncore_adp.h"
#endif

#ifdef NCORE
extern nc_dev_t		g_ncDevice;
extern NC_STREAM_CTX	g_ncDelimiters;
#endif

/*
 * _ds_tokenize() - tokenize the message
 *
 * DESCRIPTION
 *    tokenizes the supplied message
 *
 * INPUT ARGUMENTS
 *     DSPAM_CTX *CTX    pointer to context
 *     char *header      pointer to message header
 *     char *body        pointer to message body
 *     ds_diction_t      diction to store components
 *
 * RETURN VALUES
 *   standard errors on failure
 *   zero if successful
 *
 */

int
_ds_tokenize (DSPAM_CTX * CTX, char *headers, char *body, ds_diction_t diction)
{
  if (diction == NULL)
    return EINVAL;

  if (CTX->tokenizer == DSZ_SBPH || CTX->tokenizer == DSZ_OSB)
    return _ds_tokenize_sparse(CTX, headers, body, diction);
  else
    return _ds_tokenize_ngram(CTX, headers, body, diction);
}

int _ds_tokenize_ngram(
  DSPAM_CTX *CTX, 
  char *headers, 
  char *body, 
  ds_diction_t diction)
{
  char *token;				/* current token */
  char *previous_token = NULL;		/* used for bigrams (chained tokens) */
#ifdef NCORE
  nc_strtok_t NTX;
#endif
  char *line = NULL;			/* header broken up into lines */
  char *ptrptr;
  char heading[128];			/* current heading */
  int l, tokenizer = CTX->tokenizer;

  struct nt *header = NULL;
  struct nt_node *node_nt;
  struct nt_c c_nt;

  /* Tokenize URLs in message */

  if (_ds_match_attribute(CTX->config->attributes, "ProcessorURLContext", "on"))  {
    _ds_url_tokenize(diction, body, "http://");
    _ds_url_tokenize(diction, body, "www.");
    _ds_url_tokenize(diction, body, "href=");
  }

  /*
   * Header Tokenization 
   */
 
  header = nt_create (NT_CHAR);
  if (header == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  line = strtok_r (headers, "\n", &ptrptr);
  while (line) {
    nt_add (header, line);
    line = strtok_r (NULL, "\n", &ptrptr);
  }

  node_nt = c_nt_first (header, &c_nt);
  heading[0] = 0;
  while (node_nt) {
    int multiline;

#ifdef VERBOSE
    LOGDEBUG("processing line: %s", node_nt->ptr);
#endif

    line = node_nt->ptr;
    token = strtok_r (line, ":", &ptrptr);
    if (token && token[0] != 32 && token[0] != 9 && !strstr (token, " "))
    {
      multiline = 0;
      strlcpy (heading, token, 128);
      previous_token = NULL;
    } else {
      multiline = 1;
    }

#ifdef VERBOSE
    LOGDEBUG ("Reading '%s' header from: '%s'", heading, line);
#endif

    if (CTX->flags & DSF_WHITELIST) {
      /* Use the entire From: line for auto-whitelisting */

      if (!strcmp(heading, "From")) {
        char wl[256];
        char *fromline = line + 5;
        unsigned long long whitelist_token;

        if (fromline[0] == 32) 
          fromline++;
        snprintf(wl, sizeof(wl), "%s*%s", heading, fromline);
        whitelist_token = _ds_getcrc64(wl);
        ds_diction_touch(diction, whitelist_token, wl, 0);
        diction->whitelist_token = whitelist_token;
      }
    }

    /* Received headers use a different set of delimiters to preserve things
       like ip addresses */

    token = strtok_r ((multiline) ? line : NULL, DELIMITERS_HEADING, &ptrptr);

    while (token)
    {
      l = strlen(token);

      if (l >= 1 && l < 50)
      {
#ifdef VERBOSE
        LOGDEBUG ("Processing '%s' token in '%s' header", token, heading);
#endif

        /* Process "current" token */
        if (!_ds_process_header_token
            (CTX, token, previous_token, diction, heading) && 
            (tokenizer == DSZ_CHAIN))
        {
          previous_token = token;
        }
      }

      token = strtok_r (NULL, DELIMITERS_HEADING, &ptrptr);
    }

    previous_token = NULL;
    node_nt = c_nt_next (header, &c_nt);
  }

  nt_destroy (header);

  /*
   * Body Tokenization
   */

#ifdef VERBOSE
  LOGDEBUG("parsing message body");
#endif

#ifdef NCORE
  token = strtok_n (body, &g_ncDelimiters, &NTX);
#else
  token = strtok_r (body, DELIMITERS, &ptrptr);
#endif
  while (token != NULL)
  {
    l = strlen (token);
    if (l >= 1 && l < 50)
    {
      /* Process "current" token */ 
      if ( !_ds_process_body_token(CTX, token, previous_token, diction)
        && tokenizer == DSZ_CHAIN)
      {
        previous_token = token;
      }
    } 
#ifdef NCORE
    token = strtok_n (NULL, NULL, &NTX);
#else
    token = strtok_r (NULL, DELIMITERS, &ptrptr);
#endif
  }

  /* Final token reassembly (anything left in the buffer) */

  return 0;
}

int _ds_tokenize_sparse(
  DSPAM_CTX *CTX, 
  char *headers, 
  char *body, 
  ds_diction_t diction)
{
  int i;
  char *token;				/* current token */
  char *previous_tokens[SPARSE_WINDOW_SIZE];	/* sparse chain */
#ifdef NCORE
  nc_strtok_t NTX;
#endif

  char *line = NULL;			/* header broken up into lines */
  char *ptrptr;
  char *bitpattern;

  char heading[128];			/* current heading */
  int l;

  struct nt *header = NULL;
  struct nt_node *node_nt;
  struct nt_c c_nt;

  for(i=0;i<SPARSE_WINDOW_SIZE;i++)
    previous_tokens[i] = NULL;

  bitpattern = _ds_generate_bitpattern(_ds_pow2(SPARSE_WINDOW_SIZE));

  /* Tokenize URLs in message */

  if (_ds_match_attribute(CTX->config->attributes, "ProcessorURLContext", "on"))
  {
    _ds_url_tokenize(diction, body, "http://");
    _ds_url_tokenize(diction, body, "www.");
    _ds_url_tokenize(diction, body, "href=");
  }

  /*
   * Header Tokenization 
   */
 
  header = nt_create (NT_CHAR);
  if (header == NULL)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    free(bitpattern);
    return EUNKNOWN;
  }

  line = strtok_r (headers, "\n", &ptrptr);
  while (line) {
    nt_add (header, line);
    line = strtok_r (NULL, "\n", &ptrptr);
  }

  node_nt = c_nt_first (header, &c_nt);
  heading[0] = 0;
  while (node_nt) {
    int multiline;

    _ds_sparse_clear(previous_tokens);

    line = node_nt->ptr;
    token = strtok_r (line, ":", &ptrptr);
    if (token && token[0] != 32 && token[0] != 9 && !strstr (token, " "))
    {
      multiline = 0;
      strlcpy (heading, token, 128);
      _ds_sparse_clear(previous_tokens);
    } else {
      multiline = 1;
    }

#ifdef VERBOSE
    LOGDEBUG ("Reading '%s' header from: '%s'", heading, line);
#endif

    if (CTX->flags & DSF_WHITELIST) {
      /* Use the entire From: line for auto-whitelisting */

      if (!strcmp(heading, "From")) {
        char wl[256];
        char *fromline = line + 5;
        unsigned long long whitelist_token;

        if (fromline[0] == 32) 
          fromline++;
        snprintf(wl, sizeof(wl), "%s*%s", heading, fromline);
        whitelist_token = _ds_getcrc64(wl); 
        ds_diction_touch(diction, whitelist_token, wl, 0);
        diction->whitelist_token = whitelist_token;
      }
    }

    /* Received headers use a different set of delimiters to preserve things
       like ip addresses */

    token = strtok_r ((multiline) ? line : NULL, DELIMITERS_HEADING, &ptrptr);

    while (token)
    {
      l = strlen(token);

      if (l > 0 && l < 50)
      {
#ifdef VERBOSE
        LOGDEBUG ("Processing '%s' token in '%s' header", token, heading);
#endif
        _ds_map_header_token (CTX, token, previous_tokens, diction, heading, bitpattern);
      }

      token = strtok_r (NULL, DELIMITERS_HEADING, &ptrptr);
    }

    for(i=0;i<SPARSE_WINDOW_SIZE;i++) {
      _ds_map_header_token(CTX, NULL, previous_tokens, diction, heading, bitpattern);
    }

    _ds_sparse_clear(previous_tokens);
    node_nt = c_nt_next (header, &c_nt);
  }
  nt_destroy (header);

  /*
   * Body Tokenization
   */

#ifdef NCORE
  token = strtok_n (body, &g_ncDelimiters, &NTX);
#else
  token = strtok_r (body, DELIMITERS, &ptrptr);
#endif
  while (token != NULL)
  {
    l = strlen (token);
    if (l > 0 && l < 50)
    {
      /* Process "current" token */ 
      _ds_map_body_token (CTX, token, previous_tokens, diction, bitpattern);
    } 

#ifdef NCORE
    token = strtok_n (NULL, NULL, &NTX);
#else
    token = strtok_r (NULL, DELIMITERS, &ptrptr);
#endif
  }

  for(i=0;i<SPARSE_WINDOW_SIZE;i++) {
    _ds_map_body_token(CTX, NULL, previous_tokens, diction, bitpattern);
  }

  _ds_sparse_clear(previous_tokens); 

  free(bitpattern);
  return 0;
}

/*
 * _ds_{process,map}_{header,body}_token()
 *
 * DESCRIPTION
 *  Token processing and mapping functions
 *    _ds_process_header_token
 *    _ds_process_body_token
 *    _ds_map_header_token
 *    _ds_map_body_token
 *
 *  These functions are responsible to converting the input words into
 *  full blown tokens with CRCs, probabilities, and producing variants
 *  based on the tokenizer approach applied. 
 */
 
int
_ds_process_header_token (DSPAM_CTX * CTX, char *token,
                          const char *previous_token, ds_diction_t diction,
                          const char *heading)
{
  char combined_token[256];
  unsigned long long crc;
  char *tweaked_token;

  if (_ds_match_attribute(CTX->config->attributes, "IgnoreHeader", heading))
    return 0;

  if (!strncmp(heading, "X-DSPAM-", 8))
    return 0;

  /* This is where we used to ignore certain headings */

  if (heading[0] != 0)
    snprintf (combined_token, sizeof (combined_token),
              "%s*%s", heading, token);
  else
    strlcpy (combined_token, token, sizeof (combined_token));

  tweaked_token = _ds_truncate_token(token);
  if (tweaked_token == NULL)
    return EUNKNOWN;

  snprintf(combined_token, sizeof(combined_token), "%s*%s", heading, tweaked_token);

  crc = _ds_getcrc64 (combined_token);
#ifdef VERBOSE
  LOGDEBUG ("Token Hit: '%s'", combined_token);
#endif
  ds_diction_touch(diction, crc, combined_token, 0);

  if (CTX->tokenizer == DSZ_CHAIN && previous_token != NULL) 
  {
    char *tweaked_previous;

    tweaked_previous = _ds_truncate_token(previous_token);
    if (tweaked_previous == NULL)
      return EUNKNOWN;

    snprintf (combined_token, sizeof (combined_token),
              "%s*%s+%s", heading, tweaked_previous, tweaked_token);
    crc = _ds_getcrc64 (combined_token);

    ds_diction_touch(diction, crc, combined_token, DSD_CHAINED);
    free(tweaked_previous);
  }

  free(tweaked_token);
  return 0;
}

int
_ds_process_body_token (DSPAM_CTX * CTX, char *token,
                        const char *previous_token, ds_diction_t diction)
{
  char combined_token[256];
  unsigned long long crc;
  char *tweaked_token;

  tweaked_token = _ds_truncate_token(token);
  if (tweaked_token == NULL)
    return EUNKNOWN;

  crc = _ds_getcrc64 (tweaked_token);

  ds_diction_touch(diction, crc, tweaked_token, DSD_CONTEXT);

  if (CTX->tokenizer == DSZ_CHAIN && previous_token != NULL)
  {
    char *tweaked_previous = _ds_truncate_token(previous_token);
    if (tweaked_previous == NULL)
      return EUNKNOWN;

    snprintf (combined_token, sizeof (combined_token), "%s+%s",
              tweaked_previous, tweaked_token);
    crc = _ds_getcrc64 (combined_token);

    ds_diction_touch(diction, crc, combined_token, DSD_CHAINED | DSD_CONTEXT);
    free(tweaked_previous);
  }
  free(tweaked_token);

  return 0;
}


int
_ds_map_header_token (DSPAM_CTX * CTX, char *token,
                      char **previous_tokens, ds_diction_t diction,
                      const char *heading, const char *bitpattern)
{
  int i, t, keylen, breadth;
  u_int32_t mask;
  unsigned long long crc;
  char key[256];
  int active = 0, top, tokenizer = CTX->tokenizer;

  if (_ds_match_attribute(CTX->config->attributes, "IgnoreHeader", heading))
    return 0;

  if (!strncmp(heading, "X-DSPAM-", 8))
    return 0;

  /* Shift all previous tokens up */
  for(i=0;i<SPARSE_WINDOW_SIZE-1;i++) {
    previous_tokens[i] = previous_tokens[i+1];
    if (previous_tokens[i])
      active++;
  }

  previous_tokens[SPARSE_WINDOW_SIZE-1] = token;

  if (token) 
    active++;

  breadth = _ds_pow2(active);
  
  /* Iterate and generate all keys necessary */
  for (mask=0; mask < breadth; mask++) {
    int terms = 0;

    key[0] = 0;
    keylen = 0;
    t = 0;
    top = 1;

    /* Each Bit */
    for(i=0;i<SPARSE_WINDOW_SIZE;i++) {

      if (t) {
        if (keylen < (sizeof(key)-1)) {
          key[keylen] = '+';
          key[++keylen] = 0;
        }
      }

      if (bitpattern[(mask*SPARSE_WINDOW_SIZE) + i] == 1) {
        if (previous_tokens[i] == NULL || previous_tokens[i][0] == 0) {
          if (keylen < (sizeof(key)-1)) {
            key[keylen] = '#';
            key[++keylen] = 0;
          }
        }
        else
        {
          int tl = strlen(previous_tokens[i]);
          if (keylen + tl < (sizeof(key)-1)) {
            strcpy(key+keylen, previous_tokens[i]);
            keylen += tl;
          }
          terms++;
        }
      } else {
        if (keylen < (sizeof(key)-1)) {
          key[keylen] = '#';
          key[++keylen] = 0;
        }
      }
      t++;
    }

    /* If the bucket has at least 1 literal, hit it */
    if ((tokenizer == DSZ_SBPH && terms != 0) ||
        (tokenizer == DSZ_OSB  && terms == 2))
    {
      char hkey[256];
      char *k = key;
      while(keylen>2 && !strcmp((key+keylen)-2, "+#")) {
        key[keylen-2] = 0;
        keylen -=2;
      }
      while(!strncmp(k, "#+", 2)) {
        top = 0; 
        k+=2;
        keylen -= 2;
      }

      if (top) {
        snprintf(hkey, sizeof(hkey), "%s*%s", heading, k);
        crc = _ds_getcrc64(hkey);
        ds_diction_touch(diction, crc, hkey, DSD_CONTEXT);
      }
    }
  }

  return 0;
}

int
_ds_map_body_token (
  DSPAM_CTX * CTX,
  char *token,
  char **previous_tokens,
  ds_diction_t diction, 
  const char *bitpattern)
{
  int i, t, keylen, breadth;
  int top, tokenizer = CTX->tokenizer;
  unsigned long long crc;
  char key[256];
  int active = 0;
  u_int32_t mask;

  /* Shift all previous tokens up */
  for(i=0;i<SPARSE_WINDOW_SIZE-1;i++) {
    previous_tokens[i] = previous_tokens[i+1];
    if (previous_tokens[i]) 
      active++;
  }

  previous_tokens[SPARSE_WINDOW_SIZE-1] = token;
  if (token) 
    active++;

  breadth = _ds_pow2(active);

  /* Iterate and generate all keys necessary */

  for(mask=0;mask < breadth;mask++) {
    int terms = 0;
    t = 0;

    key[0] = 0;
    keylen = 0;
    top = 1;

    /* Each Bit */
    for(i=0;i<SPARSE_WINDOW_SIZE;i++) {
      if (t) {
        if (keylen < (sizeof(key)-1)) {
           key[keylen] = '+';
           key[++keylen] = 0;
        }
      }
      if (bitpattern[(mask*SPARSE_WINDOW_SIZE) + i] == 1) {
        if (previous_tokens[i] == NULL || previous_tokens[i][0] == 0) {
          if (keylen < (sizeof(key)-1)) {
            key[keylen] = '#';
            key[++keylen] = 0;
          }
        }
        else
        {
          int tl = strlen(previous_tokens[i]);
          if (keylen + tl < (sizeof(key)-1)) {
            strcpy(key+keylen, previous_tokens[i]);
            keylen += tl;
          }
          terms++;
        }
      } else {
        if (keylen < (sizeof(key)-1)) {
          key[keylen] = '#';
          key[++keylen] = 0;
        }
      }
      t++;
    }

    /* If the bucket has at least 1 literal, hit it */
    if ((tokenizer == DSZ_SBPH && terms != 0) ||
        (tokenizer == DSZ_OSB  && terms == 2))
    {
      char *k = key;
      while(keylen>2 && !strcmp((key+keylen)-2, "+#")) {
        key[keylen-2] = 0;
        keylen -=2;
      }
      while(!strncmp(k, "#+", 2)) {
        top = 0; 
        k+=2;
        keylen -=2;
      }
 
      if (top) {
        crc = _ds_getcrc64(k);
        ds_diction_touch(diction, crc, k, DSD_CONTEXT);
      }
    }
  }

  return 0;
}

/* 
 *  _ds_degenerate_message()
 *
 * DESCRIPTION
 *   Degenerate the message into headers, body and tokenizable pieces
 *
 *   This function is responsible for analyzing the actualized message and
 *   degenerating it into only the components which are tokenizable.  This 
 *   process  effectively eliminates much HTML noise, special symbols,  or
 *   other  non-tokenizable/non-desirable components. What is left  is the
 *   bulk of  the message  and only  desired tags,  URLs, and other  data.
 *
 * INPUT ARGUMENTS
 *      header    pointer to buffer containing headers
 *      body      pointer to buffer containing message body
 */

int _ds_degenerate_message(DSPAM_CTX *CTX, buffer * header, buffer * body)
{
  char *decode, *x, *y;
  struct nt_node *node_nt, *node_header;
  struct nt_c c_nt, c_nt2;
  int i = 0;
  char heading[1024];

  if (! CTX->message) 
  {
    LOG (LOG_WARNING, "_ds_actualize_message() failed: CTX->message is NULL");
    return EUNKNOWN;
  }

  /* Iterate through each component and create large header/body buffers */

  node_nt = c_nt_first (CTX->message->components, &c_nt);
  while (node_nt != NULL)
  {
    struct _ds_message_part *block = (struct _ds_message_part *) node_nt->ptr;

#ifdef VERBOSE
    LOGDEBUG ("Processing component %d", i);
#endif

    if (! block->headers || ! block->headers->items)
    {
#ifdef VERBOSE
      LOGDEBUG ("  : End of Message Identifier");
#endif
    }

    else 
    {
      struct _ds_header_field *current_header;

      /* Accumulate the headers */
      node_header = c_nt_first (block->headers, &c_nt2);
      while (node_header != NULL)
      {
        current_header = (struct _ds_header_field *) node_header->ptr;
        snprintf (heading, sizeof (heading),
                  "%s: %s\n", current_header->heading,
                  current_header->data);
        buffer_cat (header, heading);
        node_header = c_nt_next (block->headers, &c_nt2);
      }

      decode = block->body->data;

      if (block->media_type == MT_TEXT    ||
               block->media_type == MT_MESSAGE ||
               block->media_type == MT_UNKNOWN ||
               (block->media_type == MT_MULTIPART && !i))
      {
        /* Accumulate the bodies, skip attachments */

        if (
             (   block->encoding == EN_BASE64 
              || block->encoding == EN_QUOTED_PRINTABLE)
            && ! block->original_signed_body)
        {
          if (block->content_disposition != PCD_ATTACHMENT)
          {
            LOGDEBUG ("decoding message block from encoding type %d",
                      block->encoding);
            decode = _ds_decode_block (block);
          }
        }
  
        /* We found a tokenizable body component, add prefilters */

        if (decode)
        {
          char *decode2 = strdup(decode);
          size_t len = strlen(decode2) + 1;
  
          /* -- PREFILTERS BEGIN -- */
  
          /* Hexadecimal 8-Bit Encodings */

          if (block->encoding == EN_8BIT) {
            char hex[5] = "0x00";
            int conv;

            x = strchr(decode2, '%');
            while(x) {
              if (isxdigit((unsigned char) x[1]) && 
                  isxdigit((unsigned char) x[2])) 
               {
                hex[2] = x[1];
                hex[3] = x[2];

                conv = strtol(hex, NULL, 16);
                if (conv) {
                  x[0] = conv;
                  memmove(x+1, x+3, len-((x+3)-decode2));
                  len -= 2;
                }
              }
              x = strchr(x+1, '%');
            }
          }

          /* HTML-Specific Filters */
  
          if (block->media_subtype == MST_HTML) {
  
            /* Remove long HTML Comments */
            x = strstr (decode2, "<!--");
            while (x != NULL)
            {
              y = strstr (x, "-->");
              if (y)
              {
                memmove(x, y + 3, len-((y+3)-decode2)); //strlen (y + 3) + 1);
                len -= ((y+3) - x);
                x = strstr (x, "<!--");
              }
              else
              {
                x = strstr (x + 4, "<!--");
              }
            }

            /* Remove short HTML Comments */
            x = strstr (decode2, "<!");
            while (x != NULL)
            {
              y = strchr (x, '>');
              if (y != NULL)
              {
                memmove(x, y + 1, len-((y+1)-decode2)); //strlen (y + 1) + 1);
                len -= ((y+1) - x);
                x = strstr (x, "<!");
              }
              else
              {
                x = strstr (x + 2, "<!");
              }
            }

            /* Remove short HTML tags and those we want to ignore */
            x = strchr (decode2, '<');
            while (x != NULL)
            {
              int erase = 0;
              y = strchr (x, '>');
              if (y != NULL) {
                if (y - x <= 15
                      || x[1] == '/'
                      || !strncasecmp (x + 1, "td ", 3)
                      || !strncasecmp (x + 1, "table ", 6)
                      || !strncasecmp (x + 1, "tr ", 3)
                      || !strncasecmp (x + 1, "div ", 4)
                      || !strncasecmp (x + 1, "p ", 2)
                      || !strncasecmp (x + 1, "body ", 5)
                      || !strncasecmp (x + 1, "!doctype", 8)
                      || !strncasecmp (x + 1, "blockquote", 10))
                {
                  erase = 1;
                }

                if (!erase) {
                  char *p = strchr (x, ' ');
                  if (!p || p > y) 
                    erase = 1;
                }
              }
              if (erase)
              {
                memmove(x, y + 1, len-((y+1)-decode2)); //strlen (y + 1) + 1);
                len -= ((y+1) - x);
                x = strstr (x, "<");
              }
              else
              {
                if (y) 
                  x = strstr (y + 1, "<");
                else
                  x = strstr (x + 1, "<");
              }
            }
          }

          /* -- PREFILTERS END -- */
  
          buffer_cat (body, decode2);
          free(decode2);

          /* If we've decoded the body, save the original copy */
          if (decode != block->body->data)
          {
            block->original_signed_body = block->body;
            block->body = buffer_create (decode);
            free (decode);
          }
        }
      }
    }
    node_nt = c_nt_next (CTX->message->components, &c_nt);
    i++;
  } /* while (node_nt != NULL) */

  if (header->data == NULL)
    buffer_cat (header, " ");

  if (body->data == NULL)
    buffer_cat (body, " ");

  return 0;
}

int _ds_url_tokenize(ds_diction_t diction, char *body, const char *key) 
{
  char *token, *url_ptr, *url_token, *ptr;
  char combined_token[256];
  unsigned long long crc;
  int key_len = strlen(key);

#ifdef VERBOSE
  LOGDEBUG("scanning for urls: %s\n", key);
#endif
  if (!body)
    return EINVAL;
  url_ptr = body;

  token = strcasestr(url_ptr, key);
  while (token != NULL)
  {
    int i = 0, old;

    while(token[i] 
       && token[i] > 32  
       && token[i] != '>' 
       && ((token[i] != '\"' && token[i] != '\'') || i <= key_len))
      i++;
    old = token[i];
    token[i] = 0; /* parse in place */

    /* Tokenize URL */
    url_token = strtok_r (token, DELIMITERS, &ptr);
    while (url_token != NULL)
    {
      snprintf (combined_token, sizeof (combined_token), "Url*%s", url_token);
      crc = _ds_getcrc64 (combined_token);
      ds_diction_touch(diction, crc, combined_token, 0);
      url_token = strtok_r (NULL, DELIMITERS, &ptr);
    }
    memset (token, 32, i);
    token[i] = old;
    url_ptr = token + i;
    token = strcasestr(url_ptr, key);
  }
  return 0;
}

/* Truncate tokens with EOT delimiters */
char * _ds_truncate_token(const char *token) {
  char *tweaked;
  int i; 

  if (token == NULL)
    return NULL;

  tweaked = strdup(token);

  if (tweaked == NULL)
    return NULL;

  i = strlen(tweaked);
  while(i>1 && strspn(tweaked+i-2, DELIMITERS_EOT)) {
    tweaked[i-1] = 0;
    i--;
  } 

  return tweaked;
}

/*
 *  _ds_spbh_clear
 *
 * DESCRIPTION
 *   Clears the SBPH stack
 *   
 *   Clears and frees all of the tokens in the SBPH stack. Used when a
 *   boundary has been crossed (such as a new message header) where
 *   tokens from the previous boundary are no longer useful.
 */

void _ds_sparse_clear(char **previous_tokens) {
  int i;
  for(i=0;i<SPARSE_WINDOW_SIZE;i++) 
    previous_tokens[i] = NULL;
  return;
}

/*
 * _ds_generate_bitpattern
 *
 * DESCRIPTION
 *   Generates a sparse bitpattern for SPARSE_WINDOW_SIZE
 *
 *   This pattern is then used to create token patterns when using SBPH or OSB
 *
 */

char *_ds_generate_bitpattern(int breadth) {
  char *bitpattern;
  unsigned long mask, exp;
  int i;

  bitpattern = malloc(SPARSE_WINDOW_SIZE * breadth);

  for(mask=0;mask<breadth;mask++) {
      for(i=0;i<SPARSE_WINDOW_SIZE;i++) {
          exp = (i) ? _ds_pow2(i) : 1;
          /* Reverse pos = SPARSE_WINDOW_SIZE - (i+1); */
          if (mask & exp)
          {
              bitpattern[(mask*SPARSE_WINDOW_SIZE) + i] = 1;
          }
          else
          {
              bitpattern[(mask*SPARSE_WINDOW_SIZE) + i] = 0;
          }
      }
  }

  return bitpattern;
}

