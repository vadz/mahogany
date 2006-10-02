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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>

#include "libdspam.h"
#include "util.h"
#include "read_config.h"
#include "config_api.h"
#include "language.h"
#include "storage_driver.h"
#include "pgsql_drv.h"

DSPAM_CTX *open_ctx, *open_mtx;

int opt_humanfriendly;

void dieout (int signal);
void usage (void);
void GenSQL (PGconn *dbh,const char *file);
void OutputMessage(DSPAM_CTX *open_ctx,char *sqlfile);

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif

/*
** Type OIDs; values come from postgresql/include/server/catalog/pg_type.h
*/

#ifndef NUMERICOID
# define NUMERICOID 1700
#endif
#ifndef INT8OID
# define INT8OID    20
#endif
#define BIGINTOID   INT8OID

int
main (int argc, char **argv)
{
#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  struct passwd *p = getpwuid (getuid ());
#endif
#endif
  struct _pgsql_drv_storage *store;
  char file[PATH_MAX+1];
  int i, ch;
#ifndef HAVE_GETOPT
  int optind = 1;
#endif

  /* Read dspam.conf */

  agent_config = read_config(NULL);
  if (!agent_config) {
    LOG(LOG_ERR, ERR_AGENT_READ_CONFIG);
    exit(EXIT_FAILURE);
  }

  if (!_ds_read_attribute(agent_config, "Home")) {
    LOG(LOG_ERR, ERR_AGENT_DSPAM_HOME);
    _ds_destroy_config(agent_config);
    exit(EXIT_FAILURE);
  }

#ifndef _WIN32
#ifdef TRUSTED_USER_SECURITY
  if (!_ds_match_attribute(agent_config, "Trust", p->pw_name) && p->pw_uid) {
    fprintf(stderr, ERR_TRUSTED_MODE "\n");
    _ds_destroy_config(agent_config);
    exit(EXIT_FAILURE);
  }
#endif
#endif

  for(i=0;i<argc;i++) {
                                                                                
    if (!strncmp (argv[i], "--profile=", 10))
    {
      if (!_ds_match_attribute(agent_config, "Profile", argv[i]+10)) {
        LOG(LOG_ERR, ERR_AGENT_NO_SUCH_PROFILE, argv[i]+10);
        _ds_destroy_config(agent_config);
        exit(EXIT_FAILURE);
      } else {
        _ds_overwrite_attribute(agent_config, "DefaultProfile", argv[i]+10);
      }
      break;
    }
  }

  open_ctx = open_mtx = NULL;

  signal (SIGINT, dieout);
  signal (SIGPIPE, dieout);
  signal (SIGTERM, dieout);

  /* Process command line */
  ch = 0;
#ifdef HAVE_GETOPT
  while((ch = getopt(argc, argv, "h")) != -1)
#else
  while ( argv[optind] &&
            argv[optind][0] == '-' &&
              (ch = argv[optind][1]) &&
                argv[optind][2] == '\0' )
#endif
  {
    switch(ch) {
      case 'h':
        /* print help, and then exit. usage exits for us */
        usage();
        break;

#ifndef HAVE_GETOPT
      default:
        fprintf(stderr, "%s: unknown option \"%s\".\n",
                argv[0], argv[optind] + 1);
        usage();
#endif
    }
#ifndef HAVE_GETOPT
    optind++;
#endif
  }
  /* reset our option array and index to where we are after getopt */
  argv += optind;
  argc -= optind;

  if (argc == 0) {
    fprintf(stderr,"Must specify an output file\n");
    usage();
  }

  memset((void *)file, 0, PATH_MAX+1);
  strncpy(file, argv[0], PATH_MAX);

  open_ctx = dspam_create(NULL,NULL,_ds_read_attribute(agent_config, "Home"), DSM_TOOLS, 0);
  if (open_ctx == NULL) {
    fprintf(stderr, "Could not initialize context: %s\n", strerror (errno));
    exit(EXIT_FAILURE);
  }

  set_libdspam_attributes(open_ctx);
  if (dspam_attach(open_ctx, NULL)) {
    fprintf(stderr,"Failed to init link to PostgreSQL\n");
    dspam_destroy(open_ctx);
    exit(EXIT_FAILURE);
  }
 
  store = (struct _pgsql_drv_storage *)(open_ctx->storage);
  GenSQL(store->dbh,file);
  //PQfinish(store->dbh);

  OutputMessage(open_ctx,file);

  if (open_ctx != NULL)
    dspam_destroy (open_ctx);
  if (open_mtx != NULL)
    dspam_destroy (open_mtx);
  _ds_destroy_config(agent_config);
  exit (EXIT_SUCCESS);
}

