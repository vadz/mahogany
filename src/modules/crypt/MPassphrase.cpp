/*-*- c++ -*-*********************************************************
 * Project    : M                                                    *
 * File name  : pssphrs.cpp: Passphrase handling classes             *
 * Author     : Carlos Henrique Bauer                                *
 * Modified by:                                                      *
 * CVS-ID     : $Id$  *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>        *
 *                                          <bauer@atlas.unisinos.br>*
 * License    : M license                                            *
 *********************************************************************/


#ifdef EXPERIMENTAL_crypto

#include <wx/intl.h>
#include <wx/textdlg.h>

#ifdef __GNUG__
#pragma implementation "MPassphrase.h"
#endif // __GNUG__

#include "modules/MCrypt.h"
#include "modules/MPassphrase.h"


static const char * defaultInputDialogCaption =
   gettext_noop("Please enter your passphrase");

static const char * defaultDialogPrompt = gettext_noop( "Input passphrase");


MPassphrase::MPassphrase(bool autoMode) :
   m_inputDialogCaption(defaultInputDialogCaption),
   m_inputDialogPrompt(defaultDialogPrompt),
   m_passphraseSet(false),
   m_autoMode(autoMode)
{
}


MPassphrase::MPassphrase(const MPassphrase & inPassphrase,
			 bool autoMode) :
   m_passphraseSet(false),
   m_autoMode(autoMode)
{
   Copy(inPassphrase);
}


MPassphrase::MPassphrase(const MDecryptedPassphrase &
			 inPassphrase,
			 bool autoMode) :
   m_passphraseSet(false),
   m_autoMode(autoMode)
{
   Set(inPassphrase);
}


void MPassphrase::Copy (const MPassphrase & inPassphrase)
{
   if(inPassphrase.m_passphraseSet)
   {
      m_passphraseSet = true;
      memcpy(m_passphrase, inPassphrase.m_passphrase,
	     sizeof(m_passphrase));
   }
}


void MPassphrase::Forget()
{
   if(m_passphraseSet)
   {
      Burn(m_passphrase, sizeof(m_passphrase));
      m_passphraseSet = false;
   }
}


int MPassphrase::Get(MDecryptedPassphrase & outPassphrase)
{
   outPassphrase.Forget();

   if(m_passphraseSet)
   {
      DecryptPassphrase(outPassphrase.m_passphrase,
			sizeof(outPassphrase.m_passphrase),
			m_passphrase);
   } else
   {
      if(m_autoMode)
      {
	 // get the passphrase from the user
	 if(!outPassphrase.GetPassphraseFromUser(m_inputDialogCaption,
						 m_inputDialogPrompt))
	 {
	    return MCrypt::OPERATION_CANCELED_BY_USER;
	 }
	    
	 // remember the passphrase
	 Set(outPassphrase);
      } else
      {
	 return MCrypt::NO_PASSPHRASE;
      }
   }
   return MCrypt::OK;
}


void MPassphrase::EncryptPassphrase(
   char * outPassphrase,
   int outPassphraseLen,
   const char * inPassphrase) const
{
   // TODO: Create a real encryption schema
   strncpy(outPassphrase, inPassphrase, outPassphraseLen);
}


void MPassphrase::DecryptPassphrase(
   char * outPassphrase,
   int outPassphraseLen,
   const char * inPassphrase) const
{
   // TODO: Create a real decryption schema
   strncpy(outPassphrase, inPassphrase, outPassphraseLen);
}


void MDecryptedPassphrase::Set(const char * inPassphrase)
{
   strncpy(m_passphrase, inPassphrase, sizeof(m_passphrase));
}


bool MDecryptedPassphrase::GetPassphraseFromUser(
   const wxString & inputDialogCaption,
   const wxString & inputDialogPrompt)
{
   const wxString & decryptedPassphrase = ::wxGetPasswordFromUser(
      _(inputDialogPrompt), _(inputDialogCaption));

   // according to the ::wxGetPasswordFromUser documentation
   // a zero len string means the input was canceled by the user
   if(!decryptedPassphrase.Len())
   {
      return FALSE;
   }
    
   strncpy(m_passphrase, (char *) decryptedPassphrase.c_str(),
	   sizeof(m_passphrase));

   MPassphrase::Burn((char *)decryptedPassphrase.c_str(),
		     decryptedPassphrase.Len());

   return TRUE;
}


#endif // EXPERIMENTAL_crypto
