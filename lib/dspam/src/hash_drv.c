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
 * hash_drv.c - hash-based storage driver 
 *              mmap'd flat-file storage for fast storage
 *              inspired by crm114 sparse spectra algorithm 
 *
 * DESCRIPTION
 *   This driver uses a random access file for storage. It is exceptionally fast
 *   and does not require any third-party dependencies. The auto-extend
 *   functionality allows the file to grow as needed.
 */

#define READ_ATTRIB(A)	   _ds_read_attribute(CTX->config->attributes, A)
#define MATCH_ATTRIB(A, B) _ds_match_attribute(CTX->config->attributes, A, B)

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

#include "storage_driver.h"
#include "config_shared.h"
#include "hash_drv.h"
#include "libdspam.h"
#include "config.h"
#include "error.h"
#include "language.h"
#include "util.h"

int
dspam_init_driver (DRIVER_CTX *DTX)
{
  DSPAM_CTX *CTX;
  char *HashConcurrentUser;
#ifdef DAEMON
   unsigned long connection_cache = 1;
#endif

  if (DTX == NULL) 
    return 0;

  CTX = DTX->CTX;
  HashConcurrentUser = READ_ATTRIB("HashConcurrentUser");

#ifdef DAEMON

  /*
   *  Stateful concurrent hash databases are preloaded into memory and
   *  shared using a reader-writer lock. At the present moment, only a single
   *  user can be loaded into any instance of the daemon, so it is only useful
   *  if you are running with a system-wide filtering user. 
   */

  if (DTX->flags & DRF_STATEFUL) {
    char filename[MAX_FILENAME_LENGTH];
    hash_drv_map_t map;
    unsigned long hash_rec_max = HASH_REC_MAX;
    unsigned long max_seek     = HASH_SEEK_MAX;
    unsigned long max_extents  = 0;
    unsigned long extent_size  = HASH_EXTENT_MAX;
    int pctincrease = 0;
    int flags = HMAP_AUTOEXTEND;
    int ret, i;

    if (READ_ATTRIB("HashConnectionCache") && !HashConcurrentUser)
      connection_cache = strtol(READ_ATTRIB("HashConnectionCache"), NULL, 0);

    DTX->connection_cache = connection_cache;

    if (READ_ATTRIB("HashRecMax"))
      hash_rec_max = strtol(READ_ATTRIB("HashRecMax"), NULL, 0);

    if (READ_ATTRIB("HashExtentSize"))
      extent_size = strtol(READ_ATTRIB("HashExtentSize"), NULL, 0);

    if (READ_ATTRIB("HashMaxExtents"))
      max_extents = strtol(READ_ATTRIB("HashMaxExtents"), NULL, 0);

    if (!MATCH_ATTRIB("HashAutoExtend", "on"))
      flags = 0;

    if (READ_ATTRIB("HashPctIncrease")) {
      pctincrease = atoi(READ_ATTRIB("HashPctIncrease"));
      if (pctincrease > 100) {
          LOG(LOG_ERR, "HashPctIncrease out of range; ignoring");
          pctincrease = 0;
      }
    }

    if (READ_ATTRIB("HashMaxSeek"))
       max_seek = strtol(READ_ATTRIB("HashMaxSeek"), NULL, 0);

    /* Connection array (just one single connection for hash_drv) */
    DTX->connections = calloc(1, sizeof(struct _ds_drv_connection *) * connection_cache);
    if (DTX->connections == NULL) 
      goto memerr;

    /* Initialize Connections */
    for(i=0;i<connection_cache;i++) {
      DTX->connections[i] = calloc(1, sizeof(struct _ds_drv_connection));
      if (DTX->connections[i] == NULL) 
        goto memerr;

      /* Our connection's storage structure */
      if (HashConcurrentUser) {
        DTX->connections[i]->dbh = calloc(1, sizeof(struct _hash_drv_map));
        if (DTX->connections[i]->dbh == NULL) 
          goto memerr;
        pthread_rwlock_init(&DTX->connections[i]->rwlock, NULL);
      } else {
        DTX->connections[i]->dbh = NULL;
        pthread_mutex_init(&DTX->connections[i]->lock, NULL);
      }
    }

    /* Load concurrent database into resident memory */
    if (HashConcurrentUser) {
      map = (hash_drv_map_t) DTX->connections[0]->dbh;

      /* Tell the server our connection lock will be reader/writer based */
      if (!(DTX->flags & DRF_RWLOCK))
        DTX->flags |= DRF_RWLOCK;

      _ds_userdir_path(filename, DTX->CTX->home, HashConcurrentUser, "css");
      _ds_prepare_path_for(filename);
      LOGDEBUG("preloading %s into memory via mmap()", filename);
      ret = _hash_drv_open(filename, map, hash_rec_max, 
          max_seek, max_extents, extent_size, pctincrease, flags); 

      if (ret) {
        LOG(LOG_CRIT, "_hash_drv_open(%s) failed on error %d: %s", 
                      filename, ret, strerror(errno)); 
        free(DTX->connections[0]->dbh);
        free(DTX->connections[0]);
        free(DTX->connections);
        return EFAILURE;
      }
    }
  }
#endif

  return 0;

#ifdef DAEMON
memerr:
  if (DTX) {
    if (DTX->connections) {
      int i;
      for(i=0;i<connection_cache;i++) {
        if (DTX->connections[i]) 
          free(DTX->connections[i]->dbh);
        free(DTX->connections[i]);
      }
    }
    free(DTX->connections);
  }
  LOG(LOG_CRIT, ERR_MEM_ALLOC);
  return EUNKNOWN;
#endif
  
}

