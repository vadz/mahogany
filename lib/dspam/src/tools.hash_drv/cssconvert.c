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

/* cssconvert.c - convert a v3.6.0 hash database to v3.6.1 (which includes
 * 8-byte alignment for 64-bit systems */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

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

#define READ_ATTRIB(A)          _ds_read_attribute(agent_config, A)
#define MATCH_ATTRIB(A, B)      _ds_match_attribute(agent_config, A, B)

int DO_DEBUG 
#ifdef DEBUG
= 1
#else
= 0
#endif
;

#include "read_config.h"
#include "hash_drv.h"
#include "error.h"
#include "language.h"
 
#define SYNTAX "syntax: cssconvert [filename]" 

typedef struct _old_hash_drv_header
{
  unsigned long hash_rec_max;
  struct _ds_spam_totals totals;
} *old_hash_drv_header_t;

int cssconvert(const char *filename);

int main(int argc, char *argv[]) {
  int r;

  if (argc<2) {
    fprintf(stderr, "%s\n", SYNTAX);
    exit(EXIT_FAILURE);
  }

  agent_config = read_config(NULL);
  if (!agent_config) {
    LOG(LOG_ERR, ERR_AGENT_READ_CONFIG);
    exit(EXIT_FAILURE);
  }

  r = cssconvert(argv[1]);
  
  if (r) {
    fprintf(stderr, "cssconvert failed on error %d\n", r);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int cssconvert(const char *filename) {
  int i;
  hash_drv_header_t header;
  void *offset;
  struct _hash_drv_map old, new;
  hash_drv_spam_record_t rec;
  unsigned long filepos;
  char newfile[128];

  unsigned long hash_rec_max = HASH_REC_MAX;
  unsigned long max_seek     = HASH_SEEK_MAX;
  unsigned long max_extents  = 0;
  unsigned long extent_size  = HASH_EXTENT_MAX;
  int pctincrease = 0;
  int flags = 0;

  if (READ_ATTRIB("HashRecMax"))
    hash_rec_max = strtol(READ_ATTRIB("HashRecMax"), NULL, 0);

  if (READ_ATTRIB("HashExtentSize"))
    extent_size = strtol(READ_ATTRIB("HashExtentSize"), NULL, 0);

  if (READ_ATTRIB("HashMaxExtents"))
    max_extents = strtol(READ_ATTRIB("HashMaxExtents"), NULL, 0);

  if (MATCH_ATTRIB("HashAutoExtend", "on"))
    flags = HMAP_AUTOEXTEND;

  if (READ_ATTRIB("HashMaxSeek"))
     max_seek = strtol(READ_ATTRIB("HashMaxSeek"), NULL, 0);

  if (READ_ATTRIB("HashPctIncrease")) {
    pctincrease = atoi(READ_ATTRIB("HashPctIncrease"));
    if (pctincrease > 100) {
        LOG(LOG_ERR, "HashPctIncrease out of range; ignoring");
        pctincrease = 0;
    }
  }

  snprintf(newfile, sizeof(newfile), "/tmp/%u.css", (unsigned int) getpid());

  if (_hash_drv_open(filename, &old, 0, max_seek,
                     max_extents, extent_size, pctincrease, flags))
  {
    return EFAILURE;
  }

  if (_hash_drv_open(newfile, &new, hash_rec_max, max_seek,
                     max_extents, extent_size, pctincrease, flags))
  {
    _hash_drv_close(&old);
    return EFAILURE;
  }

  filepos = sizeof(struct _old_hash_drv_header);
  header = old.addr;
  while(filepos < old.file_len) {
    for(i=0;i<header->hash_rec_max;i++) {
      rec = old.addr+filepos;
      if (_hash_drv_set_spamrecord(&new, rec, 0)) {
        LOG(LOG_WARNING, "aborting on error");
        _hash_drv_close(&new);
        _hash_drv_close(&old);
        unlink(newfile);
        return EFAILURE;
      }
      filepos += sizeof(struct _hash_drv_spam_record);
    }
    offset = old.addr + filepos;
    header = offset;
    filepos += sizeof(struct _old_hash_drv_header);
  }

  _hash_drv_close(&new);
  _hash_drv_close(&old);
  rename(newfile, filename);
  return 0;
}

