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

#include <wx/layout.h>
#include <wx/notebook.h>
#include <wx/menuitem.h>
#include <wx/checklst.h>
#include <wx/utils.h>  // wxMax()
#include <wx/persctrl.h>

#include "Mdefaults.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxOptionsPageSubdialog : public wxProfileSettingsEditDialog
{
public:
   wxOptionsPageSubdialog(ProfileBase *profile,
                          wxWindow *parent,
                          const wxString& label,
                          const wxString& windowName);

   void OnChange(wxEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

class wxComposeHeadersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxComposeHeadersDialog(ProfileBase *profile, wxWindow *parent);

   // transfer data to/from window
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

   // dirty flag for the checkboxes (textctrls have their own)
   bool        m_oldCheckboxValues[Header_Max];

   DECLARE_EVENT_TABLE()
};

class wxMsgViewHeadersDialog  : public wxOptionsPageSubdialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxMsgViewHeadersDialog(ProfileBase *profile, wxWindow *parent);

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // update UI: disable the text boxes which shouldn't be edited
   void OnUpdateUI(wxUpdateUIEvent& event);

   // up/down buttons notifications
   void OnButtonUp(wxCommandEvent& event) { OnButtonMove(TRUE); }
   void OnButtonDown(wxCommandEvent& event) { OnButtonMove(FALSE); }

private:
   // real button events handler
   void OnButtonMove(bool up);

   wxCheckListBox *m_checklstBox;

   DECLARE_EVENT_TABLE()
};

class wxCustomHeadersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor
   wxCustomHeadersDialog(ProfileBase *profile, wxWindow *parent);

   // transfer data from window
   virtual bool TransferDataFromWindow();

   // accessors
   const wxString& GetHeaderName() const { return m_headerName; }
   const wxString& GetHeaderValue() const { return m_headerValue; }
   bool RememberHeader() const { return m_remember; }

   // event handlers
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   wxString m_headerName,
            m_headerValue;
   bool     m_remember;

   wxComboBox *m_textctrlName,
              *m_textctrlValue;

   wxCheckBox *m_checkboxRemember;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the dialog element ids

// for wxComposeHeadersDialog
enum
{
   // the text ctrls have the consecutive ids, this is the first one
   TextCtrlId = 1000
};

// for wxMsgViewHeadersDialog
enum
{
   Btn_Up = 1000,
   Btn_Down
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

#define EVT_UPDATE_UI_RANGE(id1, id2, func) \
   {\
      wxEVT_UPDATE_UI,\
      id1, id2,\
      (wxObjectEventFunction)(wxEventFunction)(wxUpdateUIEventFunction)&func,\
      (wxObject *) NULL\
   },

BEGIN_EVENT_TABLE(wxOptionsPageSubdialog, wxDialog)
   EVT_CHECKBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_RADIOBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_TEXT(-1,     wxOptionsPageSubdialog::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxComposeHeadersDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI_RANGE(TextCtrlId,
                       TextCtrlId + wxComposeHeadersDialog::Header_Max,
                       wxComposeHeadersDialog::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMsgViewHeadersDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(Btn_Up, wxMsgViewHeadersDialog::OnButtonUp)
   EVT_BUTTON(Btn_Down, wxMsgViewHeadersDialog::OnButtonDown)

   EVT_UPDATE_UI_RANGE(Btn_Up, Btn_Down, wxMsgViewHeadersDialog::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxCustomHeadersDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI(wxID_OK, wxCustomHeadersDialog::OnUpdateUI)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxOptionsPageSubdialog - the common base class for our dialogs
// ----------------------------------------------------------------------------

wxOptionsPageSubdialog::wxOptionsPageSubdialog(ProfileBase *profile,
                                               wxWindow *parent,
                                               const wxString& label,
                                               const wxString& windowName)
                      : wxProfileSettingsEditDialog
                        (
                           profile,
                           windowName,
                           GET_PARENT_OF_CLASS(parent, wxFrame),
                           label
                        )
{
}

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
                                                 "message composition"),
                                               "ComposeHeaders")
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Headers"));

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
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->height.AsIs();

      last = m_textvalues[header];
      last->SetConstraints(c);
   }

   // the "To" field is always shown
   m_checkboxes[Header_To]->SetValue(TRUE);
   m_checkboxes[Header_To]->Enable(FALSE);

   // set the minimal and initial window size
   SetDefaultSize(4*wBtn, 8*hBtn);
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

   bool show;     // show the header or not (checkbox value)
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
         show = m_profile->readEntry(ms_profileNamesShow[header], 1) != 0;

