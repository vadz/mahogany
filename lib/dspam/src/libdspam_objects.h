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

#ifndef _LIBDSPAM_OBJECTS_H
#  define _LIBDSPAM_OBJECTS_H

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <time.h>
#include "config.h"
#include "config_shared.h"
#include "decode.h"

#if ((defined(__sun__) && defined(__svr4__)) || (defined(__sun) && defined(__SUNPRO_C))) && !defined(u_int32_t) && !defined(__BIT_TYPES_DEFINED__)
#define __BIT_TYPES_DEFINED__
typedef unsigned long long u_int64_t;
typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;
#endif

#ifdef _WIN32
typedef unsigned int u_int32_t;
typedef u_int32_t uid_t;
#endif

extern void *_drv_handle; /* Handle to storage driver library */

/*
 *  struct dspam_factor - A single determining factor
 *
 *  An element containing a determining factor in the dominant calculation of
 *  a message.  An array of these are returned to the calling  application to
 *  explain libdspam's final classification decision.
 */
                                                                                
struct dspam_factor {
  char *token_name;
  float value;
};

/*
 *  struct _ds_spam_totals - User spam totals
 *
 *  Spam totals loaded into the user's filter context upon a call to 
 *  dspam_init().  This structure represents the user's cumulative statistics.
 *
 *  spam_learned, innocent_learned
 *    The total number of messages trained on.
 *
 *  spam_misclassified, innocent_misclassified
 *    The total number of messages that were misclassified by DSPAM, and
 *    submitted for retraining.
 *
 *  spam_classified, innocent_classified
 *    The total number of messages that were classified by DSPAM, but not
 *    learned.  Used exclusively with Train-on-Error mode.
 *
 *  spam_corpusfed, innocent_corpusfed
 *    The total number of messages supplied by the end-user for training.
 *
 *  NOTE: The ordering  of the variables  in the  structure must remain
 *        consistent to ensure backward-compatibility with some storage
 *        drivers (such as the Berkeley DB drivers)
 */

struct _ds_spam_totals
{
  long spam_learned;
  long innocent_learned;
  long spam_misclassified;
  long innocent_misclassified;
  long spam_corpusfed;
  long innocent_corpusfed;
  long spam_classified;
  long innocent_classified;
};

/*
 *  struct _ds_spam_stat - Statistics for a single token:
 *
 *  probability
 *    The calculated probability of the token based on the active pvalue 
 *    algorithm (selected at configure-time).
 *
 *  spam_hits, innocent_hits
 *    The total  number of times the token has appeared in each class  of 
 *    message. If Train-on-Error or Train-until-Mature training modes are
 *    employed,  these values will not  necessarily be updated for  every 
 *    message.
 *
 *  status
 *    TST_DISK	Value was loaded from the storage interface
 *    TST_DIRTY	Statistic is dirty (not written to disk since last modified)
 */

typedef struct _ds_spam_stat
{
  double probability;
  long spam_hits;
  long innocent_hits;
  char status;
  unsigned long offset;
} *ds_spam_stat_t;

/*
 *  struct _ds_spam_signature - A historical classification signature
 *
 *  A binary representation of the original training instance.  The spam
 *  signature  contains all the  metadata used  in the original decision
 *  about the  message, so  that a 1:1 retraining  can take place if the 
 *  message  is submitted for  retraining (e.g. was  misclassified). The
 *  signature contains a series of _ds_signature_token structures, which
 *  house the  original set of tokens used and their frequency counts in 
 *  the message.  A spam signature is a temporary  piece of data that is 
 *  usually purged from disk after a short period of time.
 */

struct _ds_spam_signature
{
  void *data;
  long length;
};

/*
 *  struct _ds_signature_token - An entry in the classification signature
 *
 *  A signature token is a single entry in the binary _ds_spam_signature 
 *  data  blob,  representing  a single  data point  from  the  original 
 *  training instance.
 *
 *  token
 *    The checksum of the original token in the message
 *
 *  frequency
 *    The token's frequency in the original message
 */

