///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxHeadersDialogs.cpp - dialogs to configure headers
// Purpose:     utility dialogs used from various places
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.04.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "MHelp.h"

#  include <wx/layout.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/checkbox.h>
#  include <wx/textctrl.h>
#endif

#include "Mdefaults.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxOptionsPageSubdialog : public wxDialog
{
public:
   wxOptionsPageSubdialog(ProfileBase *profile,
                          wxWindow *parent,
                          const wxString& label)
      : wxDialog(parent, -1, label,
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL)
      { m_profile = profile; }

   void OnChange(wxEvent& event);

protected:
   ProfileBase *m_profile;

private:
   DECLARE_EVENT_TABLE()
};

class wxComposeHeadersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxComposeHeadersDialog(ProfileBase *profile, wxWindow *parent);

   // trasnfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // update UI: disable the text boxes which shouldn't be edited
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   enum Headers
   {
      Header_To,
      Header_Cc,
      Header_Bcc,
      Header_Max
   };

   // headers names
   static const char *ms_headerNames[Header_Max];

   // profile key names
   static const char *ms_profileNamesDefault[Header_Max];
   static const char *ms_profileNamesShow[Header_Max];

   // the dialog controls
   wxCheckBox *m_checkboxes[Header_Max];
   wxTextCtrl *m_textvalues[Header_Max];

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the dialog element ids
enum
{
   // the text ctrls have the consecutive ids, this is the first one
   TextCtrlId = 1000
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxOptionsPageSubdialog, wxDialog)
   EVT_CHECKBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_RADIOBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_TEXT(-1,     wxOptionsPageSubdialog::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxComposeHeadersDialog, wxOptionsPageSubdialog)
   {
      wxEVT_UPDATE_UI,
      TextCtrlId, TextCtrlId + wxComposeHeadersDialog::Header_Max,
      (wxObjectEventFunction) (wxEventFunction) (wxUpdateUIEventFunction) &
         wxComposeHeadersDialog::OnUpdateUI,
      NULL
   },
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxOptionsPageSubdialog - the common base class for our dialogs
// ----------------------------------------------------------------------------

void wxOptionsPageSubdialog::OnChange(wxEvent&)
{
   // we don't do anything, but just eat these messages - otherwise they will
   // confuse wxOptionsPage which is our parent because it only processes
   // messages from its own controls
}

// ----------------------------------------------------------------------------
// wxComposeHeadersDialog - configure the headers in the outgoing messages
// ----------------------------------------------------------------------------

// static data
const char *wxComposeHeadersDialog::ms_headerNames[] =
{
   gettext_noop("&To"),
   gettext_noop("&Cc"),
   gettext_noop("&Bcc")
};

const char *wxComposeHeadersDialog::ms_profileNamesDefault[] =
{
   MP_COMPOSE_TO,
   MP_COMPOSE_CC,
   MP_COMPOSE_BCC
};

const char *wxComposeHeadersDialog::ms_profileNamesShow[] =
{
   NULL,  // no MP_SHOW_TO - always true
   MP_SHOWCC,
   MP_SHOWBCC
};