int
dspam_shutdown_driver (DRIVER_CTX *DTX)
{
#ifdef DAEMON
  DSPAM_CTX *CTX;

  if (DTX && DTX->CTX) {
    char *HashConcurrentUser;
    CTX = DTX->CTX;
    HashConcurrentUser = READ_ATTRIB("HashConcurrentUser");

    if (DTX->flags & DRF_STATEFUL) {
      hash_drv_map_t map;
      int connection_cache = 1, i;

     if (READ_ATTRIB("HashConnectionCache") && !HashConcurrentUser)
        connection_cache = strtol(READ_ATTRIB("HashConnectionCache"), NULL, 0);

      LOGDEBUG("unloading hash database from memory");
      if (DTX->connections) {
        for(i=0;i<connection_cache;i++) {
          LOGDEBUG("unloading connection object %d", i);
          if (DTX->connections[i]) {
            if (!HashConcurrentUser) {
              pthread_mutex_destroy(&DTX->connections[i]->lock);
            }
            else {
              pthread_rwlock_destroy(&DTX->connections[i]->rwlock);
              map = (hash_drv_map_t) DTX->connections[i]->dbh;
              if (map)
                _hash_drv_close(map);
            }
            free(DTX->connections[i]->dbh);
            free(DTX->connections[i]);
          }
        }
        free(DTX->connections);
      }
    }
  }
#endif

  return 0;
}

int
_hash_drv_lock_get (
  DSPAM_CTX *CTX,
  struct _hash_drv_storage *s,
  const char *username)
{
  char filename[MAX_FILENAME_LENGTH];
  int r;

  _ds_userdir_path(filename, CTX->home, username, "lock");
  _ds_prepare_path_for(filename);

  s->lock = fopen(filename, "a");
  if (s->lock == NULL) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, filename, strerror(errno));
    return EFAILURE;
  }
  r = _ds_get_fcntl_lock(fileno(s->lock));
  if (r) {
    fclose(s->lock);
    LOG(LOG_ERR, ERR_IO_LOCK, filename, r, strerror(errno));
  }
  return r;
}

int
_hash_drv_lock_free (
  struct _hash_drv_storage *s, 
  const char *username)
{
  int r;

  if (username == NULL)
    return 0;

  r = _ds_free_fcntl_lock(fileno(s->lock));
  if (!r) {
    fclose(s->lock);
  } else {
    LOG(LOG_ERR, ERR_IO_LOCK_FREE, username, r, strerror(errno));
  }

  return r;
}

