# $Id: virtual_users.sql,v 1.1 2004/10/24 20:53:29 jonz Exp $

create table dspam_virtual_uids (
  uid smallint unsigned primary key AUTO_INCREMENT,
  username varchar(128)
) type=MyISAM;

create unique index id_virtual_uids_01 on dspam_virtual_uids(username);