wxComposeHeadersDialog::wxComposeHeadersDialog(ProfileBase *profile,
                                               wxWindow *parent)
                      : wxOptionsPageSubdialog(profile, parent,
                                               _("Configure headers for "
                                                 "message composition"))
{
   SetAutoLayout(TRUE);

   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());
   int hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // first the 2 buttons in the bottom/right corner
   wxButton *btnOk = new wxButton(this, wxID_OK, _("OK"));
   btnOk->SetDefault();
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxRight, -2*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
   btnOk->SetConstraints(c);

   wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxRight, -(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
   btnCancel->SetConstraints(c);

   // a box around all the other controls
   wxStaticBox *box = new wxStaticBox(this, -1, _("&Headers"));
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxTop, LAYOUT_Y_MARGIN);
   box->SetConstraints(c);

   // a static message telling where is what
   wxStaticText *msg1 = new wxStaticText(this, -1, _("Show the field?"));
   wxStaticText *msg2 = new wxStaticText(this, -1, _("Default value"));

   c = new wxLayoutConstraints();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   msg1->SetConstraints(c);

   c = new wxLayoutConstraints();
   c->left.RightOf(msg1, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(msg1, wxTop);
   c->width.AsIs();
   c->height.AsIs();
   msg2->SetConstraints(c);

   // create a checkbox and a text field for each header
   wxControl *last = NULL;
   for ( size_t header = 0; header < Header_Max; header++ )
   {
      m_checkboxes[header] = new wxCheckBox(this, -1, ms_headerNames[header]);
      m_textvalues[header] = new wxTextCtrl(this, TextCtrlId + header);

      // set the constraints
      c = new wxLayoutConstraints();
      // we assume that the message above is longer than any of the field
      // names anyhow
      c->width.SameAs(msg1, wxWidth);
      c->left.SameAs(msg1, wxLeft);
      c->height.AsIs();
      c->centreY.SameAs(m_textvalues[header], wxCentreY);

      m_checkboxes[header]->SetConstraints(c);

      c = new wxLayoutConstraints();
      if ( !last )
      {
         c->top.SameAs(msg2, wxTop, 4*LAYOUT_Y_MARGIN);
      }
      else
      {
         c->top.Below(last, LAYOUT_Y_MARGIN);
      }
      c->left.RightOf(m_checkboxes[header], 3*LAYOUT_X_MARGIN);
      c->right.SameAs(box, wxRight, LAYOUT_X_MARGIN);
      c->height.AsIs();

      last = m_textvalues[header];
      last->SetConstraints(c);
   }

   // the "To" field is always shown
   m_checkboxes[Header_To]->SetValue(TRUE);
   m_checkboxes[Header_To]->Enable(FALSE);

   // set the (initial) window size
   wxWindow::SetSize(4*wBtn, 10*hBtn);
   Centre(wxCENTER_FRAME | wxBOTH);
}

void wxComposeHeadersDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   event.Enable( m_checkboxes[event.GetId() - TextCtrlId]->GetValue() );
}

bool wxComposeHeadersDialog::TransferDataToWindow()
{
   // disable environment variable expansion here because we want the user to
   // edit the real value stored in the config
   ProfileEnvVarSave suspend(m_profile, false);

   int show;      // show the header or not (checkbox value)
   wxString def;  // default value (text ctrl value)

   for ( size_t header = 0; header < Header_Max; header++ )
   {
      if ( header == Header_To )
      {
         // there is no such setting for this field and the checkbox value has
         // been already set in the ctor
         show = TRUE;
      }
      else
      {
         show = m_profile->readEntry(ms_profileNamesShow[header], 1);

         m_checkboxes[header]->SetValue(show != 0);
      }

      if ( show )
      {
         def = m_profile->readEntry(ms_profileNamesDefault[header], "");

         m_textvalues[header]->SetValue(def);
      }
   }

   return TRUE;
}

bool wxComposeHeadersDialog::TransferDataFromWindow()
{
   bool show;
   wxString def;

   for ( size_t header = 0; header < Header_Max; header++ )
   {
      if ( header == Header_To )
      {
         show = TRUE;
      }
      else
      {
         show = m_checkboxes[header]->GetValue();
         m_profile->writeEntry(ms_profileNamesShow[header], show);
      }

      if ( show )
      {
         def = m_textvalues[header]->GetValue();

         m_profile->writeEntry(ms_profileNamesDefault[header], def);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxMessageHeadersDialog - configure the headers in the message view
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

bool ConfigureComposeHeaders(ProfileBase *profile, wxWindow *parent)
{
   wxComposeHeadersDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK;
}

bool ConfigureMsgViewHeaders(ProfileBase *profile, wxWindow *parent)
{
   // TODO
#if 0
   wxMsgViewHeadersDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK;
#else
   return FALSE;
#endif
}