int _hash_drv_open(
  const char *filename, 
  hash_drv_map_t map, 
  unsigned long recmaxifnew,
  unsigned long max_seek,
  unsigned long max_extents,
  unsigned long extent_size,
  int pctincrease,
  int flags) 
{
  struct _hash_drv_header header;
  int open_flags = O_RDWR;
  int mmap_flags = PROT_READ + PROT_WRITE;

  map->fd = open(filename, open_flags);

  /*
   *  Create a new hash database if desired. The record count written in the
   *  first segment will be recmaxifnew. Once the file is created, it's then
   *  mmap()'d into memory as usual.
   */

  if (map->fd < 0 && recmaxifnew) {
    FILE *f;
    struct _hash_drv_spam_record rec;
    int i;

    memset(&header, 0, sizeof(struct _hash_drv_header));
    memset(&rec, 0, sizeof(struct _hash_drv_spam_record));

    header.hash_rec_max = recmaxifnew;

    f = fopen(filename, "w");
    if (!f) {
      LOG(LOG_ERR, ERR_IO_FILE_OPEN, filename, strerror(errno));
      return EFILE;
    }

    fwrite(&header, sizeof(struct _hash_drv_header), 1, f);
    for(i=0;i<header.hash_rec_max;i++)
      fwrite(&rec, sizeof(struct _hash_drv_spam_record), 1, f);
    fclose(f);
    map->fd = open(filename, open_flags);
  }

  if (map->fd < 0) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, filename, strerror(errno));
    return EFILE;
  }

  map->header = malloc(sizeof(struct _hash_drv_header));
  if (map->header == NULL) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    close(map->fd);
    map->addr = 0;
    return EFAILURE;
  }

  read(map->fd, map->header, sizeof(struct _hash_drv_header));
  map->file_len = lseek(map->fd, 0, SEEK_END); 

  map->addr = mmap(NULL, map->file_len, mmap_flags, MAP_SHARED, map->fd, 0);
  if (map->addr == MAP_FAILED) {
    free(map->header);
    close(map->fd);
    map->addr = 0;
    return EFAILURE;
  }

  strlcpy(map->filename, filename, MAX_FILENAME_LENGTH);
  map->max_seek    = max_seek;
  map->max_extents = max_extents;
  map->extent_size = extent_size;
  map->pctincrease = pctincrease;
  map->flags       = flags;

  return 0;
}

int
_hash_drv_close(hash_drv_map_t map) {
  struct _hash_drv_header header;
  int r;

  if (!map->addr)
    return EINVAL;

  memcpy(&header, map->header, sizeof(struct _hash_drv_header));

  r = munmap(map->addr, map->file_len);
  if (r) {
    LOG(LOG_WARNING, "munmap failed on error %d: %s", r, strerror(errno));
  }
 
  lseek (map->fd, 0, SEEK_SET);
  write (map->fd, &header, sizeof(struct _hash_drv_header));
  close(map->fd);

  map->addr = 0;
  free(map->header);

  return r;
}


int
_ds_init_storage (DSPAM_CTX * CTX, void *dbh)
{
  struct _hash_drv_storage *s = NULL;
  hash_drv_map_t map = NULL;
  int ret;

  if (CTX == NULL)
    return EINVAL;

  if (!CTX->home) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    return EINVAL;
  }

  if (CTX->flags & DSF_MERGED) {
    LOG(LOG_ERR, ERR_DRV_NO_MERGED);
    return EINVAL;
  }

  if (CTX->storage)
    return EINVAL;

  /* Persistent driver storage */

  s = calloc (1, sizeof (struct _hash_drv_storage));
  if (s == NULL)
  {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  /* If running in HashConcurrentUser mode, use existing hash mapping */

  if (dbh) {
    map = dbh;
    s->dbh_attached = 1;
  } else {
    map = calloc(1, sizeof(struct _hash_drv_map));
    if (!map) {
      LOG(LOG_CRIT, ERR_MEM_ALLOC);
      free(s);
      return EUNKNOWN;
    }
    s->dbh_attached = 0;
  }

  s->map = map;

  /* Mapping defaults */

  s->hash_rec_max = HASH_REC_MAX;
  s->max_seek     = HASH_SEEK_MAX;
  s->max_extents  = 0;
  s->extent_size  = HASH_EXTENT_MAX;
  s->pctincrease  = 0;
  s->flags        = HMAP_AUTOEXTEND;

  if (READ_ATTRIB("HashRecMax"))
    s->hash_rec_max = strtol(READ_ATTRIB("HashRecMax"), NULL, 0);

  if (READ_ATTRIB("HashExtentSize"))
    s->extent_size = strtol(READ_ATTRIB("HashExtentSize"), NULL, 0);

  if (READ_ATTRIB("HashMaxExtents"))
    s->max_extents = strtol(READ_ATTRIB("HashMaxExtents"), NULL, 0);

  if (!MATCH_ATTRIB("HashAutoExtend", "on"))
    s->flags = 0;

  if (READ_ATTRIB("HashPctIncrease")) {
    s->pctincrease = atoi(READ_ATTRIB("HashPctIncrease"));
    if (s->pctincrease > 100) {
        LOG(LOG_ERR, "HashPctIncrease out of range; ignoring");
        s->pctincrease = 0;
    }
  }

  if (READ_ATTRIB("HashMaxSeek"))
    s->max_seek = strtol(READ_ATTRIB("HashMaxSeek"), NULL, 0);

  if (!dbh && CTX->username != NULL)
  {
    char db[MAX_FILENAME_LENGTH];
    int lock_result;

    if (CTX->group == NULL)
      _ds_userdir_path(db, CTX->home, CTX->username, "css");
    else
      _ds_userdir_path(db, CTX->home, CTX->group, "css");

    lock_result = _hash_drv_lock_get (CTX, s, 
      (CTX->group) ? CTX->group : CTX->username);
    if (lock_result < 0) 
      goto BAIL;

    ret = _hash_drv_open(db, s->map, s->hash_rec_max, s->max_seek, 
        s->max_extents, s->extent_size, s->pctincrease, s->flags);

    if (ret) {
      _hash_drv_close(s->map);
      free(s);
      return EFAILURE;
    }
  }

  CTX->storage = s;
  s->dir_handles = nt_create (NT_INDEX);

  if (_hash_drv_get_spamtotals (CTX))
  {
    LOGDEBUG ("unable to load totals.  using zero values.");
    memset (&CTX->totals, 0, sizeof (struct _ds_spam_totals));
  }

  return 0;

BAIL:
  free(s);
  return EFAILURE;
}

