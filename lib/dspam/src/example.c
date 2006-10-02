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
 * example.c - example of libdspam implementation
 *
 * DESCRIPTION
 *
 * 
 * compile with:
 * gcc -o example example.c -ldspam -L.libs -I. -DHAVE_CONFIG_H \
 *   -DCONFIG_DEFAULT=/usr/local/etc/dspam.conf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libdspam.h>

#define USERNAME	"testuser"

int
main (int argc, char **argv)
{
  char dspam_home[] = "/var/dspam";     /* Our dspam data home */
  char buffer[1024];
  char *message = malloc (1);
  long len = 1;

  DSPAM_CTX *CTX;               	/* DSPAM Context */
  struct _ds_spam_signature SIG;        /* Example signature */

  /*
     Performs any driver-specific startup functions (such as initializing 
     internal lock tables).  Call this only once when your application
     starts
  */

  dspam_init_driver (NULL);

  /* read in the message from stdin */
  message[0] = 0;
  while (fgets (buffer, sizeof (buffer), stdin) != NULL)
  {
    len += strlen (buffer);
    message = realloc (message, len);
    if (message == NULL)
    {
      fprintf (stderr, "out of memory!");
      exit (EXIT_FAILURE);
    }
    strcat (message, buffer);
  }

  /* MESSAGE PROCESSING */

  /* initialize dspam 

    Operating Modes:

    DSM_PROCESS:	Process message
    DSM_CLASSIFY:	Classify message only (do not write changes)

    Flags:

    DSF_SIGNATURE	Signature Mode (Use a signature)
    DSF_NOISE		Use Bayesian Noise Reduction
    DSF_WHITELIST	Use Automatic Whitelisting

    Tokenizers:

    DSZ_WORD		Use WORD tokenizer
    DSZ_CHAIN		Use CHAIN tokenizer
    DSZ_SBPH		Use SBPH tokenizer
    DSZ_OSB		Use OSB tokenizer

    Training Modes:

    DST_TEFT		Train Everything
    DST_TOE		Train-on-Error
    DST_TUM		Train-until-Mature

    Classifications:

    Used to tell libdspam the message has already been classified, and should
    be processed in such a way that the result will match the classification.

    DSR_ISSPAM		Message is spam (learn as spam)
    DSR_ISINNOCENT	Message is innocent (learn as innocent)
    DSR_NONE		No predetermined classification (classify message)

    Sources:

    Used to tell libdspam the source of the specified classification (if any).

    DSS_ERROR		Misclassification by libdspam
    DSS_CORPUS		Corpusfed message
    DSS_INOCULATION	Message inoculation
    DSS_NONE		No classification source (use only with DSR_NONE)

    NOTE: When using DSS_ERROR, a DSPAM signature should be provided, OR
          the original message in PRISTINE form (without any DSPAM headers and
          with the original message's headers).

  */ 

   /* --------------------------- EXAMPLE 1 ----------------------------*/
                      /* STANDARD INBOUND PROCESSING */

  /* In this example, DSF_SIGNATURE is specified to request that libdspam
     generate a signature, which can be stored internally for future retraining.
     This is useful if the original message in pristine form won't be available
     server-side should the user want to reclassify the message.
  */

  /* Initialize the DSPAM context */
  CTX = dspam_init (USERNAME, NULL, dspam_home, DSM_PROCESS,
                    DSF_SIGNATURE | DSF_NOISE);

  if (CTX == NULL)
  {
    fprintf (stderr, "ERROR: dspam_init failed!\n");
    exit (EXIT_FAILURE);
  }

  /* Use graham and robinson algorithms, graham's p-values */
  CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;

  /* Use CHAIN tokenizer */
  CTX->tokenizer = DSZ_CHAIN;

  /* Call DSPAM's processor with the message text */
  if (dspam_process (CTX, message) != 0)
  {
    fprintf (stderr, "ERROR: dspam_process failed");
    exit (EXIT_FAILURE);
  }

  /* Print processing results */
  printf ("Probability: %2.4f Confidence: %2.4f, Result: %s\n",
          CTX->probability,
          CTX->confidence,
          (CTX->result == DSR_ISSPAM) ? "Spam" : "Innocent");

  /* Manage signature */
  if (CTX->signature == NULL)
  {
    printf ("No signature provided\n");
  }
  else
  {
    /* Copy to a safe place */

    SIG.data = malloc (CTX->signature->length);
    if (SIG.data != NULL)
      memcpy (SIG.data, CTX->signature->data, CTX->signature->length);
  }
  SIG.length = CTX->signature->length;

  /* Destroy the context */
  dspam_destroy(CTX);

   /* --------------------------- EXAMPLE 2 ----------------------------*/
                 /* SPAM REPORTING (AS MISCLASSIFICATION) */

  /* We call everything just like before, with these exceptions:
     - We set the classification to DSR_ISSPAM
     - We set the source to DSS_ERROR

     This example will use the original message in pristine form instead of
     a signature.  See the next example (false positives) for an example using
     a signature.
  */

  /* Initialize the DSPAM context */
  CTX = dspam_init (USERNAME, NULL, dspam_home, DSM_PROCESS, 0);
  if (CTX == NULL)
  {
    fprintf (stderr, "ERROR: dspam_init failed!\n");
    exit (EXIT_FAILURE);
  }

  /* Set up the context for error correction as spam */
  CTX->classification = DSR_ISSPAM;
  CTX->source         = DSS_ERROR;

  /* Use graham and robinson algorithms, graham's p-values */
  CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;

  /* Use CHAIN tokenizer */
  CTX->tokenizer = DSZ_CHAIN;

  /* Call DSPAM */
  if (dspam_process(CTX, message) != 0)
  {
    fprintf (stderr, "ERROR: dspam_process failed\n");
    exit (EXIT_FAILURE);
  }

  /* Destroy the context */
  dspam_destroy (CTX);

  printf("Spam retrained successfully.\n");

   /* --------------------------- EXAMPLE 3 ----------------------------*/
                        /* FALSE POSITIVE REPORTING */

  /* Here we submit the message's signature for retraining as a false positive.
     We make the following changes from our original example: 
     - We set the classification to DSR_ISINNOCENT
     - We set the source to DSS_ERROR
     - We attach the binary signature to the context, and pass in a NULL text
  */

  /* Initialize DSPAM context */

  CTX = dspam_init (USERNAME, NULL, dspam_home, DSM_PROCESS, DSF_SIGNATURE);
  if (CTX == NULL)
  {
    fprintf (stderr, "ERROR: dspam_init failed!\n");
    exit (EXIT_FAILURE);
  }

  /* Set up the context for error correction as innocent */
  CTX->classification = DSR_ISINNOCENT;
  CTX->source         = DSS_ERROR;

  /* Use graham and robinson algorithms, graham's p-values */
  CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;

  /* Use CHAIN tokenizer */

  CTX->tokenizer = DSZ_CHAIN;

  /* Attach the signature to the context */
  CTX->signature = &SIG;

  /* Call DSPAM */
  if (dspam_process(CTX, NULL) != 0)
  {
    fprintf (stderr, "ERROR: dspam_process failed\n");
    exit (EXIT_FAILURE);
  }

  /* Destroy the context */
  dspam_destroy(CTX); 

  printf("False positive retrained successfully.\n");

  /* --------------------------- EXAMPLE 4 ----------------------------*/
             /* USE OF CREATE AND ATTACH WITH ATTRIBUTES API */
                                                                                
  /* If we want to provide libdspam with a set of storage driver attributes
     so that it doesn't have to go looking for a [driver].data file to get
     SQL-server information from, we can use the create/attach mode of
     preparing a context, rather than calling dspam_init()
  */
                                                                                
  /* Create the DSPAM context; called just like dspam_init() */

  CTX = dspam_create (USERNAME, NULL,  dspam_home, DSM_PROCESS, DSF_SIGNATURE);
  if (CTX == NULL)
  {
    fprintf (stderr, "ERROR: dspam_create failed!\n");
    exit (EXIT_FAILURE);
  }

  /* Now we have a context but it is not attached to the storage driver. The
     next step is to set up our set of attributes to tell the driver how to
     connect. In this example, we'll assume we're using a MySQL backend. 
  */

  dspam_addattribute(CTX, "MySQLServer", "127.0.0.1");
  dspam_addattribute(CTX, "MySQLPort", "3306");
  dspam_addattribute(CTX, "MySQLUser", "example");
  dspam_addattribute(CTX, "MySQLPass", "1234");
  dspam_addattribute(CTX, "MySQLDb", "dspam"); 

  /* Use graham and robinson algorithms, graham's p-values */
  CTX->algorithms = DSA_GRAHAM | DSA_BURTON | DSP_GRAHAM;

  /* Use CHAIN tokenizer */
  CTX->tokenizer = DSZ_CHAIN;

  /* Here, we can also pass in any other attributes used by libdspam
     (see dspam.conf). We can also do this after the attach, since they are
     not used until we process a message.
  */

  dspam_addattribute(CTX, "IgnoreHeader", "X-Virus-Scanner-Result");

  /* Now a call to dspam_attach() will connect our context to the storage 
     driver interface and establish a connection. Alternatively, if you have
     an open database handle you may pass it in as the second parameter and
     avoid opening a new database connection */

  if (dspam_attach(CTX, NULL)) {
    fprintf (stderr, "ERROR: dspam_attach failed!\n");
    exit(EXIT_FAILURE);
  }

  /* Then proceed like normal and when we're done, destroy the context like
     we normally do */

  dspam_destroy(CTX); 

  printf("Create/attach performed successfully.\n");

  /* Performs any driver-specific shutdown functions */
  dspam_shutdown_driver (NULL);

  exit (EXIT_SUCCESS);
}

