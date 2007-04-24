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

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "nodetree.h"
#include "buffer.h"

#ifndef _DECODE_H
#define _DECODE_H

/*
 * _ds_header_field
 *
 * DESCRIPTION
 *   a single header/value paid from a message block
 */

typedef struct _ds_header_field
{
  char *heading;
  char *data;
  char *original_data;		/* prior to decoding */
  char *concatenated_data;	/* multi-line */
} *ds_header_t;

/*
 * _ds_message_part
 * 
 * DESCRIPTION
 *   a message block (or part) within a message. in a single-part message, 
 *   there will be only one block (block 0). in a multipart message, each part 
 *   will be separated into a separte block. the message block consists of:
 *    - a dynamic array of headers (nodetree of ds_header_t's) for the block
 *    - body data (NULL if there is no body)
 *    - block encoding
 *    - block media type information
 *    - boundary and terminating boundary information
 */

typedef struct _ds_message_part
{
  struct nt *	headers;
  buffer *	body;
  buffer *	original_signed_body;
  char *	boundary;
  char *	terminating_boundary;
  int		encoding;
  int 		original_encoding;
  int 		media_type;
  int 		media_subtype;
  int           content_disposition;
} *ds_message_part_t;

/*
 * _ds_message
 *
 * DESCRIPTION
 *   the actual message structure, comprised of an array of message blocks.
 *   in a non-multipart email, there will only be one message block (block 0).
 *   in multipart emails, however, the first message block will represent the 
 *   header (with a NULL body_data or something like "This is a multi-part 
 *   message"), and each additional block within the email will be given its 
 *   own message_part structure with its own headers, boundary, etc.
 *
 *   embedded multipart messages are not realized by the structure, but can
 *   be identified by examining the media type or headers.
 */

typedef struct _ds_message
{
  struct nt *	components; 
  int protect;
} *ds_message_t;

/* adapter dependent functions */

#ifndef NCORE
char *  _ds_decode_base64       (const char *body);
char *  _ds_decode_quoted       (const char *body);
#endif

/* Adapter-independent functions */

ds_message_t _ds_actualize_message (const char *message);

char *  _ds_assemble_message (ds_message_t message, const char *newline);
char *  _ds_find_header (ds_message_t message, const char *heading, int flags);

ds_message_part_t _ds_create_message_part (void);
ds_header_t        _ds_create_header_field  (const char *heading);
void               _ds_analyze_header
  (ds_message_part_t block, ds_header_t  header, struct nt *boundaries);

void _ds_destroy_message	(ds_message_t message);
void _ds_destroy_headers	(ds_message_part_t block);
void _ds_destroy_block		(ds_message_part_t block);

char *	_ds_decode_block	(ds_message_part_t block);
int	_ds_encode_block	(ds_message_part_t block, int encoding);
char *	_ds_encode_base64	(const char *body);
char *	_ds_encode_quoted	(const char *body);
int     _ds_decode_headers      (ds_message_part_t block);

int	_ds_push_boundary	(struct nt *stack, const char *boundary);
int	_ds_match_boundary	(struct nt *stack, const char *buff);
int     _ds_extract_boundary    (char *buf, size_t size, char *data);
char *	_ds_pop_boundary	(struct nt *stack);

/* Encoding values */

#define EN_7BIT			0x00
#define EN_8BIT 		0x01
#define EN_QUOTED_PRINTABLE	0x02
#define EN_BASE64		0x03
#define EN_BINARY		0x04
#define EN_UNKNOWN		0xFE
#define EN_OTHER		0xFF

/* Media types which are relevant to DSPAM */

#define MT_TEXT			0x00
#define MT_MULTIPART		0x01
#define MT_MESSAGE		0x02
#define MT_APPLICATION		0x03
#define MT_UNKNOWN		0xFE
#define MT_OTHER		0xFF

/* Media subtypes which are relevant to DSPAM */

#define MST_PLAIN		0x00
#define	MST_HTML		0x01
#define MST_MIXED		0x02
#define MST_ALTERNATIVE		0x03
#define MST_RFC822		0x04
#define MST_DSPAM_SIGNATURE	0x05
#define MST_SIGNED		0x06
#define MST_INOCULATION		0x07
#define MST_ENCRYPTED		0x08
#define MST_UNKNOWN		0xFE
#define MST_OTHER		0xFF

/* Part Content-Dispositions */

#define PCD_INLINE		0x00
#define PCD_ATTACHMENT		0x01
#define PCD_UNKNOWN		0xFE
#define PCD_OTHER		0xFF

/* Block position; used when analyzing a message */

#define BP_HEADER		0x00
#define BP_BODY			0x01

/* Decoding function flags */

#define DDF_ICASE		0x01

#endif /* _DECODE_H */

