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

#ifdef USE_LDAP

#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libdspam.h"
#include "ldap_client.h"
#include "config.h"
#include "language.h"
#include "error.h"
#include "config_shared.h"

int ldap_verify(DSPAM_CTX *CTX, const char *username) {
  LDAP *ld;
  int result, i;
  int desired_version = LDAP_VERSION3;
  char *ldap_host     = _ds_read_attribute(CTX->config->attributes, "LDAPHost");
  char search_filter[1024];
  LDAPMessage *msg;

  char* base = _ds_read_attribute(CTX->config->attributes, "LDAPBase");
  char* filter = _ds_read_attribute(CTX->config->attributes, "LDAPFilter");

  if (!base || !filter || !ldap_host) {
    LOG(LOG_ERR, ERR_LDAP_MISCONFIGURED); 
    return EFAILURE;
  }

  for(i=0;i<strlen(filter);i++) {
    if (filter[i] == '%' && filter[i+1] == 'u') {
      filter[i+1] = 's';
    }
  }

  snprintf(search_filter, sizeof(search_filter), filter, username);

  if ((ld = ldap_init(ldap_host, LDAP_PORT)) == NULL ) {
    LOG(LOG_ERR, ERR_LDAP_INIT_FAIL);
    return EFAILURE;
  }

  if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version) 
    != LDAP_OPT_SUCCESS)
  {
    LOG(LOG_ERR, ERR_LDAP_PROTO_VER_FAIL);
    return EFAILURE;
  }

  if (ldap_search_s(ld, base, LDAP_SCOPE_SUBTREE, search_filter, NULL, 0, &msg)
   != LDAP_SUCCESS) 
  {
    LOG(LOG_ERR, ERR_LDAP_SEARCH_FAIL);
    return EFAILURE;
  }

  result = ldap_count_entries(ld, msg) > 0;
  ldap_msgfree (msg);
  ldap_unbind(ld);
  return result;
}

#endif

