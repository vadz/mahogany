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
 * decode.c - message decoding and parsing
 *
 *  DESCRIPTION
 *    This set of functions performs parsing and decoding of a message and
 *    embeds its components into a ds_message_t structure, suitable for
 *    logical access.
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "decode.h"
#include "error.h"
#include "util.h"
#include "language.h"
#include "buffer.h"
#include "base64.h"
#include "libdspam.h"
#ifdef NCORE
#include "ncore_adp.h"
#endif

/*
 * _ds_actualize_message (const char *message)
 *
 * DESCRIPTION
 *   primary message parser
 *
 *   this function performs all decoding and actualization of the message
 *   into the message structures defined in the .h
 *
 * INPUT ARGUMENTS
 *      message    message to decode
 *
 * RETURN VALUES
 *   pointer to an allocated message structure (ds_message_t), NULL on failure
 */

ds_message_t
_ds_actualize_message (const char *message)
{
  char *line, *in = strdup (message), *m_in;
  ds_message_part_t current_block;
  ds_header_t current_heading = NULL;
  struct nt *boundaries = nt_create (NT_CHAR);
  ds_message_t out = (ds_message_t) calloc (1, sizeof (struct _ds_message));
  int block_position = BP_HEADER;
  int in_content = 0;

  m_in = in;

  if (!in || !boundaries || !out)
    goto MEMFAIL;

  out->components = nt_create (NT_PTR);
  if (!out->components)
    goto MEMFAIL;

  current_block = _ds_create_message_part ();
  if (!current_block) 
    goto MEMFAIL;

  if (nt_add (out->components, (void *) current_block) == NULL)
    LOG (LOG_CRIT, ERR_MEM_ALLOC);

  /* Read the message from memory */

  line = strsep (&in, "\n");
  while (line) 
  {

    /* Header processing */

    if (block_position == BP_HEADER)
    {

      /* If we see two boundaries converged on top of one another */

      if (_ds_match_boundary (boundaries, line))
      {

        /* Add the boundary as the terminating boundary */

        current_block->terminating_boundary = strdup (line + 2);
        current_block->original_encoding = current_block->encoding;

        _ds_decode_headers(current_block);
        current_block = _ds_create_message_part ();

        if (!current_block) 
        {
          LOG (LOG_CRIT, ERR_MEM_ALLOC);
          goto MEMFAIL;
        }

        if (nt_add (out->components, (void *) current_block) == NULL)
          goto MEMFAIL;

        block_position = BP_HEADER;
      }

      /* Concatenate multiline headers to the original header field data */

      else if (line[0] == 32 || line[0] == '\t')
      {
        if (current_heading)
        {
          char *eow, *ptr;

          ptr = realloc (current_heading->data,
                         strlen (current_heading->data) + strlen (line) + 2);
          if (ptr) 
          {
            current_heading->data = ptr;
            strcat (current_heading->data, "\n");
            strcat (current_heading->data, line);
          } else {
            goto MEMFAIL;
          }

          /* Our concatenated data doesn't have any whitespace between lines */
          for(eow=line;eow[0] && isspace((int) eow[0]);eow++) { }

          ptr =
            realloc (current_heading->concatenated_data,
              strlen (current_heading->concatenated_data) + strlen (eow) + 1);
          if (ptr) 
          {
            current_heading->concatenated_data = ptr;
            strcat (current_heading->concatenated_data, eow);
          } else {
            goto MEMFAIL;
          }

          if (current_heading->original_data) {
            ptr =
              realloc (current_heading->original_data,
                       strlen (current_heading->original_data) +
                               strlen (line) + 2);
            if (ptr) {
              current_heading->original_data = ptr;
              strcat (current_heading->original_data, "\n");
              strcat (current_heading->original_data, line);
            } else {
              goto MEMFAIL;
            }
          }

          _ds_analyze_header (current_block, current_heading, boundaries);
        }
      }

      /* New header field */

      else if (line[0] != 0)
      {
        ds_header_t header = _ds_create_header_field (line);

        if (header != NULL)
        {
          _ds_analyze_header (current_block, header, boundaries);
          current_heading = header;
          nt_add (current_block->headers, header);
        }


      /* line[0] == 0; switch to body */

      } else {
        block_position = BP_BODY;
      }
    }

    /* Body processing */

    else if (block_position == BP_BODY)
    {
      /* Look for a boundary in the header of a part */

      if (!strncasecmp (line, "Content-Type", 12)
            || ((line[0] == 32 || line[0] == 9) && in_content))
      {
        char boundary[128];
        in_content = 1;
        if (!_ds_extract_boundary(boundary, sizeof(boundary), line)) {
          if (!_ds_match_boundary (boundaries, boundary)) {
            _ds_push_boundary (boundaries, boundary);
            free(current_block->boundary);
            current_block->boundary = strdup (boundary);
          }
        } else {
          _ds_push_boundary (boundaries, "");
        }
      } else {
        in_content = 0;
      }

      /* Multipart boundary was reached; move onto next block */

      if (_ds_match_boundary (boundaries, line))
      {

        /* Add the boundary as the terminating boundary */

        current_block->terminating_boundary = strdup (line + 2);
        current_block->original_encoding = current_block->encoding;

        _ds_decode_headers(current_block);
        current_block = _ds_create_message_part ();

        if (!current_block)
          goto MEMFAIL;

        if (nt_add (out->components, (void *) current_block) == NULL)
          goto MEMFAIL;

        block_position = BP_HEADER;
      }

      /* Plain old message (or part) body */

      else {
        buffer_cat (current_block->body, line);

        /* Don't add extra \n at the end of message's body */

        if (in != NULL)
          buffer_cat (current_block->body, "\n");
      }
    }

    line = strsep (&in, "\n");
  } /* while (line) */

  _ds_decode_headers(current_block);

  free (m_in);
  nt_destroy (boundaries);
  return out;

MEMFAIL:
  free(m_in);
  nt_destroy (boundaries);
  _ds_destroy_message(out);
  LOG (LOG_CRIT, ERR_MEM_ALLOC);
  return NULL;
}

