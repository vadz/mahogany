// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M
// File name:   gui/wxOptionsDlg.h - functions to work with the options dialog
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _WXOPTIONSDLG_H
#define   _WXOPTIONSDLG_H

// -----------------------------------------------------------------------------
// fwd decls
// -----------------------------------------------------------------------------

class wxFrame;
struct wxOptionsPageDesc;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

/// the ids of option dialogs pages
enum OptionsPage
{
   OptionsPage_Default = -1,     // open at default page, must be -1
   OptionsPage_Ident,
   OptionsPage_Network,
   OptionsPage_Compose,
   OptionsPage_Folders,
#ifdef USE_PYTHON
   OptionsPage_Python,
#endif
   OptionsPage_MessageView,
   OptionsPage_Adb,
   OptionsPage_Helpers,
   OptionsPage_Misc,
   OptionsPage_Max
};

// -----------------------------------------------------------------------------
// functions
// -----------------------------------------------------------------------------

/// creates and shows the (modal) options dialog
extern void ShowOptionsDialog(wxFrame *parent = NULL,
                              OptionsPage page = OptionsPage_Default);

/// creates and shows the dialog allowing to restore default settings
extern bool ShowRestoreDefaultsDialog(ProfileBase *profile, wxFrame *parent);

/// creates and shows the options dialog with the given options page
extern void ShowCustomOptionsDialog(const wxOptionsPageDesc& pageDesc,
                                    ProfileBase *profile = NULL,
                                    wxFrame *parent = NULL);

#endif  //_WXOPTIONSDLG_H
