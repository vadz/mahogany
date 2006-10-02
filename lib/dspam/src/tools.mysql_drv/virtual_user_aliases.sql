# $Id: virtual_user_aliases.sql,v 1.1 2005/03/12 20:39:05 jonz Exp $

create table dspam_virtual_uids (
  uid smallint not null,
  username varchar(128) not null
) type=MyISAM;

create unique index id_virtual_uids_01 on dspam_virtual_uids(username);
