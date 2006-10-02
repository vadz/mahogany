/* $Id: purge.sql,v 1.4 2006/04/21 20:39:36 jonz Exp $ */

DELETE FROM dspam_token_data
  WHERE (innocent_hits*2) + spam_hits < 5
  AND last_hit < CURRENT_DATE - 60;

DELETE FROM dspam_token_data
  WHERE innocent_hits = 1 AND spam_hits = 0
  AND last_hit < CURRENT_DATE - 15;

DELETE FROM dspam_token_data
  WHERE innocent_hits = 0 AND spam_hits = 1
  AND last_hit < CURRENT_DATE - 15;

DELETE FROM dspam_token_data
  WHERE last_hit < CURRENT_DATE - 90;

DELETE FROM dspam_signature_data
  WHERE created_on < CURRENT_DATE - 14;

VACUUM ANALYSE;
