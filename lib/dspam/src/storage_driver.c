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

#include <stdlib.h>
#include <dlfcn.h>

#include "libdspam.h"
#include "storage_driver.h"
#include "error.h"
#include "language.h"

int dspam_init_driver (DRIVER_CTX *DTX) {
  int (*ptr)(DRIVER_CTX *);

  ptr = (int (*)(DRIVER_CTX *))dlsym(_drv_handle, "dspam_init_driver");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(dspam_init_driver) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(DTX);
}

int dspam_shutdown_driver (DRIVER_CTX *DTX) {
  int (*ptr)(DRIVER_CTX *);
  
  ptr = (int (*)(DRIVER_CTX *))dlsym(_drv_handle, "dspam_shutdown_driver");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(dspam_shutdown_driver) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(DTX);
}

int _ds_init_storage (DSPAM_CTX * CTX, void *dbh) {
  int (*ptr)(DSPAM_CTX *, void *);

  ptr = (int (*)(DSPAM_CTX *, void *))dlsym(_drv_handle, "_ds_init_storage");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_init_storage) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, dbh);
}

int _ds_shutdown_storage (DSPAM_CTX * CTX)
{
  int (*ptr)(DSPAM_CTX *);

  ptr = (int (*)(DSPAM_CTX *))dlsym(_drv_handle, "_ds_shutdown_storage");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_shutdown_storage) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX);
}

int _ds_getall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  int (*ptr)(DSPAM_CTX *, ds_diction_t);

  ptr = (int (*)(DSPAM_CTX *, ds_diction_t))dlsym(_drv_handle, "_ds_getall_spamrecords");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_getall_spamrecords) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, diction);
}

int _ds_setall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  int (*ptr)(DSPAM_CTX *, ds_diction_t);  
  ptr = (int (*)(DSPAM_CTX *, ds_diction_t))dlsym(_drv_handle, "_ds_setall_spamrecords");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_setall_spamrecords) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, diction);
}

int _ds_delall_spamrecords (DSPAM_CTX * CTX, ds_diction_t diction)
{
  int (*ptr)(DSPAM_CTX *, ds_diction_t);
  ptr = (int (*)(DSPAM_CTX *, ds_diction_t))dlsym(_drv_handle, "_ds_delall_spamrecords");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_delall_spamrecords) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, diction);
}

int _ds_get_spamrecord (
  DSPAM_CTX * CTX, 
  unsigned long long token,
  struct _ds_spam_stat *stat)
{
  int (*ptr)(DSPAM_CTX *, unsigned long long, struct _ds_spam_stat *);
  ptr = (int (*)(DSPAM_CTX *, unsigned long long, struct _ds_spam_stat *))dlsym(_drv_handle, "_ds_get_spamrecord");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_get_spamrecord) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, token, stat);
}

int _ds_set_spamrecord (
  DSPAM_CTX * CTX,
  unsigned long long token,
  struct _ds_spam_stat *stat)
{
  int (*ptr)(DSPAM_CTX *, unsigned long long, struct _ds_spam_stat *);
  ptr = (int (*)(DSPAM_CTX *, unsigned long long, struct _ds_spam_stat *))dlsym(_drv_handle, "_ds_set_spamrecord");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_set_spamrecord) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, token, stat);
}

int _ds_del_spamrecord (DSPAM_CTX * CTX, unsigned long long token)
{
  int (*ptr)(DSPAM_CTX *, unsigned long long);
  ptr = (int (*)(DSPAM_CTX *, unsigned long long))dlsym(_drv_handle, "_ds_del_spamrecord");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_del_spamrecord) failed: %s", dlerror());
    return EFAILURE;
  }

  return (*ptr)(CTX, token);
}

struct _ds_storage_record *_ds_get_nexttoken (DSPAM_CTX * CTX)
{
  struct _ds_storage_record *(*ptr)(DSPAM_CTX *);
  ptr = (struct _ds_storage_record * (*)(DSPAM_CTX *))dlsym(_drv_handle, "_ds_get_nexttoken");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_get_nexttoken) failed: %s", dlerror());
    return NULL;
  }

  return (*ptr)(CTX);
}

