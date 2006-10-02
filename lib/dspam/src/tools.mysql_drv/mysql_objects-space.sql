# $Id: mysql_objects-space.sql,v 1.2 2005/04/11 00:58:27 jonz Exp $

create table dspam_token_data (
  uid smallint unsigned not null,
  token varchar(20) not null, 
  spam_hits int not null,
  innocent_hits int not null,
  last_hit date not null
) type=MyISAM;

create unique index id_token_data_01 on dspam_token_data(uid,token);

create table dspam_signature_data (
  uid smallint unsigned not null,
  signature varchar(32) not null,
  data blob not null,
  length smallint not null,
  created_on date not null
) type=MyISAM;

create unique index id_signature_data_01 on dspam_signature_data(uid,signature);
create index id_signature_data_02 on dspam_signature_data(created_on);

create table dspam_stats (
  uid smallint unsigned primary key not null,
  spam_learned int not null,
  innocent_learned int not null,
  spam_misclassified int not null,
  innocent_misclassified int not null,
  spam_corpusfed int not null,
  innocent_corpusfed int not null,
  spam_classified int not null,
  innocent_classified int not null
) type=MyISAM;

create table dspam_preferences (
  uid smallint unsigned not null, 
  preference varchar(32) not null,
  value varchar(64) not null
) type=MyISAM;

create unique index id_preferences_01 on dspam_preferences(uid, preference);