int
_ds_shutdown_storage (DSPAM_CTX * CTX)
{
  struct _hash_drv_storage *s;
  struct nt_node *node_nt;
  struct nt_c c_nt;
  int lock_result;

  if (!CTX || !CTX->storage)
    return EINVAL;

  s  = (struct _hash_drv_storage *) CTX->storage;

  /* Close open file handles to directories (iteration functions) */

  node_nt = c_nt_first (s->dir_handles, &c_nt);
  while (node_nt != NULL)
  {
    DIR *dir;
    dir = (DIR *) node_nt->ptr;
    closedir (dir);
    node_nt = c_nt_next (s->dir_handles, &c_nt);
  }
  nt_destroy (s->dir_handles);

  if (CTX->operating_mode != DSM_CLASSIFY)
    _hash_drv_set_spamtotals (CTX);

  /* Close connection to hash database only if we're not concurrent */

  if (!s->dbh_attached) {
    _hash_drv_close(s->map);
    free(s->map);
    lock_result =
      _hash_drv_lock_free (s, (CTX->group) ? CTX->group : CTX->username);
    if (lock_result < 0)
      return EUNKNOWN;
  }

  free (CTX->storage);
  CTX->storage = NULL;

  return 0;
}

int
_hash_drv_get_spamtotals (DSPAM_CTX * CTX)
{
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;

  if (s->map->addr == 0)
    return EINVAL;
  /* Totals are loaded straight from the hash header */
  memcpy(&CTX->totals, &s->map->header->totals, sizeof(struct _ds_spam_totals));
  return 0;
}

int
_hash_drv_set_spamtotals (DSPAM_CTX * CTX)
{
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;

  if (s->map->addr == NULL)
    return EINVAL;
  /* Totals are stored into the hash header */
  memcpy(&s->map->header->totals, &CTX->totals, sizeof(struct _ds_spam_totals));
  return 0;
}

int
_ds_getall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  struct _ds_spam_stat stat;
  struct _ds_spam_stat *p_stat = &stat;
  int ret = 0, x = 0;

  if (diction == NULL || CTX == NULL)
    return EINVAL;

  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    ds_term->s.spam_hits = 0;
    ds_term->s.innocent_hits = 0;
    ds_term->s.offset = 0;
    x = _ds_get_spamrecord (CTX, ds_term->key, p_stat);
    if (!x)
      ds_diction_setstat(diction, ds_term->key, p_stat);
    else if (x != EFAILURE)
      ret = x;

    ds_term = ds_diction_next(ds_c);
  }
  ds_diction_close(ds_c);

  if (ret) {
    LOGDEBUG("_ds_getall_spamtotals returning %d", ret);
  }

  return ret;
}