         m_checkboxes[header]->SetValue(show != 0);
      }

      m_oldCheckboxValues[header] = show;

      if ( show )
      {
         def = m_profile->readEntry(ms_profileNamesDefault[header], "");

         m_textvalues[header]->SetValue(def);
         m_textvalues[header]->DiscardEdits();
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
         if ( show != m_oldCheckboxValues[header] )
         {
            m_hasChanges = TRUE;

            m_profile->writeEntry(ms_profileNamesShow[header], show);
         }
      }

      if ( show && m_textvalues[header]->IsModified() )
      {
         def = m_textvalues[header]->GetValue();

         m_hasChanges = TRUE;

         m_profile->writeEntry(ms_profileNamesDefault[header], def);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxMessageHeadersDialog - configure the headers in the message view
// ----------------------------------------------------------------------------

wxMsgViewHeadersDialog::wxMsgViewHeadersDialog(ProfileBase *profile,
                                               wxWindow *parent)
                      : wxOptionsPageSubdialog(profile, parent,
                                               _("Configure headers to show "
                                                 "in message view"),
                                               "MsgViewHeaders")
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Headers"));

   // buttons to move items up/down
   wxButton *btnDown = new wxButton(this, Btn_Down, _("&Down"));
   c = new wxLayoutConstraints();
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   btnDown->SetConstraints(c);

   // FIXME: we also assume that "Down" is longer than "Up" - which may, of
   //        course, be false after translation
   wxButton *btnUp = new wxButton(this, Btn_Up, _("&Up"));
   c = new wxLayoutConstraints();
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
   c->width.SameAs(btnDown, wxWidth);
   c->height.AsIs();
   btnUp->SetConstraints(c);

   // a checklistbox with headers on the space which is left
   m_checklstBox = new wxCheckListBox(this, -1);
   c = new wxLayoutConstraints();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(btnDown, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_checklstBox->SetConstraints(c);

   // set the minimal window size
   SetDefaultSize(3*wBtn, 7*hBtn);
}

bool wxMsgViewHeadersDialog::TransferDataToWindow()
{
   // colon separated list of all the headers (even if they're not all shown)
   wxString allHeaders = READ_CONFIG(m_profile, MP_MSGVIEW_ALL_HEADERS);

   // the headers we do show: prepend ':' in front because it allows us to
   // check very easily whether the header is shown or not: we just search for
   // ":<header>:" (of course, without the ':' in front this wouldn't work for
   // the first header)
   wxString shownHeaders(':');
   shownHeaders += READ_CONFIG(m_profile, MP_MSGVIEW_HEADERS);

   size_t n = 0;
   wxString header;  // accumulator
   for ( const char *p = allHeaders.c_str(); *p != '\0'; p++ )
   {
      if ( *p == ':' )
      {
         wxASSERT_MSG( !!header, "header name shouldn't be empty" );

         m_checklstBox->Append(header);

         // check the item only if we show it
         header.Prepend(':');
         header.Append(':');
         if ( shownHeaders.Find(header) != wxNOT_FOUND )
         {
            m_checklstBox->Check(n); // the last item we added
         }

         n++;
         header.Empty();
      }
      else
      {
         header += *p;
      }
   }

   return TRUE;
}

bool wxMsgViewHeadersDialog::TransferDataFromWindow()
{
   // copy all entries from the checklistbox to the profile
   wxString shownHeaders, allHeaders, header;
   size_t count = (size_t)m_checklstBox->Number();
   for ( size_t n = 0; n < count; n++ )
   {
      header = m_checklstBox->GetString(n);
      header += ':';

      if ( m_checklstBox->IsChecked(n) )
      {
         shownHeaders += header;
      }

      allHeaders += header;
   }

   // update the headers which we show
   if ( shownHeaders != READ_CONFIG(m_profile, MP_MSGVIEW_HEADERS) )
   {
      m_hasChanges = TRUE;

      m_profile->writeEntry(MP_MSGVIEW_HEADERS, shownHeaders);
   }

   // update the list of known entries too
   if ( allHeaders != READ_CONFIG(m_profile, MP_MSGVIEW_ALL_HEADERS) )
   {
      // it shouldn't cound as "change" really, so we don't set the flag
      m_profile->writeEntry(MP_MSGVIEW_ALL_HEADERS, allHeaders);
   }

   return TRUE;
}

void wxMsgViewHeadersDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   // only enable buttons if there is something selected
   event.Enable( m_checklstBox->GetSelection() != -1 );
}