#define TOKEN_TYPE_LEN 20
#define TOKEN_DATA_LEN 30

void
GenSQL (PGconn *dbh,const char *file)
{
  int i,ntuples;
  int reverse=0;
  int type_check=0;
  int preamble=0;
  PGresult *result;
  Oid col_type;
  FILE *out;
  char token_data[TOKEN_DATA_LEN+1];
  char token_type[TOKEN_TYPE_LEN+1];
  unsigned long long token;
  memset((void *)token_type, 0, TOKEN_TYPE_LEN+1);
  memset((void *)token_data, 0, TOKEN_DATA_LEN+1);

  if (strncmp(file,"-",1)==0) {
    out=stdout;
  } else {
    if ( (out = fopen(file,"w+")) == NULL ) {
      fprintf(stderr, "Failed to open file %s for writing - %s\n", 
              file, strerror(errno));
      PQfinish(dbh);
      exit(EXIT_FAILURE);
    }
  }
  result = PQexec(dbh, "SELECT uid, token, spam_hits, innocent_hits, last_hit "
                       "FROM dspam_token_data");
  if (! result || PQresultStatus(result) != PGRES_TUPLES_OK) {
    fprintf(stderr, "Failed to run result: %s\n", PQresultErrorMessage(result));
    if (result) PQclear(result);
    PQfinish(dbh);
    exit(EXIT_FAILURE);
  }

  ntuples = PQntuples(result);
  for (i=0; i<ntuples; i++) 
  {
    if (!type_check) 
    {
      type_check = 1;
      col_type = PQftype(result, 1);

      if (col_type == BIGINTOID) 
      {
        fprintf(stderr, "Datatype of dspam_token_data.token *not* NUMERIC;\n"
           "assuming that you want to revert back to NUMERIC(20) type from BIGINT type\n");
        reverse=1;
      } else if (col_type != NUMERICOID) 
      {
        fprintf(stderr, "Type of dspam_token_data.token is not BIGINT *or* NUMERIC(20)!\n"
           "I have got no clue of how to deal with this and I am going to sulk now.\n");
        if (result) PQclear(result);
        PQfinish(dbh);
        exit(EXIT_FAILURE);
      }
    }
    if (!preamble) 
    {
      preamble = 1;
      if (reverse == 0) {
        snprintf(token_type, TOKEN_TYPE_LEN, "bigint");
      } else {
        snprintf(token_type, TOKEN_TYPE_LEN, "numeric(20)");
      }
      fprintf(out,"BEGIN;\n"
                  "DROP TABLE dspam_token_data;\n"
                  "COMMIT;\n"
                  "BEGIN;\n"
                  "CREATE TABLE dspam_token_data (\n"
                  "  uid smallint,\n"
                  "  token %s,\n"
                  "  spam_hits int,\n"
                  "  innocent_hits int,\n"
                  "  last_hit date,\n"
                  "  UNIQUE (token, uid)\n"
                  ") WITHOUT OIDS;\n"
                  "COMMIT;\n"
                  "BEGIN;\n"
                  "COPY dspam_token_data (uid,token,spam_hits,innocent_hits,last_hit) FROM stdin;\n"
                  , token_type);
    }
    if (!reverse) {
      token = strtoull( PQgetvalue(result,i,1), NULL, 0);
      snprintf(token_data, TOKEN_DATA_LEN, "%lld", token);
    } else { 
      token = (unsigned long long) strtoll( PQgetvalue(result,i,1), NULL, 0);
      snprintf(token_data, TOKEN_DATA_LEN, "%llu", token);
    }
    fprintf(out,"%s\t%s\t%s\t%s\t%s\n",
                PQgetvalue(result,i,0), token_data,
                PQgetvalue(result,i,2),
                PQgetvalue(result,i,3),
                PQgetvalue(result,i,4) );
  }
  if (result) PQclear(result);
  fprintf(out, "\\.\n\n"
               "COMMIT;\n"
               "BEGIN;\n"
               "CREATE INDEX id_token_data_03 ON dspam_token_data(token);\n"
               "CREATE INDEX id_token_data_04 ON dspam_token_data(uid);\n"
               "COMMIT;\n"
               "ANALYSE;\n");
}

