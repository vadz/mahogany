///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxSelectionDlg.h
// Purpose:     wxSelectionsOrderDialog and derived classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.03.02 (extracted from the other files)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2002 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXSELECTIONDLG_H
#define _GUI_WXSELECTIONDLG_H

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// wxSelectionsOrderDialog
// ----------------------------------------------------------------------------

// helper class used by MDialog_GetSelectionsInOrder and
// wxMsgViewHeadersDialog elsewhere
class wxSelectionsOrderDialog : public wxManuallyLaidOutDialog
{
public:
   wxSelectionsOrderDialog(wxWindow *parent,
                           const wxString& message,
                           const wxString& caption,
                           const wxString& profileKey);

   // did anything really changed?
   bool HasChanges() const { return m_hasChanges; }

   // transfer data to/from window - must be overridden to populate the check
   // list box and retrieve the results from it
   virtual bool TransferDataToWindow() = 0;
   virtual bool TransferDataFromWindow() = 0;

   // implementation only from now on

   // check list box event handler
   void OnCheckLstBoxToggle(wxCommandEvent& event) { m_hasChanges = TRUE; }

   // update UI: disable the text boxes which shouldn't be edited
   void OnUpdateUI(wxUpdateUIEvent& event);

   // up/down buttons notifications
   void OnButtonUp(wxCommandEvent& event) { OnButtonMove(TRUE); }
   void OnButtonDown(wxCommandEvent& event) { OnButtonMove(FALSE); }

protected:
   // real button events handler
   void OnButtonMove(bool up);

   wxStaticBox    *m_box;

   wxCheckListBox *m_checklstBox;

   bool m_hasChanges;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxSelectionsOrderDialogSimple
// ----------------------------------------------------------------------------

// a simple class deriving from wxSelectionsOrderDialog which just passes the
// strings around
class wxSelectionsOrderDialogSimple : public wxSelectionsOrderDialog
{
public:
   wxSelectionsOrderDialogSimple(const wxString& message,
                                 const wxString& caption,
                                 wxArrayString* choices,
                                 wxArrayInt* status,
                                 const wxString& profileKey,
                                 wxWindow *parent)
      : wxSelectionsOrderDialog(parent, message, caption, profileKey)
   {
      m_choices = choices;
      m_status = status;
   }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   wxArrayString *m_choices;
   wxArrayInt    *m_status;
};

#endif // _GUI_WXSELECTIONDLG_H

