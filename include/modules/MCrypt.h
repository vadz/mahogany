/*-*- c++ -*-**********************************************************
 * Project    : M                                                     *
 * File name  : MCrypt.h: Crypto base classes                         *
 * Author     : Carlos Henrique Bauer                                 *
 * Modified by:                                                       *
 * CVS-ID     : $Id$      *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                          <bauer@atlas.unisinos.br> *
 * License    : M license                                             *
 **********************************************************************/

#ifdef EXPERIMENTAL_crypto

#ifndef MCRYPT_H
#define MCRYPT_H

#ifndef MPASSPHRASE_H
#include "modules/MPassphrase.h"
#endif // MPASSPHRASE_H

#ifdef __GNUG__
#pragma interface "MCrypt.h"
#endif // __GNUG__

#ifndef _WX_PROCESSH__
#include <wx/process.h>
#endif // _WX_PROCESSH__

// MCRYPT Trace Mask, for use in wxLogTrace
#define MCRYPT_TRM "MCrypt"

/** Definition of the crypto interface.
    This class define the interface for all email crypto methods.

    @doc
    Here is an example of how to use a derived class:

    \begin{verbatim}
    MPassphrase passphrase(TRUE);
    wxString user("joe@domain");
    
    MPGPCrypt pgp();

    wxString msgTextIn("A message");
    wxString msgTextOut;
    
    int status;

    status  = pgp.Sign(user, passphrase, msgTextIn, msgTextOut);
    \end{verbatim}

    The crypto classes should not make copies of an user email address
    and/or its passphrase. The crypto classes are utility
    classes and are supposed to be transient.

    A programmer may give the users of his/her program the ability to
    set several email identities. He/she can associate a passphrase
    to each identity. The programmer is responsible for keeping that
    association not the crypto classes.

    {\bf BEGIN:}
    
    A message can have included:

    \begin{itemize}
       \item signed message
       \item encrypted messages
       \item encrypted and signed messages
       \item public keys
    \end{itemize}

    Signed, encrypted, encrypted and signed messages can have included:

    \begin{itemize}
       \item goto BEGIN; ;)
    \end{itemize}

    When we:
    a) decrypt, verify a signature, or
    b) check if a message is signed, encrypted or have public keys,
    we must scan almost every line of that message.

    Of course we usually do b) before a). We can choose if we want do
    repeat the b) process every time we show a message to the user of
    just in the first time we see that message. The old {\bf speed X
    memory} tradeoff issue.

    What we must cache about each message part:
    
    \begin{verbatim}
                        +----------------------+
                        |       Value          |
    +-------------------+----------------------+
    | Part Type         | begin | end | signer |
    +-------------------+-------+-----+--------+
    | encrypted message |   X   |  X  |        |
    | signed message    |   X   |  X  |   X    |
    | public key        |   X   |  X  |        |
    +-------------------+-------+-----+--------+
    \end{verbatim}

    @see MPassphrase
*/


class MCrypt
{
public:

   // MCrypt functions return status values

   // temporary values

   enum {
      OK = 0,
      CANNOT_EXEC_PROGRAM,
      OPERATION_CANCELED_BY_USER,
      NO_PASSPHRASE,
      OUT_OF_ENV_SPACE_ERROR,
      BATCH_MODE_ERROR,
      BAD_ARGUMENT_ERROR,
      PROCESS_INTERRUPTED_ERROR,
      OUT_OF_MEM_ERROR,
      NONEXISTING_KEY_ERROR,
      KEYRING_ADD_ERROR,
      KEYRING_EXTRACT_ERROR,
      KEYRING_EDIT_ERROR,
      KEYRING_VIEW_ERROR,
      KEYRING_CHECK_ERROR,
      KEYRING_SIGNATURE_ERROR,
      SIGNATURE_ERROR,
      PUBLIC_KEY_ENCRIPTION_ERROR,
      ENCRYPTION_ERROR,
      COMPRESSION_ERROR,
      SIGNATURE_CHECK_ERROR,
      PUBLIC_KEY_DECRIPTION_ERROR,
      DECRIPTION_ERROR,
      DECOMPRESSION_ERROR
   };


   // Encryption methods IDs
   enum {
      M_PGP = 1
   };

public:

   /// Default constructor.
   MCrypt();

   /// Virtual destructor
   virtual ~MCrypt();

   virtual int Decrypt(MPassphrase & passphrase,
		       const wxString & messageIn,
		       wxString & messageOut) = 0;

   /**
      
      This message encrypts and signs a message.

      @param recipient The ID of the public key used to encrypt the
      message. Usually is the email address of the recipient.

      @param messageIn The clear text message to be signed.

      @param messageOut A buffer where the encrypted message will be stored.

      @param user A pointer to the Id of the private key of the
      sender, usually is the email address used to create that key.

      @param passphrase A pointer of the passphrase for unlocking the
      private key.

      @doc
      To sign the message the function must receives valid
      {\bf user} and {\bf passphrase} arguments. If one of them is
      NULL message signing will be disabled.
      
   */

   virtual int Encrypt(const wxString & recipient,
		       const wxString & messageIn,
		       wxString & messageOut,
		       const wxString * user = NULL,
		       MPassphrase * passphrase = NULL ) = 0;

   virtual int Sign(const wxString & user,
		    MPassphrase & passphrase,
		    const wxString & messageIn,
		    wxString & messageOut) = 0;

   virtual int VerifySignature(const wxString & messageIn, wxString &
			       messageOut) const = 0;

   virtual int CheckRecipient(const wxString & recipient) const = 0;

   virtual int GetFingerprint(wxString & fp) const = 0;

   virtual int GetPublicKey(wxString & pk) const = 0;

   static void SetComment(const wxString & comment);

   static const wxString & GetComment();

   static int IsEncrypted(const wxString & messageIn);
   static int IsSigned(const wxString & messageIn);


protected:
   static wxString ms_comment;
};


inline MCrypt::MCrypt()
{
}



inline MCrypt::~MCrypt()
{
}


inline const wxString & MCrypt::GetComment()
{
   return ms_comment;
}


inline void MCrypt::SetComment(const wxString & comment)
{
   // PGP doesn't like not escaped spaces
   (ms_comment = comment).Replace(" ", "\\ ", TRUE);
}


#endif // MCRYPT_H
#endif // EXPERIMENTAL_crypto
