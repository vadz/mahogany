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
   void OnCheckLstBoxToggle(wxCommandEvent&) { m_hasChanges = TRUE; }

   // update UI here: disable the buttons when they don't do anything
   void OnCheckLstBoxSelChanged(wxCommandEvent& event);

   // up/down buttons notifications
   void OnButtonUp(wxCommandEvent&) { OnButtonMove(TRUE); }
   void OnButtonDown(wxCommandEvent&) { OnButtonMove(FALSE); }

protected:
   // can be overridden if the derived class wants to know when the 2 items in
   // the check list box are exchanged (after this has happened)
   virtual void OnItemSwap(size_t /* item1 */, size_t /* item2 */) { }

   // real button events handler
   void OnButtonMove(bool up);

   // update the buttons state to reflect the selection in the checklistbox
   void UpdateButtons(int sel);

   // the box around all our controls (here for wxFolderViewColumnsDialog only)
   wxStaticBox    *m_box;

   // the buttons to move the items in m_checklstBox up and down
   wxButton       *m_btnUp,
                  *m_btnDown;

   // the check list box containing the items we reorder
   wxCheckListBox *m_checklstBox;

   // the dirty flag
   bool m_hasChanges;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxSelectionsOrderDialog)
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

   DECLARE_NO_COPY_CLASS(wxSelectionsOrderDialogSimple)
};

#endif // _GUI_WXSELECTIONDLG_H

