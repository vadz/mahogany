///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxIdentityCombo.h - combobox for selecting an identity
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.07.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXIDENTITYCOMBO_H_
#define _GUI_WXIDENTITYCOMBO_H_

class WXDLLEXPORT wxChoice;
class WXDLLEXPORT wxWindow;

/// the id of the identity combobox (always the same)
enum
{
    IDC_IDENT_COMBO = wxID_LOWEST - 1
};

/// create the identity combobox, may return NULL if no identities defined
extern wxChoice *CreateIdentCombo(wxWindow *parent);

//(implemented in src/gui/wxMDialogs.cpp)

#endif // _GUI_WXIDENTITYCOMBO_H_