/*
 * _ds_create_message_part
 *
 * DESCRIPTION
 *   create and initialize a new message block component
 *
 * RETURN VALUES
 *   pointer to an allocated message block (ds_message_part_t), NULL on failure
 *
 */

ds_message_part_t
_ds_create_message_part (void)
{
  ds_message_part_t block = 
    (ds_message_part_t) calloc (1, sizeof (struct _ds_message_part));

  if (!block) 
    goto MEMFAIL;

  block->headers = nt_create (NT_PTR);
  if (!block->headers) 
    goto MEMFAIL;

  block->body = buffer_create (NULL);
  if (!block->body)
    goto MEMFAIL;

  block->encoding   = EN_UNKNOWN;
  block->media_type = MT_TEXT;
  block->media_subtype     = MST_PLAIN;
  block->original_encoding = EN_UNKNOWN;
  block->content_disposition = PCD_UNKNOWN;

  /* Not really necessary, but.. */

  block->boundary = NULL;
  block->terminating_boundary = NULL;
  block->original_signed_body = NULL;


  return block;

MEMFAIL:
  if (block) {
    buffer_destroy(block->body);
    nt_destroy(block->headers);
    free(block);
  }
  LOG (LOG_CRIT, ERR_MEM_ALLOC);
  return NULL;
}

/*
 * _ds_create_header_field(const char *heading)
 *
 * DESCRIPTION
 *   create and initialize a new header structure
 *
 * INPUT ARGUMENTS
 *      heading    plain text heading (e.g. "To: Mom")
 *
 * RETURN VALUES
 *   pointer to an allocated header structure (ds_header_t), NULL on failure
 */