int
_ds_setall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  ds_term_t ds_term;
  ds_cursor_t ds_c;
  int ret = EUNKNOWN;

  if (diction == NULL || CTX == NULL)
    return EINVAL;

  if (CTX->operating_mode == DSM_CLASSIFY &&
        (CTX->training_mode != DST_TOE ||
          (diction->whitelist_token == 0 && (!(CTX->flags & DSF_NOISE)))))
  {
    return 0;
  }
                                                                                
  ds_c = ds_diction_cursor(diction);
  ds_term = ds_diction_next(ds_c);
  while(ds_term)
  {
    if (!(ds_term->s.status & TST_DIRTY)) {  
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    if (CTX->training_mode == DST_TOE           &&
        CTX->classification == DSR_NONE         &&
        CTX->operating_mode == DSM_CLASSIFY     &&
        diction->whitelist_token != ds_term->key  &&
        (!ds_term->name || strncmp(ds_term->name, "bnr.", 4)))
    {
      ds_term = ds_diction_next(ds_c);
      continue;
    }

    if (ds_term->s.spam_hits > CTX->totals.spam_learned)
      ds_term->s.spam_hits = CTX->totals.spam_learned;
    if (ds_term->s.innocent_hits > CTX->totals.innocent_learned)
      ds_term->s.innocent_hits = CTX->totals.innocent_learned;

    if (!_ds_set_spamrecord (CTX, ds_term->key, &ds_term->s))
      ret = 0;
    ds_term = ds_diction_next(ds_c);
  }
  ds_diction_close(ds_c);

  return ret;
}

int
_ds_get_spamrecord (
  DSPAM_CTX * CTX,
  unsigned long long token,
  struct _ds_spam_stat *stat)
{
  struct _hash_drv_spam_record rec;
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;

  rec.spam = rec.nonspam = 0;
  rec.hashcode = token;

  stat->offset = _hash_drv_get_spamrecord(s->map, &rec);
  if (!stat->offset)
    return EFAILURE;

  stat->probability   = 0.00000;
  stat->status        = 0;
  stat->innocent_hits = rec.nonspam;
  stat->spam_hits     = rec.spam;

  return 0;
}

int
_ds_set_spamrecord (
  DSPAM_CTX * CTX,
  unsigned long long token,
  struct _ds_spam_stat *stat)
{
  struct _hash_drv_spam_record rec;
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;

  rec.hashcode = token;
  rec.nonspam = (stat->innocent_hits > 0) ? stat->innocent_hits : 0;
  rec.spam = (stat->spam_hits > 0) ? stat->spam_hits : 0;

  return _hash_drv_set_spamrecord(s->map, &rec, stat->offset);
}

int
_ds_set_signature (DSPAM_CTX * CTX, struct _ds_spam_signature *SIG,
                   const char *signature)
{
  char filename[MAX_FILENAME_LENGTH];
  char scratch[128];
  FILE *file;

  _ds_userdir_path(filename, 
                   CTX->home, 
                   (CTX->group) ? CTX->group : CTX->username, 
                   "sig");

  snprintf(scratch, sizeof(scratch), "/%s.sig", signature);
  strlcat(filename, scratch, sizeof(filename));
  _ds_prepare_path_for(filename);

  file = fopen(filename, "w");
  if (!file) {
    LOG(LOG_ERR, ERR_IO_FILE_WRITE, filename, strerror(errno));
    return EFAILURE;
  }
  fwrite(SIG->data, SIG->length, 1, file);
  fclose(file);

  return 0;
}

int
_ds_get_signature (DSPAM_CTX * CTX, struct _ds_spam_signature *SIG,
                   const char *signature)
{
  char filename[MAX_FILENAME_LENGTH];
  char scratch[128];
  FILE *file;
  struct stat statbuf;

  _ds_userdir_path(filename, 
                   CTX->home, 
                   (CTX->group) ? CTX->group : CTX->username,  
                   "sig");

  snprintf(scratch, sizeof(scratch), "/%s.sig", signature);
  strlcat(filename, scratch, sizeof(filename));

  if (stat (filename, &statbuf)) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, filename, strerror(errno));
    return EFAILURE;
  };

  SIG->data = malloc(statbuf.st_size);
  if (!SIG->data) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return EUNKNOWN;
  }

  file = fopen(filename, "r");
  if (!file) {
    LOG(LOG_ERR, ERR_IO_FILE_OPEN, filename, strerror(errno));
    return EFAILURE;
  }
  fread(SIG->data, statbuf.st_size, 1, file);
  SIG->length = statbuf.st_size;
  fclose(file);
  return 0;
}

