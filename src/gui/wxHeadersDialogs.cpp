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
#  include <wx/menuitem.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/checkbox.h>
#  include <wx/radiobox.h>
#  include <wx/textctrl.h>
#  include <wx/statbmp.h>

#  include <wx/utils.h>  // wxMax()
#endif

#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/checklst.h>
#include <wx/listctrl.h>
#include "wx/persctrl.h"

#include "Mdefaults.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxIconManager.h"

#include "HeadersDialogs.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// headers of different types live in different subgroups
static const char *gs_customHeaderSubgroups[CustomHeader_Max] =
{
   "News",
   "Mail",
   "Both"
};

// the dialog element ids
// ----------------------

// for wxComposeHeadersDialog
enum
{
   // the text ctrls have the consecutive ids, this is the first one
   TextCtrlId = 1000,
   Button_EditCustom = 1100
};

// for wxMsgViewHeadersDialog
enum
{
   Button_Up = 1000,
   Button_Down
};

// for wxCustomHeadersDialog
enum
{
   Button_Add = 1000,
   Button_Delete,
   Button_Edit
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// get the folder name from the profile (may return empty string if editing
// global settings)
static String GetFolderNameFromProfile(Profile *profile);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// dialog to configure standard headers shown during message composition
class wxComposeHeadersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxComposeHeadersDialog(Profile *profile, wxWindow *parent);

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // event handlers
      // update UI: disable the text boxes which shouldn't be edited
   void OnUpdateUI(wxUpdateUIEvent& event);
      // show "custom headers" dialog
   void OnEditCustomHeaders(wxCommandEvent& event);

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

// dialog to configure headers shown in the message view
class wxMsgViewHeadersDialog  : public wxOptionsPageSubdialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxMsgViewHeadersDialog(Profile *profile, wxWindow *parent);

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

// dialog allowing editing one custom header
class wxCustomHeaderDialog : public wxOptionsPageSubdialog
{
public:
   // ctor
   wxCustomHeaderDialog(Profile *profile,
                        wxWindow *parent,
                        bool letUserChooseType);

   // set the initial data for the controls
   void Init(const wxString& headerName,
             const wxString& headerValue,
             CustomHeaderType headerType)
   {
      m_headerName = headerName;
      m_headerValue = headerValue;
      m_headerType = headerType;
   }

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // accessors
   const wxString& GetHeaderName() const { return m_headerName; }
   const wxString& GetHeaderValue() const { return m_headerValue; }
   CustomHeaderType GetHeaderType() const { return m_headerType; }

   bool RememberHeader() const { return m_remember; }

   // event handlers
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   // the data
   wxString m_headerName,
            m_headerValue;
   bool     m_remember;

   CustomHeaderType m_headerType;

   // the dialog controls
   wxComboBox *m_textctrlName,
              *m_textctrlValue;

   wxCheckBox *m_checkboxRemember;
   wxRadioBox *m_radioboxType;

   DECLARE_EVENT_TABLE()
};

// dialog allowing editing all custom headers for the given profil
class wxCustomHeadersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor & dtor
   wxCustomHeadersDialog(Profile *profile, wxWindow *parent);
   virtual ~wxCustomHeadersDialog();

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // event handlers
   void OnUpdateUI(wxUpdateUIEvent& event);
   void OnEdit(wxCommandEvent& event);
   void OnAdd(wxCommandEvent& event);
   void OnDelete(wxCommandEvent& event);

private:
   // return TRUE if the radiobox has selection and store in sel
   bool GetSelection(size_t *sel) const
   {
      int s = m_listctrl->GetNextItem(-1,
                                      wxLIST_NEXT_ALL,
                                      wxLIST_STATE_SELECTED);

      if ( s == -1 )
         return FALSE;

      *sel = (size_t)s;

      return TRUE;
   }

   // finds header by name in the listctrl, returns wxNOT_FOUND if not found
   int FindHeaderByName(const String& headerName) const;

