///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsDlg.h - functions to work with the options dialog
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _WXOPTIONSDLG_H
#define   _WXOPTIONSDLG_H

// -----------------------------------------------------------------------------
// fwd decls
// -----------------------------------------------------------------------------
class wxFrame;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

/// the ids of option dialogs pages
enum OptionPage
{
   OptionPage_Default = -1,     // open at default page, must be -1
   OptionPage_Ident,
   OptionPage_Network = OptionPage_Ident, // so far...
   OptionPage_Compose,
   OptionPage_Folders,
#ifdef USE_PYTHON
   OptionPage_Python,
#endif
   OptionPage_Misc,
   OptionPage_Max
};

// -----------------------------------------------------------------------------
// functions
// -----------------------------------------------------------------------------

/// creates and shows the options dialog (dialog is modal)
void ShowOptionsDialog(wxFrame *parent = NULL,
                       OptionPage page = OptionPage_Default);

#endif  //_WXOPTIONSDLG_H