ds_header_t
_ds_create_header_field (const char *heading)
{
  char *in = strdup(heading);
  char *ptr, *m = in, *data;
  ds_header_t header =
    (ds_header_t) calloc (1, sizeof (struct _ds_header_field));

  if (!header || !in) 
    goto MEMFAIL;

  ptr = strsep (&in, ":");
  if (ptr) {
    header->heading = strdup (ptr);
    if (!header->heading)
      goto MEMFAIL;
    else
    {
      if (!in)
      {
        LOGDEBUG("%s:%u: unexpected data: header string '%s' doesn't "
                 "contains `:' character", __FILE__, __LINE__, header->heading);

        /* Use empty string as data as fallback for comtinue processing. */

        in = "";
      }
      else
      {
        /* Skip white space */
        while (*in == 32 || *in == 9) 
          ++in; 
      }

      data = strdup (in);
      if (!data)
        goto MEMFAIL;

      header->data = data;
      header->concatenated_data = strdup(data);
    }
  }

  free (m);
  return header;

MEMFAIL:
  free(header);
  free(m);
  LOG (LOG_CRIT, ERR_MEM_ALLOC);
  return NULL;
}

/*
 * _ds_decode_headers (ds_message_part_t block)
 *
 * DESCRIPTION
 *   decodes in-line encoded headers
 *
 * RETURN VALUES
 *   returns 0 on success
 */

int
_ds_decode_headers (ds_message_part_t block) {
  char *ptr, *dptr, *rest, *enc;
  ds_header_t header;
  struct nt_node *node_nt;
  struct nt_c c_nt;
  long decoded_len;

  node_nt = c_nt_first(block->headers, &c_nt);
  while(node_nt != NULL) {
    long enc_offset;
    header = (ds_header_t) node_nt->ptr;

    for(enc_offset = 0; header->concatenated_data[enc_offset]; enc_offset++)
    {
      enc = header->concatenated_data + enc_offset;

      if (!strncmp(enc, "=?", 2)) {
        int was_null = 0;
        char *ptrptr, *decoded = NULL;
        long offset = (long) enc - (long) header->concatenated_data;

        if (header->original_data == NULL) {
          header->original_data = strdup(header->data);
          was_null = 1;
        }

        ptr = strtok_r (enc, "?", &ptrptr);
        ptr = strtok_r (NULL, "?", &ptrptr);
        ptr = strtok_r (NULL, "?", &ptrptr);
        dptr = strtok_r (NULL, "?", &ptrptr);
        if (!dptr) {
          if (was_null) 
            header->original_data = NULL;
          continue;
        }

        rest = dptr + strlen (dptr) + 1;
        if (rest[0]!=0)
          rest++;

        if (ptr != NULL && (ptr[0] == 'b' || ptr[0] == 'B'))
          decoded = _ds_decode_base64 (dptr);
        else if (ptr != NULL && (ptr[0] == 'q' || ptr[0] == 'Q')) 
          decoded = _ds_decode_quoted (dptr);

        decoded_len = 0;

        /* Append the rest of the message */

        if (decoded)
        {
          char *new_alloc;

          decoded_len = strlen(decoded);
          new_alloc = calloc (1, offset + decoded_len + strlen (rest) + 2);
          if (new_alloc == NULL) {
            LOG (LOG_CRIT, ERR_MEM_ALLOC);
          }
          else
          {
            if (offset)
              strncpy(new_alloc, header->concatenated_data, offset);

            strcat(new_alloc, decoded);
            strcat(new_alloc, rest);
            free(decoded); 
            decoded = new_alloc;
          }
        }

        if (decoded) { 
          enc_offset += (decoded_len-1);
          free(header->concatenated_data);
          header->concatenated_data = decoded;
        }
        else if (was_null) {
          header->original_data = NULL;
        }
      }
    }

    if (header->original_data != NULL) {
      free(header->data);
      header->data = strdup(header->concatenated_data);
    }

    node_nt = c_nt_next(block->headers, &c_nt);
  }

  return 0;
}

/*
 *  _ds_analyze_header (ds_message_part_t block, ds_header_t header,
 *                      struct nt *boundaries)
 *
 * DESCRIPTION
 *   analyzes the header passed in and performs various operations including:
 *     - setting media type and subtype
 *     - setting transfer encoding
 *     - adding newly discovered boundaries
 *
 *   based on the heading specified. essentially all headers should be
 *   analyzed for future expansion
 *
 * INPUT ARGUMENTS
 *      block		the message block to which the header belongs
 *      header		the header to analyze
 *      boundaries	a list of known boundaries found within the block 
 */

