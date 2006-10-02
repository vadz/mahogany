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

/* cssstat.c - Print hash file statistics */

#define READ_ATTRIB(A)		_ds_read_attribute(agent_config, A)
#define MATCH_ATTRIB(A, B)	_ds_match_attribute(agent_config, A, B)

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

int DO_DEBUG 
#ifdef DEBUG
= 1
#else
= 0
#endif
;

#include "read_config.h"
#include "hash_drv.h"
#include "language.h"
#include "error.h"
 
#define SYNTAX "syntax: cssstat [filename]"

int cssstat(const char *filename);

int main(int argc, char *argv[]) {
  char *filename;
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

  filename = argv[1];

  r = cssstat(filename);
  
  if (r) {
    fprintf(stderr, "cssstat failed on error %d\n", r);
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int cssstat(const char *filename) {
  struct _hash_drv_map map;
  hash_drv_header_t header;
  hash_drv_spam_record_t rec;
  unsigned long filepos = sizeof(struct _hash_drv_header);
  unsigned long nfree = 0, nused = 0;
  unsigned long efree, eused;
  unsigned long extents = 0;
  unsigned long i;

  unsigned long hash_rec_max = HASH_REC_MAX;
  unsigned long max_seek     = HASH_SEEK_MAX;
  unsigned long max_extents  = 0;
  unsigned long extent_size  = HASH_EXTENT_MAX;
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

  if (_hash_drv_open(filename, &map, 0, max_seek, 
                     max_extents, extent_size, 0, flags))
  {
    return EFAILURE;
  }

  header = map.addr;
  printf("filename %s length %ld\n", filename, (long) map.file_len);

  while(filepos < map.file_len) {
    printf("extent %lu: record length %lu\n", extents, 
      (unsigned long) header->hash_rec_max);
    efree = eused = 0;
    for(i=0;i<header->hash_rec_max;i++) {
      rec = map.addr+filepos;
      if (rec->hashcode) {
        eused++; 
        nused++;
      } else {
        efree++;
        nfree++;
      }
      filepos += sizeof(struct _hash_drv_spam_record);
    }
    header = map.addr + filepos;
    filepos += sizeof(struct _hash_drv_header);
    extents++;

    printf("\textent records used %lu\n", eused);
    printf("\textent records free %lu\n", efree);
  }

  _hash_drv_close(&map);

  printf("total database records used %lu\n", nused);
  printf("total database records free %lu\n", nfree);
  printf("total extents               %lu\n", extents);
  return 0;
}

