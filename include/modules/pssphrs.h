/*-*- c++ -*-********************************************************
 * Project:   M                                                     *
 * File name: pssphrs.h: Passphrase handling classes definition     *
 * Author:    Carlos Henrique Bauer                                 *
 * Modified by:                                                     *
 * CVS-ID:    $Id$    *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                        <bauer@atlas.unisinos.br> *
 * License: M license                                               *
 *******************************************************************/

#ifndef EXPERIMENTAL

#ifndef PSSPHRS_H
#define PSSPHRS_H

#ifndef _WX_WXSTRINGH__
#include <wx/string.h>
#endif // _WX_WXSTRINGH__

#ifdef __GNUG__
#pragma interface "pssphrs.h"
#endif // __GNUG__


class MDecryptedPassPhrase;


class MPassPhrase
{
public:
    MPassPhrase(bool autoMode=TRUE);
    MPassPhrase(const MPassPhrase & inPassPhrase, bool autoMode=TRUE);
    MPassPhrase(const MDecryptedPassPhrase & inPassPhrase, bool autoMode=TRUE);
    ~MPassPhrase();

    void Copy (const MPassPhrase & inPassPhrase);

    void SetAutoMode(bool autoMode = TRUE);

    void SetInputDialogCaption(const wxString & caption);
    void SetInputDialogPrompt(const wxString & prompt);
    
    void Set(const MDecryptedPassPhrase & inPassPhrase);
    int Get(MDecryptedPassPhrase & outPassPhrase);
    
    void Forget();

    bool IsSet() const;
    
    MPassPhrase & operator = (
	const MDecryptedPassPhrase & inPassPhrase);
    MPassPhrase & operator = (const MPassPhrase & inPassPhrase);
    
    static void Burn(char * inPassPhrase, int inPassPhraseLen);

protected:    
    void EncryptPassPhrase(char * outPassPhrase,
			   int outPassPhaseLen,
			   const char * inPassPhrase) const;

    void DecryptPassPhrase(char * outPassPhrase,
			   int outPassPhraseLen,
			   const char * inPassPhrase) const;
    
protected:
    char m_passPhrase[1025];
    wxString m_inputDialogCaption;
    wxString m_inputDialogPrompt;
    bool m_passPhraseSet;
    bool m_autoMode;
};


/*******************************************************************
* A class that erases the passphrase when is destructed
*******************************************************************/

class MDecryptedPassPhrase
{
    friend MPassPhrase;
    
public:
    MDecryptedPassPhrase();
    ~MDecryptedPassPhrase();

    void Forget();

    bool GetPassPhraseFromUser(const wxString & inputDialogCaption,
			       const wxString & inputDialogPrompt);
    
    int Len() const;
    int Length() const;
    
    operator const char *();
    const char * c_str();

    MDecryptedPassPhrase & operator =(const char * inPassPhrase);

    void Set(const char * inPassPhrase);
    void Set(const MPassPhrase & inPassPhrase);

protected:
    char m_passPhrase[1025];
};


inline MPassPhrase::~MPassPhrase()
{
    Forget();
}


inline bool MPassPhrase::IsSet() const
{
    return m_passPhraseSet;
}


inline void MPassPhrase::SetAutoMode(bool autoMode)
{
    m_autoMode = autoMode;
}


inline void MPassPhrase::SetInputDialogCaption(
    const wxString & caption)
{
    m_inputDialogCaption = caption;
}


inline void MPassPhrase::SetInputDialogPrompt(
    const wxString & prompt)
{
    m_inputDialogPrompt = prompt;
}


inline void MPassPhrase::Set(const MDecryptedPassPhrase & inPassPhrase)
{
    EncryptPassPhrase(m_passPhrase, sizeof(m_passPhrase),
		      inPassPhrase.m_passPhrase);
    m_passPhraseSet = true;
}


inline MPassPhrase & MPassPhrase::operator = (
    const MDecryptedPassPhrase & inPassPhrase)
{
    Set(inPassPhrase);
    return *this;
}


inline MPassPhrase & MPassPhrase::operator =(
    const MPassPhrase & inPassPhrase)
{
    Copy(inPassPhrase);
    return *this;
}


inline void MPassPhrase::Burn(char * inPassPhrase,
			      int inPassPhraseLen)
{
    memset(inPassPhrase, '\0', inPassPhraseLen);
}


inline MDecryptedPassPhrase::MDecryptedPassPhrase()
{
    *m_passPhrase = '\0';
}


inline MDecryptedPassPhrase::~MDecryptedPassPhrase()
{
    Forget();
}


inline void MDecryptedPassPhrase::Forget()
{
    MPassPhrase::Burn(m_passPhrase, sizeof(m_passPhrase));
}


inline int MDecryptedPassPhrase::Len() const
{
    return strlen(m_passPhrase);
}


inline int MDecryptedPassPhrase::Length() const
{
    return Len();
}


inline MDecryptedPassPhrase::operator const char *()
{
    return m_passPhrase;
}


inline const char * MDecryptedPassPhrase::c_str()
{
    return m_passPhrase;
}


inline MDecryptedPassPhrase & MDecryptedPassPhrase::operator =(
    const char * inPassPhrase)
{
    strncpy(m_passPhrase, inPassPhrase, sizeof(m_passPhrase));
    return *this;
}


#endif // PSSPHRS_H
#endif // EXPERIMENTAL