/*
** Code taken from pgsql_drv.c
*/
void OutputMessage(DSPAM_CTX *open_ctx,char *sqlfile)
{
  FILE *file;
  char filename[MAX_FILENAME_LENGTH];
  char buffer[256];
  char hostname[128] = "";
  char user[64] = "";
  char db[64] = "";
  int port = 5432, i = 0;
  if (_ds_read_attribute(open_ctx->config->attributes, "PgSQLServer")) {
    char *p;

    strlcpy(hostname,
           _ds_read_attribute(open_ctx->config->attributes, "PgSQLServer"),
            sizeof(hostname));

    if (_ds_read_attribute(open_ctx->config->attributes, "PgSQLPort"))
      port = atoi(_ds_read_attribute(open_ctx->config->attributes, "PgSQLPort"));
    else
      port = 0;

    if ((p = _ds_read_attribute(open_ctx->config->attributes, "PgSQLUser")))
      strlcpy(user, p, sizeof(user));
    if ((p = _ds_read_attribute(open_ctx->config->attributes, "PgSQLDb")))
      strlcpy(db, p, sizeof(db));

  } else {
    snprintf (filename, MAX_FILENAME_LENGTH, "%s/pgsql.data", open_ctx->home);
    file = fopen (filename, "r");
    if (file == NULL)
    {
      fprintf(stderr, "Failed to open config file %s - %s\n",
              filename, strerror(errno));
      dieout(0);
    }
 
    db[0] = 0;
 
    while (fgets (buffer, sizeof (buffer), file) != NULL)
    {
      chomp (buffer);
      if (!i)
        strlcpy (hostname, buffer, sizeof (hostname));
      else if (i == 1)
        port = atoi (buffer);
      else if (i == 2)
        strlcpy (user, buffer, sizeof (user));
      else if (i == 4)
        strlcpy (db, buffer, sizeof (db));
      i++;
    }
    fclose (file);
  }

  if (db[0] == 0)
  {
    fprintf(stderr, "file %s: incomplete pgsql connect data", filename);
    dieout(0);
  }

  if (port == 0) port = 5432;

  fprintf(stderr, "Created SQL in %s; run using:\n", sqlfile);
  
  if ( strlen(hostname) == 0 )
    fprintf(stderr, "\tpsql -q -p %d -U %s -d %s -f %s -v AUTOCOMMIT=off\n",
                    port, user, db, sqlfile);
  else
    fprintf(stderr, "\tpsql -q -h %s -p %d -U %s -d %s -f %s -v AUTOCOMMIT=off\n",
                    hostname, port, user, db, sqlfile);

}

void
dieout (int signal)
{
  fprintf (stderr, "terminated.\n");
  if (open_ctx != NULL)
    dspam_destroy (open_ctx);
  if (open_mtx != NULL)
    dspam_destroy (open_mtx);
  _ds_destroy_config(agent_config);
  exit (EXIT_SUCCESS);
}

void
usage (void)
{
  (void)fprintf (stderr, "Usage: dspam_pg2int8 [-h] file\n"
      "\tCreates SQL file to migrate from NUMERIC to BIGINT type and vice-versa in PostgreSQL.\n"
      "\t-h: print this message\n");
  _ds_destroy_config(agent_config);
  exit(EXIT_FAILURE);
}

