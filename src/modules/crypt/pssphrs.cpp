/*-*- c++ -*-********************************************************
 * Project:   M                                                     *
 * File name: pssphrs.cpp: Passphrase handling classes              *
 * Author:    Carlos Henrique Bauer                                 *
 * Modified by:                                                     *
 * CVS-ID:    $Id$    *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                        <bauer@atlas.unisinos.br> *
 * License: M license                                               *
 *******************************************************************/


#ifdef EXPERIMENTAL

#include <wx/intl.h>
#include <wx/textdlg.h>

#ifdef __GNUG__
#pragma implementation "pssphrs.h"
#endif // __GNUG__

#include <MCrypt.h>
#include <pssphrs.h>


static const char * defaultInputDialogCaption = _("Please enter your\
 PGP passphrase");
static const char * defaultDialogPrompt = _("Input passphrase");


MPassPhrase::MPassPhrase(bool autoMode) :
    m_inputDialogCaption(defaultInputDialogCaption),
    m_inputDialogPrompt(defaultDialogPrompt),
    m_passPhraseSet(false),
    m_autoMode(autoMode)
{
}


MPassPhrase::MPassPhrase(const MPassPhrase & inPassPhrase,
			 bool autoMode) :
    m_passPhraseSet(false),
    m_autoMode(autoMode)
{
    Copy(inPassPhrase);
}


MPassPhrase::MPassPhrase(const MDecryptedPassPhrase &
			 inPassPhrase,
			 bool autoMode) :
    m_passPhraseSet(false),
    m_autoMode(autoMode)
{
    Set(inPassPhrase);
}


void MPassPhrase::Copy (const MPassPhrase & inPassPhrase)
{
    if(inPassPhrase.m_passPhraseSet) {
	m_passPhraseSet = true;
	memcpy(m_passPhrase, inPassPhrase.m_passPhrase,
	       sizeof(m_passPhrase));
    }
}


void MPassPhrase::Forget()
{
    if(m_passPhraseSet) {
	Burn(m_passPhrase, sizeof(m_passPhrase));
	m_passPhraseSet = false;
    }
}


int MPassPhrase::Get(MDecryptedPassPhrase & outPassPhrase)
{
    outPassPhrase.Forget();

    if(m_passPhraseSet) {
	DecryptPassPhrase(outPassPhrase.m_passPhrase,
			  sizeof(outPassPhrase.m_passPhrase),
			  m_passPhrase);
    } else {
	if(m_autoMode) {
	    // get the passphrase from the user
	    if(!outPassPhrase.GetPassPhraseFromUser(m_inputDialogCaption,
						    m_inputDialogPrompt)) {
		return MCrypt::OPERATION_CANCELED_BY_USER;
	    }
	    
	    // remember the passphrase
	    Set(outPassPhrase);
	} else {
	    return MCrypt::NO_PASSPHRASE;
	}
    }
    return MCrypt::OK;
}


void MPassPhrase::EncryptPassPhrase(
    char * outPassPhrase,
    int outPassPhraseLen,
    const char * inPassPhrase) const
{
    // TODO: Create a real encryption schema
    strncpy(outPassPhrase, inPassPhrase, outPassPhraseLen);
}


void MPassPhrase::DecryptPassPhrase(
    char * outPassPhrase,
    int outPassPhraseLen,
    const char * inPassPhrase) const
{
    // TODO: Create a real decryption schema
    strncpy(outPassPhrase, inPassPhrase, outPassPhraseLen);
}


void MDecryptedPassPhrase::Set(const char * inPassPhrase)
{
    strncpy(m_passPhrase, inPassPhrase, sizeof(m_passPhrase));
}


bool MDecryptedPassPhrase::GetPassPhraseFromUser(
    const wxString & inputDialogCaption,
    const wxString & inputDialogPrompt)
{
    const wxString & decryptedPassPhrase = ::wxGetPasswordFromUser(
	inputDialogPrompt, inputDialogCaption);

    // according to the ::wxGetPasswordFromUser documentation
    // a zero len string means the input was canceled by the user
    if(!decryptedPassPhrase.Len()) {
	return FALSE;
    }
    
    strncpy(m_passPhrase, (char *) decryptedPassPhrase.c_str(),
	    sizeof(m_passPhrase));

    MPassPhrase::Burn((char *)decryptedPassPhrase.c_str(),
		      decryptedPassPhrase.Len());

    return TRUE;
}


#endif // EXPERIMENTAL