void
_ds_analyze_header (
  ds_message_part_t block, 
  ds_header_t header,
  struct nt *boundaries)
{
  if (!header || !block || !header->data)
    return;

  /* Content-Type header */

  if (!strcasecmp (header->heading, "Content-Type"))
  {
    int len = strlen(header->data);
    if (!strncasecmp (header->data, "text", 4)) {
      block->media_type = MT_TEXT;
      if (len >= 5 && !strncasecmp (header->data + 5, "plain", 5))
        block->media_subtype = MST_PLAIN;
      else if (len >= 5 && !strncasecmp (header->data + 5, "html", 4))
        block->media_subtype = MST_HTML;
      else
        block->media_subtype = MST_OTHER;
    }

    else if (!strncasecmp (header->data, "application", 11))
    {
      block->media_type = MT_APPLICATION;
      if (len >= 12 && !strncasecmp (header->data + 12, "dspam-signature", 15))
        block->media_subtype = MST_DSPAM_SIGNATURE;
      else
        block->media_subtype = MST_OTHER;
    }

    else if (!strncasecmp (header->data, "message", 7))
    {
      block->media_type = MT_MESSAGE;
      if (len >= 8 && !strncasecmp (header->data + 8, "rfc822", 6))
        block->media_subtype = MST_RFC822;
      else if (len >= 8 && !strncasecmp (header->data + 8, "inoculation", 11))
        block->media_subtype = MST_INOCULATION;
      else
        block->media_subtype = MST_OTHER;
    }

    else if (!strncasecmp (header->data, "multipart", 9))
    {
      char boundary[128];

      block->media_type = MT_MULTIPART;
      if (len >= 10 && !strncasecmp (header->data + 10, "mixed", 5))
        block->media_subtype = MST_MIXED;
      else if (len >= 10 && !strncasecmp (header->data + 10, "alternative", 11))
        block->media_subtype = MST_ALTERNATIVE;
      else if (len >= 10 && !strncasecmp (header->data + 10, "signed", 6))
        block->media_subtype = MST_SIGNED;
      else if (len >= 10 && !strncasecmp (header->data + 10, "encrypted", 9))
        block->media_subtype = MST_ENCRYPTED;
      else
        block->media_subtype = MST_OTHER;

      if (!_ds_extract_boundary(boundary, sizeof(boundary), header->data)) {
        if (!_ds_match_boundary (boundaries, boundary)) {
          _ds_push_boundary (boundaries, boundary);
          free(block->boundary);
          block->boundary = strdup (boundary);
        }
      } else {
        _ds_push_boundary (boundaries, "");
      }
    }
    else {
      block->media_type = MT_OTHER;
      block->media_subtype = MST_OTHER;
    }

  }

  /* Content-Transfer-Encoding */

  else if (!strcasecmp (header->heading, "Content-Transfer-Encoding"))
  {
    if (!strncasecmp (header->data, "7bit", 4))
      block->encoding = EN_7BIT;
    else if (!strncasecmp (header->data, "8bit", 4))
      block->encoding = EN_8BIT;
    else if (!strncasecmp (header->data, "quoted-printable", 16))
      block->encoding = EN_QUOTED_PRINTABLE;
    else if (!strncasecmp (header->data, "base64", 6))
      block->encoding = EN_BASE64;
    else if (!strncasecmp (header->data, "binary", 6))
      block->encoding = EN_BINARY;
    else
      block->encoding = EN_OTHER;
  }

  if (!strcasecmp (header->heading, "Content-Disposition"))
  {
    if (!strncasecmp (header->data, "inline", 6)) 
      block->content_disposition = PCD_INLINE;
    else if (!strncasecmp (header->data, "attachment", 10))
      block->content_disposition = PCD_ATTACHMENT;
    else
      block->content_disposition = PCD_OTHER;
  }

  return;
}

/*
 * _ds_destroy_message (ds_message_t message)
 *
 * DESCRIPTION
 *   destroys a message structure (ds_message_t)
 *
 * INPUT ARGUMENTS
 *      message    the message structure to be destroyed
 */

