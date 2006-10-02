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

/*
 * bnr.c - bayesian noise reduction - contextual symmetry logic
 *
 * http://bnr.nuclearelephant.com 
 *
 */

#ifdef HAVE_CONFIG_H
#include <auto-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "bnr.h"

/*
 * bnr_init(): Create and initialize a new noise reduction context
 * parameters:	type (int)	BNR_CHAR:  Token identifier as character arrays
 *				BNR_INDEX: Token identifiers as pointers
 * 		identifier (char)	An identifier to add to the pattern
 *					name to identify the type of stream
 *				
 * returns:	pointer to the new context
 */

BNR_CTX *bnr_init(int type, char identifier)
{
  BNR_CTX *BTX;

  BTX = calloc(1, sizeof(BNR_CTX));
  if (BTX == NULL) {
    perror("memory allocation error: bnr_init() failed");
    return NULL;
  }

  BTX->identifier  = identifier;
  BTX->window_size = 3;
  BTX->ex_radius   = 0.25;
  BTX->in_radius   = 0.33;
  BTX->stream     = bnr_list_create(type);
  BTX->patterns   = bnr_hash_create(1543ul);
  if (BTX->stream == NULL || BTX->patterns == NULL) {
    perror("memory allocation error: bnr_init() failed");
    bnr_list_destroy(BTX->stream);
    bnr_hash_destroy(BTX->patterns);
    free(BTX);
    return NULL;
  }

  return BTX;
}

/*
 * bnr_destroy(): Destroys a noise reduction context no longer being used
 * parameters:	BTX (BNR_CTX *)	The context to destroy
 * returns:	0 on success
 */

int bnr_destroy(BNR_CTX *BTX) {
  bnr_list_destroy(BTX->stream);
  bnr_hash_destroy(BTX->patterns);
  free(BTX);
  return 0;
}

/*
 * bnr_add(): Adds a token to the noise reduction stream. This function
 *   should be called once for each token in the message body (in order).
 *
 * parameters:	BTX (BNR_CTX *)	The noise reduction context to use
 *		token (void *)	The token's name, or pointer if NT_INDEX
 *		value (float)	The token's probability
 * returns:	0 on success
 */

int bnr_add(BNR_CTX *BTX, void *token, float value) {

  return (bnr_list_insert(BTX->stream, token, value) != NULL) ? 0 : EFAILURE;
}

/*
 * bnr_instantiate(): Instantiates a series of patterns for the given stream.
 *   This function should be called after all tokens are added to the stream.
 *
 * parameters:	BTX (BNR_CTX *)		The noise reduction context to use
 * returns:	0 on success
 */

int bnr_instantiate(BNR_CTX *BTX) {
  int BNR_SIZE = BTX->window_size;
  float previous_bnr_probs[BNR_SIZE];
  struct bnr_list_node *node_list;
  struct bnr_list_c c_list;
  char bnr_token[64];
  int i;

  for(i=0;i<BNR_SIZE;i++)
    previous_bnr_probs[i] = 0.00000;

  node_list = c_bnr_list_first(BTX->stream, &c_list);
  while(node_list != NULL) {
    
    for(i=0;i<BNR_SIZE-1;i++) {
      previous_bnr_probs[i] = previous_bnr_probs[i+1];
    }

    previous_bnr_probs[BNR_SIZE-1] = _bnr_round(node_list->value);
    sprintf(bnr_token, "bnr.%c|", BTX->identifier);
    for(i=0;i<BNR_SIZE;i++) {
      char x[6];
      snprintf(x, 6, "%01.2f_", previous_bnr_probs[i]);
      strcat(bnr_token, x);
    }

#ifdef LIBBNR_VERBOSE_DEBUG
    fprintf(stderr, "libbnr: instantiating pattern '%s'\n", bnr_token);
#endif

    bnr_hash_hit (BTX->patterns, bnr_token);
    node_list = c_bnr_list_next(BTX->stream, &c_list);
  }

  return 0;
}

/*
 * bnr_get_pattern(): Retrieves the next instantiated pattern.
 *   This function should be called after a call to bnr_instantiate(). Each
 *   call to bnr_get_pattern() will return the next instantiated pattern, which
 *   should then be looked up by your classifier and assigned a value using
 *   bnr_set_pattern().
 *
 * parameters:	BTX (BNR_CTX *)	The noise reduction context to use
 * returns:	The name of the next instantiated pattern in the context
 */
 
char *bnr_get_pattern(BNR_CTX *BTX) {
  struct bnr_hash_node *node;
 
  if (!BTX->pattern_iter) {
    node = c_bnr_hash_first(BTX->patterns, &BTX->c_pattern);
    BTX->pattern_iter = 1;
  } else {
    node = c_bnr_hash_next(BTX->patterns, &BTX->c_pattern);
  }

  if (node)
    return node->name;

  BTX->pattern_iter = 0;
  return NULL;
}