struct _ds_signature_token
{
  unsigned long long token;
  unsigned char frequency;
};

/* 
 *  struct _ds_config - libdspam attributes configuration
 *
 *  Each  classification context may have an attributes  configuration
 *  which  is read by various  components of libdspam.  This structure
 *  contains an array of attributes and the size of the array.
 */

struct _ds_config
{
  config_t attributes;
  long size;
};

/*
 *  DSPAM_CTX - The DSPAM Classification Context
 *
 *  A classification context is attached directly to a filter instance 
 *  and supplies the entire context for the filter instance to operate
 *  under.  This  includes  the  user  and group,  operational  flags, 
 *  training  mode, and  the message  being  operated  on. The  filter
 *  instance also  sets specific output variables  within the  context
 *  such  as the  result of a  classification,  confidence  level, and
 *  etcetera.
 *
 *  username, group (input)
 *    The current username and group that is being operated on. 
 *
 *  totals (output)
 *    The set of statistics loaded when dspam_init() is called.
 *
 *  signature (input, output)
 *    The signature represents a DSPAM signature, and can be  supplied
 *    as  an input  variable for  retraining  (e.g. in the  event of a 
 *    misclassification)  or  used as  an output  variable  to store a 
 *    signature  generated   by  the  filter  instance  during  normal 
 *    classification.
 *
 *  message (input)
 *    The  message being operated on, post-actualization. This can  be 
 *    left NULL, and libdspam will automatically actualize the message
 *
 *  probability (output)
 *    The probability of the resulting operation.  This is generally a
 *    floating  point number  between 0 and  1, 1  being  the  highest 
 *    probability of high order classification.
 *
 *  result (output)
 *    The  final result of the requested operation.  This is generally 
 *    either DSR_ISSPAM, DSR_ISINNOCENT, or DSR_WHITELISTED.
 *
 *  confidence (output)
 *    The  confidence  that the  filter has  in  its  returned  result. 
 *    NOTE: Confidence is not always supported, and may be zero.
 *
 *  operating_mode (input)
 *    Sets the operating mode of the filter instance.  This can be one
 *    of the following:
 *
 *	DSM_PROCESS	Classify and learn the  supplied message using 
 *			whatever training mode is specified 
 *
 *	DSM_CLASSIFY	Classify the  supplied  message  only; do  not 
 *                      learn or update any counters.
 *
 *	DSM_TOOLS	Identifies that  the calling function is  from
 *			a utility, and no operation will be requested.
 *
 *  training_mode (input)
 *    The training mode sets the type of training the filter  instance 
 *    should apply to the process. This can be one of:
 *
 *	DST_TEFT		Train-on-Everything
 *				Trains every single message  processed
 *
 *	DST_TOE			Train-on-Error
 *				Trains only on a misclassification  or
 *                              corpus-fed message.
 *
 *	DST_TUM			Train-until-Mature
 *				Trains individual tokens based on  the
 *				maturity of the user's dictionary
 *
 *      DST_NOTRAIN		No Training
 *                              Process the message but do not perform 
 *                              any training.
 *  training_buffer (input)
 *    Sets the amount  of training-loop buffering.  This  number is  a 
 *    range from 0-10  and changes  the amount of  token sedation used 
 *    during the training loop.  The higher the number, the more token 
 *    statistics are watered down  during initial  training to prevent 
 *    false  positives.  Setting  this  value to  zero results  in  no 
 *    sedation being performed.
 *
 *  flags (input)
 *    Applies different fine-tuning behavior to the context:
 *
 *	DSF_NOISE		Apply Bayesian Noise Reduction logic
 *	DSF_SIGNATURE		Signature is provided/requested
 *      DSF_WHITELIST		Use automatic whitelisting logic
 *      DSF_MERGED		Merge user/group data in memory
 *      DSF_UNLEARN		Unlearn the message
 *      DSF_BIAS		Assign processor bias to unknown tokens
 *
 *  tokenizer (input)
 *    Specifies which tokenizer to use
 *
 *      DSZ_WORD		Use WORD (uniGram) tokenizer
 *      DSZ_CHAIN               Use CHAIN (biGram) tokenizer
 *      DSZ_SBPH                Use SBPH (Sparse BP Hashing) tokenizer
 *      DSZ_OSB                 Use OSB (Orthogonal Sparse biGram)
 *
 *  algorithms (input)
 *    Optional API to override the default algorithms. This value is set
 *    with the default compiled values whenever dspam_create() is called.
 *
 *	DSA_GRAHAM		Graham-Bayesian
 *	DSA_BURTON		Burton-Bayesian    
 *	DSA_ROBINSON		Robinson's Geometric Mean Test
 *	DSA_CHI_SQUARE		Fisher-Robinson's Chi-Square
 *      DSA_NAIVE		Naive-Bayesian
 * 
 *    P-Value Computations:
 *
 *      DSP_ROBINSON		Robinson's Technique
 *      DSP_GRAHAM		Graham's Technique
 *      DSP_MARKOV		Markov Weighted Technique
 *
 *  locked (output)
 *    Identifies that the user's storage is presently locked
 */

