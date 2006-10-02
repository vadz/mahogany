/* $Id: pgsql_objects.sql,v 1.14 2006/07/28 15:34:37 jonz Exp $ */

CREATE TABLE dspam_token_data (
  uid smallint,
  token bigint,
  spam_hits int,
  innocent_hits int,
  last_hit date,
  UNIQUE (uid, token)
) WITHOUT OIDS;

CREATE TABLE dspam_signature_data (
  uid smallint,
  signature varchar(128),
  data bytea,
  length int,
  created_on date,
  UNIQUE (uid, signature)
) WITHOUT OIDS;

CREATE TABLE dspam_stats (
  uid smallint PRIMARY KEY,
  spam_learned int,
  innocent_learned int,
  spam_misclassified int,
  innocent_misclassified int,
  spam_corpusfed int,
  innocent_corpusfed int,
  spam_classified int,
  innocent_classified int
) WITHOUT OIDS;

CREATE TABLE dspam_preferences (
  uid smallint,
  preference varchar(128),
  value varchar(128),
  UNIQUE (uid, preference)
) WITHOUT OIDS;

create language plpgsql;
create function lookup_tokens(integer,bigint[])
  returns setof dspam_token_data
  language plpgsql stable
  as '
declare
  v_rec record;
begin
  for v_rec in select * from dspam_token_data
                where uid=$1
                  and token in (select $2[i]
                                  from generate_series(array_lower($2,1),
                                                       array_upper($2,1)) s(i))
  loop
    return next v_rec;
  end loop;
  return;
end;';

/* For much better performance
 * see http://archives.postgresql.org/pgsql-performance/2004-11/msg00416.php
 * and http://archives.postgresql.org/pgsql-performance/2004-11/msg00417.php
 * for details
 */
alter table "dspam_token_data" alter "token" set statistics 200;
alter table dspam_signature_data alter signature set statistics 200;
alter table dspam_token_data alter innocent_hits set statistics 200;
alter table dspam_token_data alter spam_hits set statistics 200;
CREATE INDEX id_token_data_sumhits ON dspam_token_data ((spam_hits + innocent_hits));
analyze;