void *_ds_connect (DSPAM_CTX *CTX)
{
  return NULL;
}

int
_ds_create_signature_id (DSPAM_CTX * CTX, char *buf, size_t len)
{
  char session[64];
  char digit[6];
  int pid, j;
 
  pid = getpid ();
  snprintf (session, sizeof (session), "%8lx%d", (long) time (NULL), pid);
 
  for (j = 0; j < 2; j++) 
  {  
    snprintf (digit, 6, "%d", rand ());
    strlcat (session, digit, 64);
  }

  strlcpy (buf, session, len);
  return 0;
}

int
_ds_verify_signature (DSPAM_CTX * CTX, const char *signature)
{
  char filename[MAX_FILENAME_LENGTH];
  char scratch[128];
  struct stat statbuf;

  _ds_userdir_path(filename, 
                   CTX->home, 
                   (CTX->group) ? CTX->group : CTX->username, 
                   "sig");

  snprintf(scratch, sizeof(scratch), "/%s.sig", signature);
  strlcat(filename, scratch, sizeof(filename));

  if (stat (filename, &statbuf)) 
    return 1;

  return 0;
}

struct _ds_storage_record *
_ds_get_nexttoken (DSPAM_CTX * CTX)
{
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;
  struct _hash_drv_spam_record rec;
  struct _ds_storage_record *sr;
  struct _ds_spam_stat stat;

  rec.hashcode = 0;

  sr = calloc(1, sizeof(struct _ds_storage_record));
  if (!sr) {
    LOG(LOG_CRIT, ERR_MEM_ALLOC);
    return NULL;
  }

  if (s->offset_nexttoken == 0) {
    s->offset_header = s->map->addr;
    s->offset_nexttoken = sizeof(struct _hash_drv_header);
    memcpy(&rec,
           s->map->addr+s->offset_nexttoken, 
           sizeof(struct _hash_drv_spam_record));
    if (rec.hashcode)
      _ds_get_spamrecord (CTX, rec.hashcode, &stat);
  }

  while(rec.hashcode == 0 ||
   ((unsigned long) s->map->addr + s->offset_nexttoken ==
    (unsigned long) s->offset_header + sizeof(struct _hash_drv_header) +
    (s->offset_header->hash_rec_max * sizeof(struct _hash_drv_spam_record))))
  {
    s->offset_nexttoken += sizeof(struct _hash_drv_spam_record);

    if ((unsigned long) s->map->addr + s->offset_nexttoken >
        (unsigned long) s->offset_header + sizeof(struct _hash_drv_header) +
      (s->offset_header->hash_rec_max * sizeof(struct _hash_drv_spam_record)))
    { 
      if (s->offset_nexttoken < s->map->file_len) {
        s->offset_header = s->map->addr + 
          (s->offset_nexttoken - sizeof(struct _hash_drv_spam_record));

        s->offset_nexttoken += sizeof(struct _hash_drv_header);
        s->offset_nexttoken -= sizeof(struct _hash_drv_spam_record);
      } else {
        free(sr);
        return NULL;
      }
    }

    memcpy(&rec,
           s->map->addr+s->offset_nexttoken, 
           sizeof(struct _hash_drv_spam_record));
    _ds_get_spamrecord (CTX, rec.hashcode, &stat);
  }

  sr->token = rec.hashcode;
  sr->spam_hits = stat.spam_hits;
  sr->innocent_hits = stat.innocent_hits;
  sr->last_hit = time(NULL);
  return sr;
}

int
_ds_delete_signature (DSPAM_CTX * CTX, const char *signature)
{
  char filename[MAX_FILENAME_LENGTH];
  char scratch[128];

  _ds_userdir_path(filename, 
                   CTX->home, 
                   (CTX->group) ? CTX->group : CTX->username, 
                   "sig");

  snprintf(scratch, sizeof(scratch), "/%s.sig", signature);
  strlcat(filename, scratch, sizeof(filename));  
  return unlink(filename);
}