void
_ds_destroy_message (ds_message_t message)
{
  struct nt_node *node_nt;
  struct nt_c c;
  int i = 0;

  if (message == NULL)
    return;

  if (message->components) {
    node_nt = c_nt_first (message->components, &c);
    while (node_nt != NULL)
    {
      ds_message_part_t block = (ds_message_part_t) node_nt->ptr;
      _ds_destroy_block(block);
      node_nt = c_nt_next (message->components, &c);
      i++;
    }
    nt_destroy (message->components);
  }
  free (message);
  return;
}

/*
 * _ds_destroy_headers (ds_message_part_t block)
 *
 * DESCRIPTION
 *   destroys a message block's header pairs
 *   does not free the structures themselves; these are freed at nt_destroy
 *
 * INPUT ARGUMENTS
 *      block    the message block containing the headers to destsroy
 */

void
_ds_destroy_headers (ds_message_part_t block)
{
  struct nt_node *node_nt;
  struct nt_c c;

  if (!block || !block->headers)
    return;

  node_nt = c_nt_first (block->headers, &c);
  while (node_nt != NULL)
  {
    ds_header_t field = (ds_header_t) node_nt->ptr;

    if (field)
    {
      free (field->original_data);
      free (field->heading);
      free (field->concatenated_data);
      free (field->data);
    }
    node_nt = c_nt_next (block->headers, &c);
  }

  return;
}

/*
 * _ds_destroy_block (ds_message_part_t block)
 *
 * DESCRIPTION
 *   destroys a message block
 *
 * INPUT ARGUMENTS
 *   block   the message block to destroy
 */

void
_ds_destroy_block (ds_message_part_t block)
{
  if (!block)
    return;

  if (block->headers) 
  {
    _ds_destroy_headers (block);
    nt_destroy (block->headers);
  }
  buffer_destroy (block->body);
  buffer_destroy (block->original_signed_body);
  free (block->boundary);
  free (block->terminating_boundary);
//  free (block);
  return;
}

/*
 * _ds_decode_block (ds_message_part_t block)
 *   
 * DESCRIPTION
 *   decodes a message block
 * 
 * INPUT ARGUMENTS
 *   block   the message block to decode
 *
 * RETURN VALUES
 *   a pointer to the allocated character array containing the decoded message 
 *   NULL on failure
 */

char *
_ds_decode_block (ds_message_part_t block)
{
  if (block->encoding == EN_BASE64)
    return _ds_decode_base64 (block->body->data);
  else if (block->encoding == EN_QUOTED_PRINTABLE)
    return _ds_decode_quoted (block->body->data);

  LOG (LOG_WARNING, "decoding of block encoding type %d not supported",
       block->encoding);
  return NULL;
}

/*
 * _ds_decode_{base64,quoted}
 *
 * DESCRIPTION
 *   supporting block decoder functions
 *   these function call (or perform) specific decoding functions
 * 
 * INPUT ARGUMENTS
 *   body	encoded message body
 *
 * RETURN VALUES
 *   a pointer to the allocated character array containing the decoded body
 */

#ifndef NCORE
char *
_ds_decode_base64 (const char *body)
{
  if (body == NULL)
    return NULL;

  return base64decode (body);
}

char *
_ds_decode_quoted (const char *body)
{
  char *out, *x;
  char hex[3];
  int val;
  size_t len;

  if (!body)
    return NULL;

  out = strdup (body);
  if (!out)
  {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }
  len = strlen(out) + 1;

  hex[2] = 0;
  x = strchr (out, '=');
  while (x != NULL)
  {
    hex[0] = x[1];
    hex[1] = x[2];
    if (x[1] == '\n')
    {
      memmove(x, x+2, len-((x+2)-out)); //strlen(x+2)+1);
      len -= 2;
      x = strchr (x, '=');
    }
    else
    {
      if (((hex[0] >= 'A' && hex[0] <= 'F')
           || (hex[0] >= 'a' && hex[0] <= 'f')
           || (hex[0] >= '0' && hex[0] <= '9'))
          && ((hex[1] >= 'A' && hex[1] <= 'F')
              || (hex[1] >= 'a' && hex[1] <= 'f')
              || (hex[1] >= '0' && hex[1] <= '9')))
      {
        val = (int) strtol (hex, NULL, 16);
        if (val) {
          x[0] = val;
          memmove(x+1, x+3, len-((x+3)-out)); //strlen(x+3)+1);
          len -= 2;
        }
      }
      x = strchr (x + 1, '=');
    }
  }

  return out;
}
#endif /* NCORE */

