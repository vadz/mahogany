/*-*- c++ -*-**********************************************************
 * Project    : M                                                     *
 * File name  : MPassphrase.h: Passphrase handling classes definition *
 * Author     : Carlos Henrique Bauer                                 *
 * Modified by:                                                       *
 * CVS-ID     : $Id$     *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                          <bauer@atlas.unisinos.br> *
 * License    : M license                                             *
 *********************************************************************/

#ifdef EXPERIMENTAL

#ifndef MPASSPHRASE_H
#define MPASSPHRASE_H

#ifndef _WX_WXSTRINGH__
#include <wx/string.h>
#endif // _WX_WXSTRINGH__

#ifdef __GNUG__
#pragma interface "MPassphrase.h"
#endif // __GNUG__


class MDecryptedPassphrase;


class MPassphrase
{
public:
   MPassphrase(bool autoMode = TRUE);
   MPassphrase(const MPassphrase & inPassphrase, bool autoMode = TRUE);
   MPassphrase(const MDecryptedPassphrase & inPassphrase,
	       bool autoMode = TRUE);
   ~MPassphrase();

   void Copy (const MPassphrase & inPassphrase);

   void SetAutoMode(bool autoMode = TRUE);

   void SetInputDialogCaption(const wxString & caption);
   void SetInputDialogPrompt(const wxString & prompt);
    
   void Set(const MDecryptedPassphrase & inPassphrase);
   int Get(MDecryptedPassphrase & outPassphrase);
    
   void Forget();

   bool IsSet() const;
    
   MPassphrase & operator = (const MDecryptedPassphrase &
			     inPassphrase);

   MPassphrase & operator = (const MPassphrase & inPassphrase);
    
   static void Burn(char * inPassphrase, int inPassphraseLen);

protected:    
   void EncryptPassphrase(char * outPassphrase,
			  int outPassPhaseLen,
			  const char * inPassphrase) const;

   void DecryptPassphrase(char * outPassphrase,
			  int outPassphraseLen,
			  const char * inPassphrase) const;
    
protected:
   char m_passphrase[1025];
   wxString m_inputDialogCaption;
   wxString m_inputDialogPrompt;
   bool m_passphraseSet;
   bool m_autoMode;
};


/*******************************************************************
* A class that erases the passphrase when is destructed
*******************************************************************/

class MDecryptedPassphrase
{
   friend MPassphrase;
    
public:
   MDecryptedPassphrase();
   ~MDecryptedPassphrase();

   void Forget();

   bool GetPassphraseFromUser(const wxString & inputDialogCaption,
			      const wxString & inputDialogPrompt);
    
   size_t Len() const;
    
   operator const char *();
   const char * c_str();

   MDecryptedPassphrase & operator =(const char * inPassphrase);

   void Set(const char * inPassphrase);
   void Set(const MPassphrase & inPassphrase);

protected:
   char m_passphrase[1025];
};


inline MPassphrase::~MPassphrase()
{
   Forget();
}


inline bool MPassphrase::IsSet() const
{
   return m_passphraseSet;
}


inline void MPassphrase::SetAutoMode(bool autoMode)
{
   m_autoMode = autoMode;
}


inline void MPassphrase::SetInputDialogCaption(
   const wxString & caption)
{
   m_inputDialogCaption = caption;
}


inline void MPassphrase::SetInputDialogPrompt(
   const wxString & prompt)
{
   m_inputDialogPrompt = prompt;
}


inline void MPassphrase::Set(const MDecryptedPassphrase & inPassphrase)
{
   EncryptPassphrase(m_passphrase, sizeof(m_passphrase),
		     inPassphrase.m_passphrase);
   m_passphraseSet = true;
}


inline MPassphrase & MPassphrase::operator = (
   const MDecryptedPassphrase & inPassphrase)
{
   Set(inPassphrase);
   return *this;
}


inline MPassphrase & MPassphrase::operator =(
   const MPassphrase & inPassphrase)
{
   Copy(inPassphrase);
   return *this;
}


inline void MPassphrase::Burn(char * inPassphrase,
			      int inPassphraseLen)
{
   memset(inPassphrase, '\0', inPassphraseLen);
}


inline MDecryptedPassphrase::MDecryptedPassphrase()
{
   *m_passphrase = '\0';
}


inline MDecryptedPassphrase::~MDecryptedPassphrase()
{
   Forget();
}


inline void MDecryptedPassphrase::Forget()
{
   MPassphrase::Burn(m_passphrase, sizeof(m_passphrase));
}


inline size_t MDecryptedPassphrase::Len() const
{
   return strlen(m_passphrase);
}


inline MDecryptedPassphrase::operator const char *()
{
   return m_passphrase;
}


inline const char * MDecryptedPassphrase::c_str()
{
   return m_passphrase;
}


inline MDecryptedPassphrase & MDecryptedPassphrase::operator =(
   const char * inPassphrase)
{
   strncpy(m_passphrase, inPassphrase, sizeof(m_passphrase));
   return *this;
}


#endif // MPASSPHRASE_H
#endif // EXPERIMENTAL