char * 
_ds_get_nextuser (DSPAM_CTX * CTX) 
{
  static char user[MAX_FILENAME_LENGTH];
  static char path[MAX_FILENAME_LENGTH];
  struct _hash_drv_storage *s = (struct _hash_drv_storage *) CTX->storage;
  struct nt_node *node_nt, *prev;
  struct nt_c c_nt;
  char *x = NULL, *y;
  DIR *dir = NULL;

  struct dirent *entry;

  if (s->dir_handles->items == 0)
  {
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/data", CTX->home);
    dir = opendir (filename);
    if (dir == NULL)
    {
      LOG (LOG_WARNING,
           "unable to open directory '%s' for reading: %s",
           CTX->home, strerror (errno));
      return NULL;
    }

    nt_add (s->dir_handles, (void *) dir);
    strlcpy (path, filename, sizeof (path));
  }
  else
  {
    node_nt = c_nt_first (s->dir_handles, &c_nt);
    while (node_nt != NULL)
    {
      if (node_nt->next == NULL)
        dir = (DIR *) node_nt->ptr;
      node_nt = c_nt_next (s->dir_handles, &c_nt);
    }
  }

  while ((entry = readdir (dir)) != NULL)
  {
    struct stat st;
    char filename[MAX_FILENAME_LENGTH];
    snprintf (filename, sizeof (filename), "%s/%s", path, entry->d_name);

    if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
      continue;

    if (stat (filename, &st)) {
      continue;
    }

    /* push a new directory */
    if (st.st_mode & S_IFDIR)
    {
      DIR *ndir;

      ndir = opendir (filename);
      if (ndir == NULL)
        continue;
      strlcat (path, "/", sizeof (path));
      strlcat (path, entry->d_name, sizeof (path));
      nt_add (s->dir_handles, (void *) ndir);
      return _ds_get_nextuser (CTX);
    }
    else if (strlen(entry->d_name)>4 &&
      !strncmp ((entry->d_name + strlen (entry->d_name)) - 4, ".css", 4))
    {
      strlcpy (user, entry->d_name, sizeof (user));
      user[strlen (user) - 4] = 0;
      return user;
    }
  }

  /* pop current directory */
  y = strchr (path, '/');
  while (y != NULL)
  {
    x = y;
    y = strchr (x + 1, '/');
  }
  if (x)
    x[0] = 0;

  /* pop directory handle from list */
  node_nt = c_nt_first (s->dir_handles, &c_nt);
  prev = NULL;
  while (node_nt != NULL)
  {
    if (node_nt->next == NULL)
    {
      dir = (DIR *) node_nt->ptr;
      closedir (dir);
      if (prev != NULL) {
        prev->next = NULL;
        s->dir_handles->insert = NULL;
      }
      else
        s->dir_handles->first = NULL;
      free (node_nt);
      s->dir_handles->items--;
      prev = node_nt;
      break;
    }
    prev = node_nt;
    node_nt = c_nt_next (s->dir_handles, &c_nt);
  }
  if (s->dir_handles->items > 0)
    return _ds_get_nextuser (CTX);

  user[0] = 0;
  return NULL;
}

struct _ds_storage_signature *
_ds_get_nextsignature (DSPAM_CTX * CTX)
{
  return NULL;
}

int
_ds_delall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  return 0;
}

int _hash_drv_autoextend(
    hash_drv_map_t map, 
    int extents, 
    unsigned long last_extent_size)
{
  struct _hash_drv_header header;
  struct _hash_drv_spam_record rec;
  int i;

  _hash_drv_close(map);

  map->fd = open(map->filename, O_RDWR);
  if (map->fd < 0) {
    LOG(LOG_WARNING, "unable to resize hash. open failed: %s", strerror(errno));
    return EFAILURE;
  }

  memset(&header, 0, sizeof(struct _hash_drv_header));
  memset(&rec, 0, sizeof(struct _hash_drv_spam_record));
  
  if (extents == 0 || !map->pctincrease)
    header.hash_rec_max = map->extent_size;
  else
    header.hash_rec_max = last_extent_size
                   + (last_extent_size * (map->pctincrease/100.0));

  LOGDEBUG("adding extent last: %d(%ld) new: %d(%ld) pctincrease: %1.2f", extents, last_extent_size, extents+1, header.hash_rec_max, (map->pctincrease/100.0));

  lseek (map->fd, 0, SEEK_END);
  write (map->fd, &header, sizeof(struct _hash_drv_header));
  for(i=0;i<header.hash_rec_max;i++) 
    write (map->fd, &rec, sizeof(struct _hash_drv_spam_record));
  close(map->fd);

  _hash_drv_open(map->filename, map, 0, map->max_seek, 
      map->max_extents, map->extent_size, map->pctincrease, map->flags);
  return 0;
}

