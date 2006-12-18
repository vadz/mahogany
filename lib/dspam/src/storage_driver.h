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

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#ifndef _STORAGE_DRIVER_H
#  define _STORAGE_DRIVER_H

#include "libdspam_objects.h"
#include "diction.h"
#include "pref.h"
#include "config_shared.h"
#ifdef DAEMON
#include <pthread.h>
#endif

/*
 *  _ds_drv_connection: a storage resource for a server worker thread 
 *  in stateful (daemon) mode, the storage driver may create a series
 *  of _ds_drv_connections which are then pooled into a series of worker
 *  threads. depending on the locking paradigm (mutex or rwlock), one
 *  connection may or may not service multiple threads simultaneously.
 *  connections usually contain stateful resources such as connections to
 *  a backend database or in the case of hash_drv, pointers to an mmap'd
 *  portion of memory.
 */

struct _ds_drv_connection
{
  void *dbh;
#ifdef DAEMON
  pthread_mutex_t lock;
  pthread_rwlock_t rwlock;
#endif
};

/*
 *  DRIVER_CTX: storage driver context
 *  a single storage driver context is used to pool connection resources,
 *  set driver behavior (e.g. locking paradigm), and most importantly connect
 *  to the dspam context being used by the worker thread. 
 */

typedef struct {
  DSPAM_CTX *CTX;				/* IN */
  int status;					/* OUT */
  int flags;					/* IN */
  int connection_cache;				/* IN */
  struct _ds_drv_connection **connections;	/* OUT */
} DRIVER_CTX;

/*
 *  _ds_storage_record: dspam-facing storage structure
 *  the _ds_storage_record structure is a common structure passed between
 *  libdspam and the storage abstraction layer. each instance represents a 
 *  single token in a diction. once the storage driver has it, it can create
 *  its own internal structures, but must pass this structure back and forth
 *  to libdspam.
 */

struct _ds_storage_record
{
  unsigned long long token;
  long spam_hits;
  long innocent_hits;
  time_t last_hit;
};

/*
 *  _ds_storage_signature: dspam-facing signature structure
 *  the _ds_storage_signature structure is a common structure passed between
 *  libdspam and the storage abstraction layer. the signature represents
 *  binary training data used later for reclassification of errors. once the
 *  storage driver has it, it can create its own internal structures, but
 *  must pass this structure back and forth to libdspam.
 */
 
struct _ds_storage_signature
{
  char signature[256];
  void *data;
  long length;
  time_t created_on;
};

int dspam_init_driver     (DRIVER_CTX *DTX);
int dspam_shutdown_driver (DRIVER_CTX *DTX);
int _ds_init_storage      (DSPAM_CTX * CTX, void *dbh);
int _ds_shutdown_storage  (DSPAM_CTX * CTX);
void *_ds_connect         (DSPAM_CTX *CTX);

int _ds_getall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction);
int _ds_setall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction);
int _ds_delall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction);

int _ds_get_spamrecord(
  DSPAM_CTX * CTX,
  unsigned long long token,
  struct _ds_spam_stat *stat);
int _ds_set_spamrecord(
  DSPAM_CTX * CTX,
  unsigned long long token,
  struct _ds_spam_stat *stat);
int _ds_del_spamrecord(
  DSPAM_CTX * CTX,
  unsigned long long token);

struct _ds_storage_record *_ds_get_nexttoken (DSPAM_CTX * CTX);
struct _ds_storage_signature *_ds_get_nextsignature (DSPAM_CTX * CTX);
char *_ds_get_nextuser (DSPAM_CTX * CTX);

int _ds_delete_signature(
  DSPAM_CTX * CTX,
  const char *signature);
int _ds_get_signature(
  DSPAM_CTX * CTX,
  struct _ds_spam_signature *SIG,
  const char *signature);
int _ds_set_signature(
  DSPAM_CTX * CTX,
  struct _ds_spam_signature *SIG,
  const char *signature);
int _ds_verify_signature(
  DSPAM_CTX * CTX,
  const char *signature);
int _ds_create_signature_id(
  DSPAM_CTX * CTX,
  char *buf,
  size_t len);

/*
 *  Storage Driver Preferences Extension
 *  When defined, the built-in preferences functions are overridden with
 *  functions specific to the storage driver. This allows preferences to be
 *  alternatively stored in the storage facility instead of flat files. 
 *  The selected storage driver must support preference extensions.
 */

agent_pref_t _ds_pref_load(
  config_t config,
  const char *user,
  const char *home, void *dbh);
int _ds_pref_set(
  config_t config,
  const char *user,
  const char *home,
  const char *attrib,
  const char *value,
   void *dbh);
int _ds_pref_del(
  config_t config,
  const char *user,
  const char *home, 
  const char *attrib,
  void *dbh);

/* Driver context flags */

#define DRF_STATEFUL	0x01
#define DRF_RWLOCK	0x02

/* Driver statuses */

#define DRS_ONLINE	0x01
#define DRS_OFFLINE	0x02
#define DRS_UNKNOWN	0xFF

#ifdef _MSC_VER
  #define ULL_CONST(x)  x ## ui64
#else
  #define ULL_CONST(x)  x ## llu
#endif

#define CONTROL_TOKEN	ULL_CONST(11624422384514212933)
			/* $$CONTROL$$ */

#endif /* _STORAGE_DRIVER_H */