void wxMsgViewHeadersDialog::OnButtonMove(bool up)
{
    int selection = m_checklstBox->GetSelection();
    if ( selection != -1 )
    {
        wxString label = m_checklstBox->GetString(selection);

        int positionNew = up ? selection - 1 : selection + 2;
        if ( positionNew >= 0 && positionNew < m_checklstBox->Number() )
        {
            bool wasChecked = m_checklstBox->IsChecked(selection);

            int positionOld = up ? selection + 1 : selection;

            // insert the item
            m_checklstBox->InsertItems(1, &label, positionNew);

            // and delete the old one
            m_checklstBox->Delete(positionOld);

            int selectionNew = up ? positionNew : positionNew - 1;
            m_checklstBox->Check(selectionNew, wasChecked);
            m_checklstBox->SetSelection(selectionNew);
        }
        //else: out of range, silently ignore
    }
}

// ----------------------------------------------------------------------------
// wxCustomHeadersDialog - edit the value of a custom header field
// ----------------------------------------------------------------------------

wxCustomHeadersDialog::wxCustomHeadersDialog(ProfileBase *profile,
                                             wxWindow *parent)
                     : wxOptionsPageSubdialog(profile, parent,
                                              _("Configure custom header"),
                                              "CustomHeader")
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // [Ok], [Cancel] and a box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("Edit header"));

   // calc the max width of the label
   wxString labelName = _("Header name: "),
            labelValue = _("Header value: ");

   int widthName, widthValue;
   GetTextExtent(labelName, &widthName, NULL);
   GetTextExtent(labelValue, &widthValue, NULL);

   int widthLabel = wxMax(widthName, widthValue);

   // header name
   wxStaticText *label;

   label = new wxStaticText(this, -1, labelName);
   m_textctrlName = new wxPTextEntry("CustomHeaderName", this);

   c = new wxLayoutConstraints();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthLabel);
   c->height.AsIs();
   c->centreY.SameAs(m_textctrlName, wxTop, 2*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   c = new wxLayoutConstraints();
   c->left.RightOf(label, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   m_textctrlName->SetConstraints(c);

   // header value
   label = new wxStaticText(this, -1, labelValue);
   m_textctrlValue = new wxPTextEntry("CustomHeaderValue", this);

   c = new wxLayoutConstraints();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthLabel);
   c->height.AsIs();
   c->centreY.SameAs(m_textctrlValue, wxTop, 2*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   c = new wxLayoutConstraints();
   c->left.RightOf(label, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   c->top.Below(m_textctrlName, 2*LAYOUT_Y_MARGIN);
   m_textctrlValue->SetConstraints(c);

   // a checkbox (space at the end to workaround wxMSW bug - otherwise, the
   // caption is truncated)
   m_checkboxRemember = new wxCheckBox(this, -1, _("Remember these values "));
   c = new wxLayoutConstraints();
   c->height.AsIs();
   c->width.AsIs();
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   c->centreX.SameAs(box, wxCentreX);
   m_checkboxRemember->SetConstraints(c);

   // final touch
   m_textctrlName->SetFocus();
   SetDefaultSize(4*wBtn, 8*hBtn);
}

bool wxCustomHeadersDialog::TransferDataFromWindow()
{
   m_headerName = m_textctrlName->GetValue();
   m_headerValue = m_textctrlValue->GetValue();
   m_remember = m_checkboxRemember->GetValue();

   return TRUE;
}

void wxCustomHeadersDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   // only enable the [Ok] button if the header name is not empty
   event.Enable( !!m_textctrlName->GetValue() );
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

bool ConfigureComposeHeaders(ProfileBase *profile, wxWindow *parent)
{
   wxComposeHeadersDialog dlg(profile, parent);

   return (dlg.ShowModal() == wxID_OK) && dlg.HasChanges();
}

bool ConfigureMsgViewHeaders(ProfileBase *profile, wxWindow *parent)
{
   wxMsgViewHeadersDialog dlg(profile, parent);

   return (dlg.ShowModal() == wxID_OK) && dlg.HasChanges();
}

bool ConfigureCustomHeader(ProfileBase *profile, wxWindow *parent,
                           String *headerName, String *headerValue,
                           bool *storedInProfile)
{
   wxCustomHeadersDialog dlg(profile, parent);

   if ( dlg.ShowModal() == wxID_OK )
   {
      *headerName = dlg.GetHeaderName();
      *headerValue = dlg.GetHeaderValue();

      bool remember = dlg.RememberHeader();

      if ( storedInProfile )
         *storedInProfile = remember;

      if ( remember )
      {
         profile->SetPath(M_CUSTOM_HEADERS_CONFIG_SECTION);
         profile->writeEntry(*headerName, *headerValue);
         profile->ResetPath();
      }

      return true;
   }
   else
   {
      // cancelled
      return false;
   }
}