typedef struct
{
  struct _ds_spam_totals	totals;
  struct _ds_spam_signature *	signature;
  struct _ds_message *		message;
  struct _ds_config *		config;

  char		*username;
  char		*group;
  char		*home;		 /* DSPAM Home */
  int		operating_mode;  /* DSM_ */
  int		training_mode;   /* DST_ */
  int		training_buffer; /* 0-10 */
  int		wh_threshold;    /* Whitelisting Threshold (default 10) */
  int		classification;  /* DSR_ */
  int		source;		 /* DSS_ */
  int		learned;	 /* Did we actually learn something? */
  int           tokenizer;       /* DSZ_ */
  u_int32_t	flags;
  u_int32_t	algorithms;

  int		result;
  char		class[32];
  float		probability;
  float		confidence;

  int		locked;
  void *	storage;
  time_t	_process_start;
  int		_sig_provided;

  struct nt *	factors;

} DSPAM_CTX;

/* Processing Flags */

#define DSF_SIGNATURE		0x02
#define DSF_BIAS		0x04
#define DSF_NOISE		0x08
#define DSF_WHITELIST		0x10
#define DSF_MERGED		0x20
#define DSF_UNLEARN		0x80

/* Tokenizers */

#define DSZ_WORD		0x01
#define DSZ_CHAIN		0x02
#define DSZ_SBPH		0x03
#define DSZ_OSB			0x04

/* Algorithms */

#define DSA_GRAHAM		0x01
#define DSA_BURTON		0x02
#define DSA_ROBINSON		0x04
#define DSA_CHI_SQUARE		0x08
#define DSP_ROBINSON		0x10
#define DSP_GRAHAM		0x20
#define DSP_MARKOV		0x40
#define DSA_NAIVE		0x80

/* Operating Modes */

#define DSM_PROCESS             0x00
#define DSM_TOOLS		0x01
#define DSM_CLASSIFY		0x02
#define DSM_NONE		0xFF

/* Training Modes */ 

#define DST_TEFT		0x00
#define DST_TOE			0x01
#define DST_TUM			0x02
#define DST_NOTRAIN		0xFE
#define DST_DEFAULT		0xFF

/* Classification Results */

#define	DSR_ISSPAM		0x01
#define DSR_ISINNOCENT		0x02
#define DSR_NONE		0xFF

/* Classification Sources */

#define DSS_ERROR       0x00 /* Retraining an error */
#define DSS_CORPUS      0x01 /* Training a message from corpus */
#define DSS_INOCULATION 0x02 /* Message is an inoculation */
#define DSS_NONE	0xFF /* Standard inbound processing */

/* Statuses for token-status bit */
#define TST_DISK	0x01
#define TST_DIRTY	0x02

/* Token Types */
#define DTT_DEFAULT	0x00
#define DTT_BNR		0x01

#define DSP_UNCALCULATED	-1

#define BURTON_WINDOW_SIZE	27

#endif /* _LIBDSPAM_OBJECTS */