/*
 * _ds_encode_block (ds_message_part_t block, int encoding)
 *   
 * DESCRIPTION
 *   encodes a message block using the encoding specified and replaces the
 *   block's message body with the encoded data
 * 
 * INPUT ARGUMENTS
 *      block       the message block to encode
 *      encoding    encoding to use (EN_)
 *
 * RETURN VALUES
 *    returns 0 on success
 */

int
_ds_encode_block (ds_message_part_t block, int encoding)
{
  /* we can't encode a block with the same encoding */

  if (block->encoding == encoding)
    return EINVAL;

  /* we can't encode a block that's already encoded */

  if (block->encoding == EN_BASE64 || block->encoding == EN_QUOTED_PRINTABLE)
    return EFAILURE;

  if (encoding == EN_BASE64) {
    char *encoded = _ds_encode_base64 (block->body->data);
    buffer_destroy (block->body);
    block->body = buffer_create (encoded);
    free (encoded);
    block->encoding = EN_BASE64;
  }
  else if (encoding == EN_QUOTED_PRINTABLE) {

    /* TODO */

    return 0;
  }

  LOGDEBUG("unsupported encoding: %d", encoding);
  return 0;
}

/*
 * _ds_encode_{base64,quoted}
 *
 * DESCRIPTION
 *   supporting block encoder functions
 *   these function call (or perform) specific encoding functions
 *
 * INPUT ARGUMENTS
 *   body        decoded message body
 *
 * RETURN VALUES
 *   a pointer to the allocated character array containing the encoded body
 */

char *
_ds_encode_base64 (const char *body)
{
  return base64encode (body);
}

/*
 * _ds_assemble_message (ds_message_t message)
 *
 * DESCRIPTION
 *   assembles a message structure into a flat text message
 *
 * INPUT ARGUMENTS
 *      message    the message structure (ds_message_t) to assemble
 *
 * RETURN VALUES
 *   a pointer to the allocated character array containing the text message
 */