unsigned long _hash_drv_seek(
  hash_drv_map_t map,
  unsigned long offset,
  unsigned long long hashcode,
  int flags)
{
  hash_drv_header_t header = map->addr + offset;
  hash_drv_spam_record_t rec;
  unsigned long long fpos;
  unsigned long iterations = 0;

  if (offset >= map->file_len)
    return 0;

  fpos = sizeof(struct _hash_drv_header) + 
    ((hashcode % header->hash_rec_max) * sizeof(struct _hash_drv_spam_record));

  rec = map->addr + offset + fpos;
  while(rec->hashcode != hashcode  &&   /* Match token     */ 
        rec->hashcode != 0         &&   /* Insert on empty */
        iterations < map->max_seek)     /* Max Iterations  */
  {
    iterations++;
    fpos += sizeof(struct _hash_drv_spam_record);

    if (fpos >= (header->hash_rec_max * sizeof(struct _hash_drv_spam_record)))
      fpos = sizeof(struct _hash_drv_header);
    rec = map->addr + offset + fpos;
  }     

  if (rec->hashcode == hashcode) 
    return fpos;

  if (rec->hashcode == 0 && (flags & HSEEK_INSERT)) 
    return fpos;

  return 0;
}

int
_hash_drv_set_spamrecord (
  hash_drv_map_t map,
  hash_drv_spam_record_t wrec,
  unsigned long map_offset)
{
  hash_drv_spam_record_t rec;
  unsigned long offset = 0, extents = 0, last_extent_size = 0, rec_offset = 0;

  if (map->addr == NULL)
    return EINVAL;

  if (map_offset) {
    rec = map->addr + map_offset;
  } else {
    while(rec_offset <= 0 && offset < map->file_len)
    {
      rec_offset = _hash_drv_seek(map, offset, wrec->hashcode, HSEEK_INSERT);
      if (rec_offset <= 0) {
        hash_drv_header_t header = map->addr + offset;
        offset += sizeof(struct _hash_drv_header) +
          (sizeof(struct _hash_drv_spam_record) * header->hash_rec_max);
        last_extent_size = header->hash_rec_max;
        extents++;
      }
    }
  
    if (rec_offset <= 0) {
      if (map->flags & HMAP_AUTOEXTEND) {
        if (extents > map->max_extents && map->max_extents)
          goto FULL;
  
        if (!_hash_drv_autoextend(map, extents-1, last_extent_size))
          return _hash_drv_set_spamrecord(map, wrec, map_offset);
        else
          return EFAILURE;
      } else {
        goto FULL;
      }
    }
  
    rec = map->addr + offset + rec_offset;
  }
  rec->hashcode = wrec->hashcode;
  rec->nonspam  = wrec->nonspam;
  rec->spam     = wrec->spam;

  return 0;

FULL:
  LOG(LOG_WARNING, "hash table %s full", map->filename);
  return EFAILURE;
}

unsigned long
_hash_drv_get_spamrecord (
  hash_drv_map_t map,
  hash_drv_spam_record_t wrec)
{
  hash_drv_spam_record_t rec;
  unsigned long offset = 0, extents = 0, rec_offset = 0;

  if (map->addr == NULL)
    return 0;

  while(rec_offset <= 0 && offset < map->file_len)
  {
    rec_offset = _hash_drv_seek(map, offset, wrec->hashcode, 0);
    if (rec_offset <= 0) {
      hash_drv_header_t header = map->addr + offset;
      offset += sizeof(struct _hash_drv_header) +
        (sizeof(struct _hash_drv_spam_record) * header->hash_rec_max);
      extents++;
    }
  }

  if (rec_offset <= 0) 
    return 0;

  offset += rec_offset;
  rec = map->addr + offset;
  
  wrec->nonspam  = rec->nonspam;
  wrec->spam     = rec->spam;
  return offset;
}

/* Preference Stubs for Flat-File */

agent_pref_t _ds_pref_load(config_t config, const char *user,
  const char *home, void *dbh)
{
  return _ds_ff_pref_load(config, user, home, dbh);
}

int _ds_pref_set(config_t config, const char *user, const char *home,
  const char *attrib, const char *value, void *dbh)
{
  return _ds_ff_pref_set(config, user, home, attrib, value, dbh);
}

int _ds_pref_del(config_t config, const char *user, const char *home,
  const char *attrib, void *dbh)
{
  return _ds_ff_pref_del(config, user, home, attrib, dbh);
}

