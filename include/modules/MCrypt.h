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

#ifdef EXPERIMENTAL

#ifndef MCRYPT_H
#define MCRYPT_H

#ifndef MPASSPHRASE_H
#include "MPassphrase.h"
#endif // MPASSPHRASE_H

#ifdef __GNUG__
#pragma interface "MCrypt.h"
#endif // __GNUG__

#ifndef _WX_PROCESSH__
#include <wx/process.h>
#endif // _WX_PROCESSH__


#ifdef __WXDEBUG__
// MCRYPT Trace Mask, for use in wxLogTrace
#define MCRYPT_TRM "MCrypt"
#endif // __WXDEBUG__


/** Definition of the crypto interface.
    This class define the interface for all email crypto methods.

    @doc
    Here is an example of how to use it:

    <pre>
    MPassphrase passphrase(TRUE);
    wxString user("joe@domain");
    
    MPGPCrypt pgp(user, passphrase);

    wxString msgTextIn("A message");
    wxString msgTextOut;
    
    int status;

    status  = pgp.Sign(msgTextIn, msgTextOut);
    </pre>

    The crypto classes should not make copies of an user email address
    and/or its passphrase. The crypto classes are utility
    classes and are supposed to be transient.

    A programmer may give the users of his/her program the ability to
    set several email identities. He/she can associate a passphrase
    to each identity. The programmer is responsible for keeping that
    association not the crypto classes.

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

   MCrypt(const wxString & user, MPassphrase & passphrase);
   virtual ~MCrypt();

   virtual int Encrypt(const wxString & recipient,
		       const wxString & messageIn,
		       wxString & messageOut,
		       bool sign = TRUE) = 0;

   virtual int VerifySignature(const wxString & messageIn, wxString &
			       messageOut) const = 0;

   virtual int Sign(const wxString & messageIn,
		    wxString & messageOut) = 0;

   virtual int CheckRecipient(const wxString & recipient) const = 0;

   virtual int Decrypt(const wxString & messageIn,
		       wxString & messageOut) = 0;

   virtual int GetFingerprint(wxString & fp) const = 0;

   virtual int GetPublicKey(wxString & pk) const = 0;

   static void SetComment(const wxString & comment);

   static const wxString & GetComment();

   static int IsEncrypted(const wxString & messageIn);
   static int IsSigned(const wxString & messageIn);


protected:
   static wxString ms_comment;

   const wxString & m_user;
   MPassphrase &  m_passphrase;

private:
   // declared but not defined
   MCrypt();
};


inline MCrypt::MCrypt(const wxString & user, MPassphrase & passphrase) :
   m_user(user), m_passphrase(passphrase)
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
#endif // EXPERIMENTAL