struct _ds_storage_signature *_ds_get_nextsignature (DSPAM_CTX * CTX) 
{
  struct _ds_storage_signature *(*ptr)(DSPAM_CTX *);
  ptr = (struct _ds_storage_signature * (*)(DSPAM_CTX *))dlsym(_drv_handle, "_ds_get_nextsignature");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_get_nextsignature) failed: %s", dlerror());
    return NULL;
  }

  return (*ptr)(CTX);
}

char *_ds_get_nextuser (DSPAM_CTX * CTX)
{
  char *(*ptr)(DSPAM_CTX *);
  ptr = (char * (*)(DSPAM_CTX *))dlsym(_drv_handle, "_ds_get_nextuser");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_get_nextuser) failed: %s", dlerror());
    return NULL;
  }

  return (*ptr)(CTX);
}

int _ds_delete_signature (DSPAM_CTX * CTX, const char *signature)
{
  int (*ptr)(DSPAM_CTX *, const char *);
  ptr = (int (*)(DSPAM_CTX *, const char *))dlsym(_drv_handle, "_ds_delete_signature");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_delete_signature) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(CTX, signature);
}

int _ds_get_signature (
  DSPAM_CTX * CTX, 
  struct _ds_spam_signature *SIG,
  const char *signature)
{
  int (*ptr)(DSPAM_CTX *, struct _ds_spam_signature *, const char *);
  ptr = (int (*)(DSPAM_CTX *, struct _ds_spam_signature *, const char *))dlsym(_drv_handle, "_ds_get_signature");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_get_signature) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(CTX, SIG, signature);
}

int _ds_set_signature (
  DSPAM_CTX * CTX,
  struct _ds_spam_signature *SIG,
  const char *signature)
{
  int (*ptr)(DSPAM_CTX *, struct _ds_spam_signature *, const char *);  ptr = (int (*)(DSPAM_CTX *, struct _ds_spam_signature *, const char *))dlsym(_drv_handle, "_ds_set_signature");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_set_signature) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(CTX, SIG, signature);
}

int _ds_verify_signature (DSPAM_CTX * CTX, const char *signature)
{
  int (*ptr)(DSPAM_CTX *, const char *);  ptr = (int (*)(DSPAM_CTX *, const char *))dlsym(_drv_handle, "_ds_verify_signature");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_verify_signature) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(CTX, signature);
}

int _ds_create_signature_id (DSPAM_CTX * CTX, char *buf, size_t len)
{
  int (*ptr)(DSPAM_CTX *, char *, size_t);
  ptr = (int (*)(DSPAM_CTX *, char *, size_t))dlsym(_drv_handle, "_ds_create_signature_id");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_create_signature_id) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(CTX, buf, len);
}

void *_ds_connect (DSPAM_CTX *CTX)
{
  void *(*ptr)(DSPAM_CTX *);
  ptr = (void * (*)(DSPAM_CTX *))dlsym(_drv_handle, "_ds_connect");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_connect) failed: %s", dlerror());
    return NULL;
  }
  return (*ptr)(CTX);
}

agent_pref_t _ds_pref_load(
  config_t config,
  const char *user,
  const char *home, void *dbh)
{
  agent_pref_t (*ptr)(config_t, const char *, const char *, void *);
  ptr = (agent_pref_t (*)(config_t, const char *, const char *, void *))dlsym(_drv_handle, "_ds_pref_load");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_pref_load) failed: %s", dlerror());
    return NULL;
  }
  return (*ptr)(config, user, home, dbh);
}

int _ds_pref_set(
  config_t config,
  const char *user,
  const char *home,
  const char *attrib,
  const char *value,
  void *dbh)
{
  int (*ptr)(config_t, const char *, const char *, const char *, const char *, void *);
  ptr = (int (*)(config_t, const char *, const char *, const char *, const char *, void *))dlsym(_drv_handle, "_ds_pref_set");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_pref_set) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(config, user, home, attrib, value, dbh);
}

int _ds_pref_del(
  config_t config,
  const char *user,
  const char *home, 
  const char *attrib,
  void *dbh)
{
  int (*ptr)(config_t, const char *, const char *, const char *, void *);
  ptr = (int (*)(config_t, const char *, const char *, const char *, void *))dlsym(_drv_handle, "_ds_pref_del");
  if (!ptr) {
    LOG(LOG_CRIT, "dlsym(_ds_pref_del) failed: %s", dlerror());
    return EFAILURE;
  }
  return (*ptr)(config, user, home, attrib, dbh);
}

