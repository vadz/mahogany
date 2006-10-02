/* $Id: virtual_users.sql,v 1.1 2004/10/24 20:51:55 jonz Exp $ */

CREATE SEQUENCE dspam_virtual_uids_seq;

CREATE TABLE dspam_virtual_uids (
  uid smallint DEFAULT nextval('dspam_virtual_uids_seq') PRIMARY KEY,
  username varchar(128)
) WITHOUT OIDS;

CREATE UNIQUE INDEX id_virtual_uids_01 ON dspam_virtual_uids(username);
CREATE UNIQUE INDEX id_virtual_uids_02 ON dspam_virtual_uids(uid);
