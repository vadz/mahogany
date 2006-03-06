///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMimeDialog.cpp
// Purpose:     implements the MIME-related dialogs
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-09-27
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "guidef.h"

   #include <wx/layout.h>

   #include <wx/statbox.h>
   #include <wx/stattext.h>

   #include "strutil.h"
#endif // USE_PCH

#include "gui/wxDialogLayout.h"
#include "MimeDialog.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// wxMimeOpenWithDialog: the dialog class
// ----------------------------------------------------------------------------

class wxMimeOpenWithDialog : public wxManuallyLaidOutDialog
{
public:
   wxMimeOpenWithDialog(wxWindow *parent,
                        const String& mimetype,
                        bool *openAsMsg);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   const wxString& GetCommand() const { return m_command; }

protected:
   void OnCheckBox(wxCommandEvent& event);

private:
   // the value of m_txtCommand after the dialog is closed
   wxString m_command;

   // pointer to "open as message" flag, may be NULL
   bool *m_openAsMsg;

   // the GUI controls
   wxStaticText *m_labelCommand;
   wxPTextEntry *m_txtCommand;
   wxCheckBox *m_chkOpenAsMsg;


   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMimeOpenWithDialog)
};

BEGIN_EVENT_TABLE(wxMimeOpenWithDialog, wxManuallyLaidOutDialog)
   EVT_CHECKBOX(wxID_ANY, wxMimeOpenWithDialog::OnCheckBox)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMimeOpenWithDialog construction
// ----------------------------------------------------------------------------

wxMimeOpenWithDialog::wxMimeOpenWithDialog(wxWindow *parent,
                                           const String& mimetype,
                                           bool *openAsMsg)
                    : wxManuallyLaidOutDialog(parent,
                                              _("Open with"),
                                              _T("MimeOpenWithDialog"))
{
   // init the data
   // -------------

   m_openAsMsg = openAsMsg;

   // create the controls
   // -------------------

   // first the buttons and the optional checkbox
   wxStaticBox *box = CreateStdButtonsAndBox(wxEmptyString, StdBtn_NoBox);

   // next the controls which are always shown
   wxString msg = wxString::Format
                  (
                     _("Please enter the command to handle data of type \"%s\".\n"
                       "\n"
                       "The command may contain the string \"%%s\" which will "
                       "be replaced by the file name and if you don't specify it, "
                       "the file name will be appended at the end."),
                     mimetype.c_str()
                  );
   if ( m_openAsMsg )
   {
      msg += _T('\n');
      msg += _("or use the checkbox below to open it as a mail message in "
               "Mahogany itself");
   }

   wxStaticText *help = CreateMessage(this, msg, box);

   m_labelCommand = new wxStaticText(this, wxID_ANY, _("Open &with:"));

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.Below(help, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_labelCommand->SetConstraints(c);

   // we want to default to /Prompts/MimeHandler/Type/Subtype but also fall
   // back to /Prompts/MimeHandler/Type and even /Prompts/MimeHandler hence we
   // pass two extra colons at the end to tell wxPTextEntry to recurse upwards
   m_txtCommand = new wxPTextEntry
                      (
                        _T("/Prompts/MimeHandler/") + mimetype + _T("::"),
                        this,
                        wxID_ANY
                      );
   c = new wxLayoutConstraints;
   c->centreY.SameAs(m_labelCommand, wxCentreY);
   c->left.RightOf(m_labelCommand, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   m_txtCommand->SetConstraints(c);


   // finally the optional checkbox
   if ( m_openAsMsg )
   {
      m_chkOpenAsMsg = new wxCheckBox(this, wxID_ANY, _("Open as &mail message"));
      wxLayoutConstraints *c = new wxLayoutConstraints;
      c->top.Below(m_txtCommand, 3*LAYOUT_Y_MARGIN);
      c->centreX.SameAs(this, wxCentreX);
      c->width.AsIs();
      c->height.AsIs();

      m_chkOpenAsMsg->SetConstraints(c);
   }


   SetDefaultSize(6*wBtn, 10*hBtn);
}

// ----------------------------------------------------------------------------
// wxMimeOpenWithDialog data transfer
// ----------------------------------------------------------------------------

bool wxMimeOpenWithDialog::TransferDataToWindow()
{
   /*
      TODO: remember commands used for each MIME type and propose them by
            default the next time the dialog is used!
    */

   if ( m_openAsMsg )
      m_chkOpenAsMsg->SetValue(*m_openAsMsg);

   return true;
}

bool wxMimeOpenWithDialog::TransferDataFromWindow()
{
   if ( m_openAsMsg && m_chkOpenAsMsg->IsChecked() )
   {
      *m_openAsMsg = true;

      // leave m_command empty
   }
   else // don't open as mail message, use the command
   {
      // the command must contain either exactly one '%s' format specificator
      // or none at all
      wxString command = m_txtCommand->GetValue();
      String specs = strutil_extract_formatspec(command);
      if ( specs.empty() )
      {
         // at least the filename should be there!
         command += _T(" %s");
      }
      else if ( specs != _T('s') )
      {
         wxLogError(_("Please use at most one %% sign in the command string or "
                      "use %%%% if you need to insert an additional one."));

         return false;
      }

      m_command = command;
   }

   return true;
}

// ----------------------------------------------------------------------------
// wxMimeOpenWithDialog event handlers
// ----------------------------------------------------------------------------

void wxMimeOpenWithDialog::OnCheckBox(wxCommandEvent& event)
{
   bool enableText = !event.IsChecked();

   m_labelCommand->Enable(enableText);
   m_txtCommand->Enable(enableText);
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

String
GetCommandForMimeType(wxWindow *parent,
                      const String& mimetype,
                      bool *openAsMsg)
{
   wxMimeOpenWithDialog dlg(parent, mimetype, openAsMsg);

   String cmd;
   if ( dlg.ShowModal() == wxID_OK )
   {
      // don't leave cmd empty if the user selected to open as message or the
      // caller would think that the dialog was cancelled
      cmd = openAsMsg && *openAsMsg ? String(_T("M")) : dlg.GetCommand();
   }
   //else: cmd remains empty

   return cmd;
}