char *
_ds_assemble_message (ds_message_t message)
{
  buffer *out = buffer_create (NULL);
  struct nt_node *node_nt, *node_header;
  struct nt_c c_nt, c_nt2;
  char *heading;
  char *copyback;
  int i = 0;

  if (!out) {
    LOG (LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  node_nt = c_nt_first (message->components, &c_nt);
  while (node_nt != NULL && node_nt->ptr != NULL)
  {
    ds_message_part_t block =
      (ds_message_part_t) node_nt->ptr;
#ifdef VERBOSE
    LOGDEBUG ("assembling component %d", i);
#endif

    /* Assemble headers */

    if (block->headers != NULL && block->headers->items > 0)
    {
      node_header = c_nt_first (block->headers, &c_nt2);
      while (node_header != NULL)
      {
        char *data;
        ds_header_t current_header =
          (ds_header_t) node_header->ptr;

        data = (current_header->original_data == NULL) ? current_header->data :
               current_header->original_data;

        heading = malloc(
            ((current_header->heading) ? strlen(current_header->heading) : 0) 
          + ((data) ? strlen(data) : 0)
          + 4);

        if (current_header->heading != NULL &&
            (!strncmp (current_header->heading, "From ", 5) || 
             !strncmp (current_header->heading, "--", 2)))
          sprintf (heading, "%s:%s\n", 
            (current_header->heading) ? current_header->heading : "",
            (data) ? data: "");
        else
          sprintf (heading, "%s: %s\n",
            (current_header->heading) ? current_header->heading : "",
            (data) ? data : "");

        buffer_cat (out, heading);
        free(heading);
        node_header = c_nt_next (block->headers, &c_nt2);
      }
    }

    buffer_cat (out, "\n");

    /* Assemble bodies */

    if (block->original_signed_body != NULL && message->protect)
      buffer_cat (out, block->original_signed_body->data);
    else
      buffer_cat (out, block->body->data);

    if (block->terminating_boundary != NULL)
    {
      buffer_cat (out, "--");
      buffer_cat (out, block->terminating_boundary);
    }
   
    node_nt = c_nt_next (message->components, &c_nt);
    i++;

    if (node_nt != NULL && node_nt->ptr != NULL)
      buffer_cat (out, "\n");
  }

  copyback = out->data;
  out->data = NULL;
  buffer_destroy (out);
  return copyback;
}

/*
 * _ds_{push,pop,match,extract}_boundary 
 *
 * DESCRIPTION
 *   these functions maintain and service a boundary "stack" on the message
 */

int
_ds_push_boundary (struct nt *stack, const char *boundary)
{
  char *y;

  if (boundary == NULL || boundary[0] == 0)
    return EINVAL;

  y = malloc (strlen (boundary) + 3);
  if (y == NULL)
    return EUNKNOWN;

  sprintf (y, "--%s", boundary);
  nt_add (stack, (char *) y);
  free(y);

  return 0;
}

char *
_ds_pop_boundary (struct nt *stack)
{
  struct nt_node *node, *last_node = NULL, *parent_node = NULL;
  struct nt_c c;
  char *boundary = NULL;
                                                                                
  node = c_nt_first (stack, &c);
  while (node != NULL)
  {
    parent_node = last_node;
    last_node = node;
    node = c_nt_next (stack, &c);
  }
  if (parent_node != NULL)
    parent_node->next = NULL;
  else
    stack->first = NULL;
                                                                                
  if (last_node == NULL)
    return NULL;
                                                                                
  boundary = strdup (last_node->ptr);
                                                                                
  free (last_node->ptr);
  free (last_node);
                                                                                
  return boundary;
}
                                                                                
int
_ds_match_boundary (struct nt *stack, const char *buff)
{
  struct nt_node *node;
  struct nt_c c;
                                                                                
  node = c_nt_first (stack, &c);
  while (node != NULL)
  {
    if (!strncmp (buff, node->ptr, strlen (node->ptr)))
    {
      return 1;
    }
    node = c_nt_next (stack, &c);
  }
  return 0;
}

int
_ds_extract_boundary (char *buf, size_t size, char *mem)
{
  char *data, *ptr, *ptrptr;

  if (mem == NULL)
    return EINVAL;

  data = strdup(mem);
  if (data == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  for(ptr=data;ptr<(data+strlen(data));ptr++) {
    if (!strncasecmp(ptr, "boundary", 8)) {
      ptr = strchr(ptr, '=');
      if (ptr == NULL) {
        free(data);
        return EFAILURE;
      }
      ptr++;
      while(isspace((int) ptr[0]))
        ptr++;
      if (ptr[0] == '"')
        ptr++;
      strtok_r(ptr, " \";\n\t", &ptrptr);
      strlcpy(buf, ptr, size);
      free(data);
      return 0;
    }
  }

  free(data);
  return EFAILURE;
}

/*
 * _ds_find_header (ds_message_t message, consr char *heading, int flags) {
 *
 * DESCRIPTION
 *   finds a header and returns its value
 *
 * INPUT ARGUMENTS
 *   message     the message structure to search
 *   heading	the heading to search for 	
 *   flags	optional search flags
 *
 * FLAGS
 *   DDF_ICASE	case insensitive search
 *
 * RETURN VALUES
 *   a pointer to the header structure's value
 *
 */

char *
_ds_find_header (ds_message_t message, const char *heading, int flags) {
  ds_message_part_t block;
  ds_header_t head;
  struct nt_node *node_nt;

  if (message->components->first) {
    if ((block = message->components->first->ptr)==NULL)
      return NULL;
    if (block->headers == NULL)
      return NULL;
  } else {
    return NULL;
  }

  node_nt = block->headers->first;
  while(node_nt != NULL) {
    head = (ds_header_t) node_nt->ptr;
    if (flags & DDF_ICASE) {
      if (head && !strcasecmp(head->heading, heading))
        return head->data;
    } else {
      if (head && !strcmp(head->heading, heading))
        return head->data;
    }
    node_nt = node_nt->next;
  }

  return NULL;
}