/*
 * bnr_set_pattern(): Sets the value of a pattern
 *   This function should be called once for each pattern instantiated. The
 *   name of the patterns can be retrieved using repeated calls to
 *   bnr_get_pattern(). The value of the pattern should then be looked up by
 *   the classifier and set in the context using this function.
 *
 * parameters:	BTX (BNR_CTX *)		The noise reduction context to use
 * 		name (const char *)	The name of the pattern to set
 *		value (float)		The p-value of the pattern
 * returns:	0 on success
 */

int bnr_set_pattern(BNR_CTX *BTX, const char *name, float value) {
  return bnr_hash_set(BTX->patterns, name, value);
}

/*
 * bnr_get_token() Retrieves the next token from the stream.
 *   This function should be called after a call to bnr_finalize(). Each
 *   call to bnr_get_token() will return the next token and set its elimination
 *   status (by way of the passed-in variable).
 * parameters:	BTX (BNR_CTX *)		The noise reduction context to use
 * returns:	The name (or pointer) of the next non-eliminated token
 */

void *bnr_get_token(BNR_CTX *BTX, int *eliminated) {
  struct bnr_list_node *node;

  if (BTX->stream_iter == 0) {
    BTX->stream_iter = 1;
    node = c_bnr_list_first(BTX->stream, &BTX->c_stream);
  } else {
    node = c_bnr_list_next(BTX->stream, &BTX->c_stream);
  }

  if (node) {
    if (node->eliminated) 
      *eliminated = 1;
    else 
      *eliminated = 0;
    return node->ptr;
  }

  BTX->stream_iter = 0;
  return NULL;
}

/*
 * _bnr_round(): [internal] Round value to the nearest 0.05
 * parameters:	value (float)	Value to be rounded
 * returns:	Rounded value as a float
 */

float _bnr_round(float n) {
  int r = (n*100);
  while(r % 5)
    r++;
  return (r/100.0);
}

/*
 * bnr_finalize() Finalizes the noise reduction context and performs dubbing
 *   This function should be called after all calls to bnr_set_pattern() have
 *   completed. This function performs the actual noise reduction process
 *   after which calls to bnr_get_token() may be called.
 *
 * parameters:	BTX (BNR_CTX *)	The noise reduction context to use
 * returns:	0 on success
 */

int bnr_finalize(BNR_CTX *BTX) {
  int BNR_SIZE = BTX->window_size;
  struct bnr_list_node * previous_bnr_tokens[BNR_SIZE];
  float previous_bnr_probs[BNR_SIZE];
  struct bnr_list_node *node_list;
  struct bnr_list_c c_list;
  char bnr_token[64];
  int i, interesting;

  for(i=0;i<BNR_SIZE;i++) {
    previous_bnr_probs[i] = 0.00000;
    previous_bnr_tokens[i] = NULL;
  } 

  node_list = c_bnr_list_first(BTX->stream, &c_list);
  while(node_list != NULL) {
    float pattern_value;

    for(i=0;i<BNR_SIZE-1;i++) {
      previous_bnr_probs[i] = previous_bnr_probs[i+1];
      previous_bnr_tokens[i] = previous_bnr_tokens[i+1];
    }

    previous_bnr_probs[BNR_SIZE-1] = _bnr_round(node_list->value);
    previous_bnr_tokens[BNR_SIZE-1] = node_list;

    sprintf(bnr_token, "bnr.%c|", BTX->identifier);
    for(i=0;i<BNR_SIZE;i++) {
      char x[6];
      snprintf(x, 6, "%01.2f_", previous_bnr_probs[i]);
      strcat(bnr_token, x);
    }

    /* Identify interesting patterns */
    
    pattern_value = bnr_hash_value(BTX->patterns, bnr_token);
    interesting = (fabs(0.5-pattern_value) > BTX->ex_radius);

    if (interesting) {

#ifdef LIBBNR_VERBOSE_DEBUG
      fprintf(stderr, "Analyzing Pattern '%s' P-Value: %1.5f\n", bnr_token, 
        pattern_value);
#endif

      /* Eliminate inconsistent tokens */
      for(i=0;i<BNR_SIZE;i++) {
        if (previous_bnr_tokens[i]) {

          /* If the token is inconsistent with the current pattern */
          if (fabs(previous_bnr_tokens[i]->value - pattern_value) > BTX->in_radius)
          {
#ifdef LIBBNR_VERBOSE_DEBUG
            fprintf(stderr, "\tEliminating '%s' P-Value: %1.5f\n", 
              (const char *) previous_bnr_tokens[i]->ptr, 
              previous_bnr_tokens[i]->value);
#endif
            BTX->eliminations++;
            previous_bnr_tokens[i]->eliminated = 1;
          }
        }
      }
    }

    node_list = c_bnr_list_next(BTX->stream, &c_list);
  }

  return 0;
}