   // modifies an existing header in the listctrl
   void ModifyHeader(int index,
                     const String& headerName,
                     const String& headerValue,
                     CustomHeaderType type);

   // adds a header to the listctrl
   void AddHeader(int index,
                  const String& headerName,
                  const String& headerValue,
                  CustomHeaderType type);

   // retrieves the name and value for header by index from the listctrl
   void GetHeader(int index, String *headerName, String *headerValue);

   // our profile: the headers info is stored under
   // M_CUSTOM_HEADERS_CONFIG_SECTION in it
   Profile *m_profile;

   // the types of the headers (not ints, but CustomHeaderTypes, in fact)
   wxArrayInt m_headerTypes;

   // dialog controls
   wxButton *m_btnDelete,
            *m_btnAdd,
            *m_btnEdit;

   wxListCtrl *m_listctrl;

   DECLARE_EVENT_TABLE()
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

BEGIN_EVENT_TABLE(wxComposeHeadersDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI_RANGE(TextCtrlId,
                       TextCtrlId + wxComposeHeadersDialog::Header_Max,
                       wxComposeHeadersDialog::OnUpdateUI)
   EVT_BUTTON(Button_EditCustom, wxComposeHeadersDialog::OnEditCustomHeaders)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMsgViewHeadersDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(Button_Up, wxMsgViewHeadersDialog::OnButtonUp)
   EVT_BUTTON(Button_Down, wxMsgViewHeadersDialog::OnButtonDown)

   EVT_UPDATE_UI_RANGE(Button_Up, Button_Down, wxMsgViewHeadersDialog::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxCustomHeaderDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI(wxID_OK, wxCustomHeaderDialog::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxCustomHeadersDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI(Button_Delete, wxCustomHeadersDialog::OnUpdateUI)
   EVT_UPDATE_UI(Button_Edit, wxCustomHeadersDialog::OnUpdateUI)

   EVT_BUTTON(Button_Add, wxCustomHeadersDialog::OnAdd)
   EVT_BUTTON(Button_Delete, wxCustomHeadersDialog::OnDelete)
   EVT_BUTTON(Button_Edit, wxCustomHeadersDialog::OnEdit)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

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

wxComposeHeadersDialog::wxComposeHeadersDialog(Profile *profile,
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

   // for advanced users only: a button to invoke the dialog for configuring
   // other headers
   if ( READ_APPCONFIG(MP_USERLEVEL) == M_USERLEVEL_ADVANCED )
   {
      wxButton *btnEditCustom = new wxButton(this, Button_EditCustom,
                                             _("&More..."));

      wxWindow *btnOk = FindWindow(wxID_OK);
      c = new wxLayoutConstraints();
      c->right.LeftOf(btnOk, LAYOUT_X_MARGIN);
      c->top.SameAs(btnOk, wxTop);
      c->width.Absolute(wBtn);
      c->height.Absolute(hBtn);
      btnEditCustom->SetConstraints(c);
   }

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

void wxComposeHeadersDialog::OnEditCustomHeaders(wxCommandEvent& event)
{
   wxCustomHeadersDialog dlg(m_profile, GetParent());

   dlg.ShowModal();
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

wxMsgViewHeadersDialog::wxMsgViewHeadersDialog(Profile *profile,
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
   wxButton *btnDown = new wxButton(this, Button_Down, _("&Down"));
   c = new wxLayoutConstraints();
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   btnDown->SetConstraints(c);

   // FIXME: we also assume that "Down" is longer than "Up" - which may, of
   //        course, be false after translation
   wxButton *btnUp = new wxButton(this, Button_Up, _("&Up"));
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
// wxCustomHeaderDialog - edit the value of one custom header field
// ----------------------------------------------------------------------------

wxCustomHeaderDialog::wxCustomHeaderDialog(Profile *profile,
                                           wxWindow *parent,
                                           bool letUserChooseType)
                     : wxOptionsPageSubdialog(profile, parent,
                                              _("Edit custom header"),
                                              "CustomHeader")
{
   // init the member vars
   // --------------------

   m_headerType = CustomHeader_Invalid;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // [Ok], [Cancel] and a box around everything else
   wxString foldername = GetFolderNameFromProfile(profile);
   wxString labelBox;
   if ( !foldername.IsEmpty() )
      labelBox.Printf(_("Header value for folder '%s'"), foldername.c_str());
   else
      labelBox.Printf(_("Default value for header"));
   wxStaticBox *box = CreateStdButtonsAndBox(labelBox);

   // calc the max width of the label
   wxString labelName = _("&Name: "),
            labelValue = _("&Value: ");

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

   int extraHeight;
   if ( letUserChooseType )
   {
      static const char *radioItems[CustomHeader_Max] =
      {
         gettext_noop("news postings"),
         gettext_noop("mail messages"),
         gettext_noop("all messages")
      };

      wxString *radioStrings = new wxString[ (size_t) CustomHeader_Max];
      for ( size_t n = 0; n < CustomHeader_Max; n++ )
      {
         radioStrings[n] = _(radioItems[n]);
      }

      // in this mode we always save the value in the profile, so we don't need
      // the checkbox - but we have instead a radio box allowing the user to
      // choose for which kind of message he wants to use this header
      m_radioboxType = new wxPRadioBox("CurstomHeaderType",
                                       this, -1, _("Use this header for:"),
                                       wxDefaultPosition, wxDefaultSize,
                                       CustomHeader_Max, radioStrings,
                                       1, wxRA_SPECIFY_COLS);

      delete [] radioStrings;

      c = new wxLayoutConstraints();
      c->top.Below(m_textctrlValue, LAYOUT_Y_MARGIN);
      c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->height.AsIs();
      m_radioboxType->SetConstraints(c);

      m_headerType = CustomHeader_Both;

      m_checkboxRemember = (wxCheckBox *)NULL;

      extraHeight = 3;
   }
   else
   {
      // a checkbox which allows the user to save (or not) the value for this
      // header in the profile

      // NB: space at the end to workaround wxMSW bug - otherwise, the caption
      //     is truncated
      m_checkboxRemember = new wxCheckBox(this, -1, _("Save these values "));
      c = new wxLayoutConstraints();
      c->height.AsIs();
      c->width.AsIs();
      c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
      c->centreX.SameAs(box, wxCentreX);

      m_checkboxRemember->SetConstraints(c);

      m_radioboxType = (wxRadioBox *)NULL;

      extraHeight = 0;
   }

   // final touch
   Layout();
   m_textctrlName->SetFocus();
   SetDefaultSize(4*wBtn, (8 + extraHeight)*hBtn);
}

bool wxCustomHeaderDialog::TransferDataToWindow()
{
   if ( !!m_headerName )
   {
      m_textctrlName->SetValue(m_headerName);
   }

   if ( !!m_headerValue )
   {
      m_textctrlValue->SetValue(m_headerValue);
   }

   if ( m_radioboxType )
   {
      m_radioboxType->SetSelection(m_headerType);
   }

   return TRUE;
}

bool wxCustomHeaderDialog::TransferDataFromWindow()
{
   m_headerName = m_textctrlName->GetValue();
   m_headerValue = m_textctrlValue->GetValue();

   if ( m_checkboxRemember )
      m_remember = m_checkboxRemember->GetValue();

   if ( m_radioboxType )
      m_headerType = (CustomHeaderType)m_radioboxType->GetSelection();

   return TRUE;
}

void wxCustomHeaderDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   // only enable the [Ok] button if the header name is not empty
   event.Enable( !!m_textctrlName->GetValue() );
}

// ----------------------------------------------------------------------------
// wxCustomHeadersDialog - edit all custom headers for the given profile
// ----------------------------------------------------------------------------

wxCustomHeadersDialog::wxCustomHeadersDialog(Profile *profile,
                                             wxWindow *parent)
                     : wxOptionsPageSubdialog(profile, parent,
                                              _("Configure custom headers"),
                                              "CustomHeaders")
{
   // init member data
   // ----------------
   m_profile = profile;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // [Ok], [Cancel] and a box around everything else
   wxString foldername = GetFolderNameFromProfile(profile);
   wxString labelBox;
   if ( !foldername.IsEmpty() )
      labelBox.Printf(_("Custom &headers for folder '%s'"), foldername.c_str());
   else
      labelBox.Printf(_("Default custom headers"));

   wxStaticBox *box = CreateStdButtonsAndBox(labelBox);

   // start with 3 buttons
   m_btnDelete = new wxButton(this, Button_Delete, _("&Delete"));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->centreY.SameAs(box, wxCentreY);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_btnDelete->SetConstraints(c);

   m_btnAdd = new wxButton(this, Button_Add, _("&Add..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->bottom.Above(m_btnDelete, -LAYOUT_Y_MARGIN);
   m_btnAdd->SetConstraints(c);

   m_btnEdit = new wxButton(this, Button_Edit, _("&Edit..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->top.Below(m_btnDelete, LAYOUT_Y_MARGIN);
   m_btnEdit->SetConstraints(c);

   // now the listctrl
   m_listctrl = new wxPListCtrl("HeaderEditList", this, -1,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnDelete, 3*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listctrl->SetConstraints(c);

   // listctrl initialization
   // -----------------------

   // create the imagelist
   static const char *iconNames[] =
   {
      "image_news",
      "image_mail",
      "image_both"
   };

   wxIconManager *iconmanager = mApplication->GetIconManager();
   wxImageList *imagelist = new wxImageList(16, 16, FALSE, CustomHeader_Max);
   for ( size_t nImage = 0; nImage < CustomHeader_Max; nImage++ )
   {
      imagelist->Add(iconmanager->GetBitmap(iconNames[nImage]));
   }

   m_listctrl->SetImageList(imagelist, wxIMAGE_LIST_SMALL);

   // setup the listctrl: we have 2 columns
   m_listctrl->InsertColumn(0, _("Name"));
   m_listctrl->InsertColumn(1, _("Value"));

   // finishing initialization
   // ------------------------

   m_listctrl->SetFocus();
   SetDefaultSize(5*wBtn, 9*hBtn);
}

wxCustomHeadersDialog::~wxCustomHeadersDialog()
{
   delete m_listctrl->GetImageList(wxIMAGE_LIST_SMALL);
}

int wxCustomHeadersDialog::FindHeaderByName(const String& headerName) const
{
   int index = -1;
   while ( (index = m_listctrl->GetNextItem(index)) != -1 )
   {
      if ( m_listctrl->GetItemText(index) == headerName )
      {
         return index;
      }
   }

   return wxNOT_FOUND;
}

void wxCustomHeadersDialog::ModifyHeader(int index,
                                         const String& headerName,
                                         const String& headerValue,
                                         CustomHeaderType type)
{
   if ( m_listctrl->SetItem(index, 0, headerName) == -1 )
   {
      FAIL_MSG("can't set item info in listctrl");
   }

   if ( m_listctrl->SetItem(index, 1, headerValue) == -1 )
   {
      FAIL_MSG("can't set item info in listctrl");
   }

   wxListItem li;
   li.m_mask = wxLIST_MASK_IMAGE;
   li.m_itemId = index;
   li.m_col = 0;
   if ( !m_listctrl->SetItem(li) )
   {
      FAIL_MSG("can't change items icon in listctrl");
   }

   m_headerTypes[(size_t)index] = type;
}

void wxCustomHeadersDialog::AddHeader(int index,
                                      const String& headerName,
                                      const String& headerValue,
                                      CustomHeaderType type)
{
   m_headerTypes.Add(type);

   if ( m_listctrl->InsertItem(index, headerName, type) == -1 )
   {
      FAIL_MSG("can't insert item into listctrl");
   }

   if ( m_listctrl->SetItem(index, 1, headerValue) == -1 )
   {
      FAIL_MSG("can't set item info in listctrl");
   }
}

void wxCustomHeadersDialog::GetHeader(int index,
                                      String *headerName,
                                      String *headerValue)
{
   *headerName = m_listctrl->GetItemText(index);

   wxListItem li;
   li.m_mask = wxLIST_MASK_TEXT;
   li.m_itemId = index;
   li.m_col = 1;
   if ( !m_listctrl->GetItem(li) )
   {
      FAIL_MSG("can't get header value from listctrl");
   }

   *headerValue = li.m_text;
}

bool wxCustomHeadersDialog::TransferDataToWindow()
{
   wxArrayString headerNames, headerValues;
   wxArrayInt headerTypes;
   size_t nHeaders = GetCustomHeaders(m_profile,
                                      CustomHeader_Invalid,
                                      &headerNames,
                                      &headerValues,
                                      &headerTypes);

   for ( size_t nHeader = 0; nHeader < nHeaders; nHeader++ )
   {
      AddHeader(nHeader,
                headerNames[nHeader],
                headerValues[nHeader],
                (CustomHeaderType)headerTypes[nHeader]);
   }

   return TRUE;
}

bool wxCustomHeadersDialog::TransferDataFromWindow()
{
   // FIXME not really efficient - we could update only the entries which
   //       really changed instead of rewriting everything

   // delete the old entries
   wxString root = M_CUSTOM_HEADERS_CONFIG_SECTION;
   {
      ProfilePathChanger pathChanger(m_profile, root);

      for ( int type = 0; type < CustomHeader_Max; type++ )
      {
         m_profile->DeleteGroup(gs_customHeaderSubgroups[type]);
      }
   }

   // write the new ones
   String headerName, headerValue;
   int nItems = m_listctrl->GetItemCount();
   for ( int nItem = 0; nItem < nItems; nItem++ )
   {
      GetHeader(nItem, &headerName, &headerValue);

      // write the entry into the correct subgroup
      int type = m_headerTypes[(size_t)nItem];
      wxString path;
      path << root << '/' << gs_customHeaderSubgroups[type];
      ProfilePathChanger pathChanger(m_profile, path);

      m_profile->writeEntry(headerName, headerValue);
   }

   return TRUE;
}

void wxCustomHeadersDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   event.Enable( m_listctrl->GetSelectedItemCount() > 0 );
}

void wxCustomHeadersDialog::OnEdit(wxCommandEvent& WXUNUSED(event))
{
   size_t sel;
   CHECK_RET( GetSelection(&sel), "button should be disabled" );

   wxCustomHeaderDialog dlg(m_profile, GetParent(), TRUE);

   String headerName, headerValue;
   GetHeader(sel, &headerName, &headerValue);
   dlg.Init(headerName, headerValue, (CustomHeaderType)m_headerTypes[sel]);

   if ( dlg.ShowModal() == wxID_OK )
   {
      headerName = dlg.GetHeaderName();
      headerValue = dlg.GetHeaderValue();
      CustomHeaderType type = dlg.GetHeaderType();

      ModifyHeader(sel, headerName, headerValue, type);
   }
}

void wxCustomHeadersDialog::OnAdd(wxCommandEvent& WXUNUSED(event))
{
   wxCustomHeaderDialog dlg(m_profile, GetParent(), TRUE);

   if ( dlg.ShowModal() == wxID_OK )
   {
      String headerName = dlg.GetHeaderName();
      String headerValue = dlg.GetHeaderValue();
      CustomHeaderType headerType = dlg.GetHeaderType();

      int index = FindHeaderByName(headerName);
      if ( index == wxNOT_FOUND )
      {
         AddHeader(m_listctrl->GetItemCount(),
                   headerName, headerValue, headerType);
      }
      else
      {
         ModifyHeader(index, headerName, headerValue, headerType);
      }
   }
}

void wxCustomHeadersDialog::OnDelete(wxCommandEvent& WXUNUSED(event))
{
   size_t sel;
   CHECK_RET( GetSelection(&sel), "button should be disabled" );

   m_listctrl->DeleteItem(sel);
   m_headerTypes.Remove(sel);
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static String GetFolderNameFromProfile(Profile *profile)
{
   String folderName, profileName = profile->GetName();
   size_t lenPrefix = strlen(M_PROFILE_CONFIG_SECTION);
   if ( strncmp(profileName, M_PROFILE_CONFIG_SECTION, lenPrefix) == 0 )
   {
      const char *p = profileName.c_str() + lenPrefix;

      if ( *p )
      {
         // +1 to skip following '/'
         folderName = p + 1;
         folderName = folderName.BeforeFirst('/');
      }
      //else: leave empty
   }

   return folderName;
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

bool ConfigureComposeHeaders(Profile *profile, wxWindow *parent)
{
   wxComposeHeadersDialog dlg(profile, parent);

   return (dlg.ShowModal() == wxID_OK) && dlg.HasChanges();
}

bool ConfigureMsgViewHeaders(Profile *profile, wxWindow *parent)
{
   wxMsgViewHeadersDialog dlg(profile, parent);

   return (dlg.ShowModal() == wxID_OK) && dlg.HasChanges();
}

bool ConfigureCustomHeader(Profile *profile, wxWindow *parent,
                           String *headerName, String *headerValue,
                           bool *storedInProfile,
                           CustomHeaderType type)
{
   bool letUserChooseType = type == CustomHeader_Invalid;
   wxCustomHeaderDialog dlg(profile, parent, letUserChooseType);

   if ( dlg.ShowModal() == wxID_OK )
   {
      // get data from the dialog
      *headerName = dlg.GetHeaderName();
      *headerValue = dlg.GetHeaderValue();
      if ( letUserChooseType )
      {
         type = dlg.GetHeaderType();
      }

      bool remember = letUserChooseType ? TRUE : dlg.RememberHeader();

      if ( storedInProfile )
         *storedInProfile = remember;

      if ( remember )
      {
         String path;
         path << M_CUSTOM_HEADERS_CONFIG_SECTION << '/'
               << gs_customHeaderSubgroups[type];

         ProfilePathChanger pathChanger(profile, path);
         profile->writeEntry(*headerName, *headerValue);
      }

      return true;
   }
   else
   {
      // cancelled
      return false;
   }
}

bool ConfigureCustomHeaders(Profile *profile, wxWindow *parent)
{
   wxCustomHeadersDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK;
}

// TODO we should implement inheritance!!
size_t GetCustomHeaders(Profile *profile,
                        CustomHeaderType typeWanted,
                        wxArrayString *names,
                        wxArrayString *values,
                        wxArrayInt *types)
{
   // init
   names->Empty();
   values->Empty();
   if ( types )
      types->Empty();

   // read headers of all types, select those which we need
   String headerName, headerValue;
   long dummy;
   for ( int type = 0; type < CustomHeader_Max; type++ )
   {
      // check whether we're interested in the entries of this type at all
      if ( (typeWanted < CustomHeader_Both) && (typeWanted != type) )
      {
         // no, we want only "Mail" entries and the current type is "News" (or
         // vice versa), skip this type
         continue;
      }

      // go to the subgroup
      String path;
      path << M_CUSTOM_HEADERS_CONFIG_SECTION << '/'
           << gs_customHeaderSubgroups[type];
      ProfilePathChanger pathChanger(profile, path);

      // enum all entries in it
      bool cont = profile->GetFirstEntry(headerName, dummy);
      while ( cont )
      {
         if ( names->Index(headerName) == wxNOT_FOUND )
         {
            headerValue = profile->readEntry(headerName, "");

            names->Add(headerName);
            values->Add(headerValue);
            if ( types )
               types->Add(type);
         }
         //else: ignore all occurences except the first

         cont = profile->GetNextEntry(headerName, dummy);
      }
   }

   return names->GetCount();
}
