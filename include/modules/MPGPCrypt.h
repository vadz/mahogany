/*-*- c++ -*-*********************************************************
 * Project    : M                                                    *
 * File name  : MPGPCrypt.h: PGP crypto class definition              *
 * Author     : Carlos Henrique Bauer                                *
 * Modified by:                                                      *
 * CVS-ID     : $Id$   *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>        *
 *                                          <bauer@atlas.unisinos.br>*
 * License    : M license                                            *
 ********************************************************************/

#ifdef EXPERIMENTAL

#ifndef MPGPCRYPT_H
#define MPGPCRYPT_H

#ifndef MEXTPROCCRYPT_H
#include "modules/MExtProcCrypt.h"
#endif // MEXTPROCCRYPT_H

#ifdef __GNUG__
#pragma interface "MPGPCrypt.h"
#endif // __GNUG__

class MPGPCrypt : public MExtProcCrypt
{

public:
   MPGPCrypt();

   virtual int Decrypt(MPassphrase & passphrase,
		       const wxString & messageIn,
		       wxString & messageOut);

   virtual int Encrypt(const wxString & recipient,
		       const wxString & messageIn,
		       wxString & messageOut,
		       const wxString * user = NULL,
		       MPassphrase * passphrase = NULL );

   virtual int Sign(const wxString & user,
		    MPassphrase & passphrase,
		    const wxString & messageIn,
		    wxString & messageOut);

   virtual int VerifySignature(const wxString & messageIn, wxString &
			       messageOut) const;
    
   virtual int CheckRecipient(const wxString & recipient) const;
    

   virtual int GetFingerprint(wxString & fp) const;
   
   virtual int GetPublicKey(wxString & pk) const;
};

#endif // MPGPCRYPT_H
#endif // EXPERIMENTAL
