/*-*- c++ -*-*******************************************************
 * Project:   M                                                    *
 * File name: MCrypt.h: crypto classes implementation              *
 * Author:    Carlos Henrique Bauer                                *
 * Modified by:                                                    *
 * CVS-ID:    $Id$   *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>        *
 *                                        <bauer@atlas.unisinos.br>*
 * License: M license                                              *
 ******************************************************************/

#include <wx/intl.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#ifdef __GNUG__
#pragma implementation "MCrypt.h"
#endif

#include "MCrypt.h"

wxString MCrypt::ms_comment(gettext_noop("Mahogany\\ crypto\\ library"));


/*
int MCryptIsEncrypted(const wxString & messageIn)
{
// PGP encrypted messsage
//-----BEGIN PGP MESSAGE-----  
}


int MCrypt::IsSigned(const wxString & messageIn)
{
// PGP signed message
//-----BEGIN PGP SIGNED MESSAGE-----
}
*/
