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

/*
   Explanation of the profile entries we use: in each profile we have

   CustomHeaders_Mail = "X-Foo:X-Bar:...:X-Baz"

   and also CustomHeaders_News and CustomHeaders_Both which specify the set of
   custom headers to use for email messages, news messages and all messages
   respectively.

   The value of an extra header X-Foo is stored under CustomHeaders:X-Foo:Mail,
   CustomHeaders:X-Foo:News or CustomHeaders:X-Foo:Both.

   Note that the names of the custom headers must be valid profile names and
   can't contain ':'. This is not a restriction as a valid RFC 822 (or 1036)
   header satisfies both of these requirments anyhow.
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"

#  include <wx/layout.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>      // for wxStaticText
#  include <wx/utils.h>         // for wxMax
#  include <wx/checklst.h>
#  include <wx/textdlg.h>       // for wxTextEntryDialog

#  include "strutil.h"   // strutil_flatten_array()
#  include "Mdefaults.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "MApplication.h"
#  include "gui/wxIconManager.h"
#endif // USE_PCH

#include <wx/imaglist.h>        // for wxImageList

#include "gui/wxSelectionDlg.h"

#include "HeadersDialogs.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_CC;
extern const MOption MP_COMPOSE_TO;
extern const MOption MP_MSGVIEW_ALL_HEADERS;
extern const MOption MP_MSGVIEW_HEADERS;
extern const MOption MP_ORGANIZATION;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the prefix for custom headers values
#define CUSTOM_HEADERS_PREFIX _T("CustomHeaders")

// the organization header
#define ORGANIZATION _T("Organization")

// prefixes identifying headers of different types
static const wxChar *gs_customHeaderSubgroups[CustomHeader_Max] =
{
   _T("News"),
   _T("Mail"),
   _T("Both")
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

// for wxCustomHeadersDialog
enum
{
   Button_Add = 1000,
   Button_Delete,
   Button_Edit
};

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

protected:
   // event handlers

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
   static const wxChar *ms_headerNames[Header_Max];

   // profile key names
   static const wxChar *ms_profileNamesDefault[Header_Max];

   // the dialog controls
   wxTextCtrl *m_textvalues[Header_Max];

   static void InitStaticArrays();

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxComposeHeadersDialog)
};

// dialog to configure headers shown in the message view
class wxMsgViewHeadersDialog  : public wxSelectionsOrderDialog
{
public:
   // ctor which takes the profile whose settings we will edit
   wxMsgViewHeadersDialog(Profile *profile, wxWindow *parent);
   virtual ~wxMsgViewHeadersDialog();

   // transfer data to/from window
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   Profile *m_profile;

   DECLARE_NO_COPY_CLASS(wxMsgViewHeadersDialog)
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
   DECLARE_NO_COPY_CLASS(wxCustomHeaderDialog)
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

   // our profile: the headers info is stored here
   Profile *m_profile;

   // the types of the headers (not ints, but CustomHeaderTypes, in fact)
   wxArrayInt m_headerTypes;

   // dialog controls
   wxButton *m_btnDelete,
            *m_btnAdd,
            *m_btnEdit;

   wxListCtrl *m_listctrl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxCustomHeadersDialog)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxComposeHeadersDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(Button_EditCustom, wxComposeHeadersDialog::OnEditCustomHeaders)
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
const wxChar *wxComposeHeadersDialog::ms_headerNames[] =
{
   gettext_noop("&To"),
   gettext_noop("&Cc"),
   gettext_noop("&Bcc")
};

const wxChar *wxComposeHeadersDialog::ms_profileNamesDefault[Header_Max];

void wxComposeHeadersDialog::InitStaticArrays()
{
   if ( !ms_profileNamesDefault[0] )
   {
      ms_profileNamesDefault[0] = MP_COMPOSE_TO;
      ms_profileNamesDefault[1] = MP_COMPOSE_CC;
      ms_profileNamesDefault[2] = MP_COMPOSE_BCC;
   }
}

wxComposeHeadersDialog::wxComposeHeadersDialog(Profile *profile,
                                               wxWindow *parent)
                      : wxOptionsPageSubdialog(profile, parent,
                                               _("Configure headers for "
                                                 "message composition"),
                                               _T("ComposeHeaders"))
{
   InitStaticArrays();

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxString foldername = profile->GetFolderName();
   wxString labelBox;
   if ( !foldername.empty() )
      labelBox.Printf(_("&Headers for folder '%s'"), foldername.c_str());
   else
      labelBox.Printf(_("Default headers"));
   wxStaticBox *box = CreateStdButtonsAndBox(labelBox);

   // and a button to invoke the dialog for configuring other headers
   wxButton *btnEditCustom = new wxButton(this, Button_EditCustom,
                                          _("&More..."));

   wxWindow *btnOk = FindWindow(wxID_OK);
   c = new wxLayoutConstraints();
   c->right.LeftOf(btnOk, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(btnOk, wxTop);
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   btnEditCustom->SetConstraints(c);

   // a static message telling where is what
   wxStaticText *msg1 = new wxStaticText(this, -1, _("Recipient"));
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
      wxStaticText *label = new wxStaticText(this, -1, wxGetTranslation(ms_headerNames[header]));
      m_textvalues[header] = new wxTextCtrl(this, TextCtrlId + header);

      // set the constraints
      c = new wxLayoutConstraints();
      // we assume that the message above is longer than any of the field
      // names anyhow
      c->width.SameAs(msg1, wxWidth);
      c->left.SameAs(msg1, wxLeft);
      c->height.AsIs();
      c->centreY.SameAs(m_textvalues[header], wxCentreY);

      label->SetConstraints(c);

      c = new wxLayoutConstraints();
      if ( !last )
      {
         c->top.SameAs(msg2, wxTop, 4*LAYOUT_Y_MARGIN);
      }
      else
      {
         c->top.Below(last, LAYOUT_Y_MARGIN);
      }
      c->left.RightOf(label, 3*LAYOUT_X_MARGIN);
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->height.AsIs();

      last = m_textvalues[header];
      last->SetConstraints(c);
   }

   // set the minimal and initial window size
   SetDefaultSize(4*wBtn, 8*hBtn);
}

void wxComposeHeadersDialog::OnEditCustomHeaders(wxCommandEvent& /* event */)
{
   wxCustomHeadersDialog dlg(m_profile, GetParent());

   (void)dlg.ShowModal();
}

bool wxComposeHeadersDialog::TransferDataToWindow()
{
   // disable environment variable expansion here because we want the user to
   // edit the real value stored in the config
   ProfileEnvVarSave suspend(m_profile, false);

   // default value for the text ctrl
   wxString def;

   for ( size_t header = 0; header < Header_Max; header++ )
   {
      def = m_profile->readEntry(ms_profileNamesDefault[header], _T(""));

      m_textvalues[header]->SetValue(def);
      m_textvalues[header]->DiscardEdits();
   }

   return TRUE;
}

bool wxComposeHeadersDialog::TransferDataFromWindow()
{
   wxString def;

   for ( size_t header = 0; header < Header_Max; header++ )
   {
      if ( m_textvalues[header]->IsModified() )
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
                      : wxSelectionsOrderDialog(parent,
                                               _("&Headers"),
                                               _("Configure headers to show "
                                                 "in message view"),
                                               _T("MsgViewHeaders"))
{
   m_profile = profile;
   SafeIncRef(profile);

   SetDefaultSize(3*wBtn, 10*hBtn);
}

wxMsgViewHeadersDialog::~wxMsgViewHeadersDialog()
{
   SafeDecRef(m_profile);
}

bool wxMsgViewHeadersDialog::TransferDataToWindow()
{
   // colon separated list of all the headers (even if they're not all shown)
   wxString allHeaders = READ_CONFIG(m_profile, MP_MSGVIEW_ALL_HEADERS);

   // the headers we do show: prepend ':' in front because it allows us to
   // check very easily whether the header is shown or not: we just search for
   // ":<header>:" (of course, without the ':' in front this wouldn't work for
   // the first header)
   wxString shownHeaders(_T(':'));
   shownHeaders += READ_CONFIG(m_profile, MP_MSGVIEW_HEADERS);

   size_t n = 0;
   wxString header;  // accumulator
   for ( const wxChar *p = allHeaders.c_str(); *p != '\0'; p++ )
   {
      if ( *p == ':' )
      {
         wxASSERT_MSG( !!header, _T("header name shouldn't be empty") );

         m_checklstBox->Append(header);

         // check the item only if we show it
         header.Prepend(_T(':'));
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
#if wxCHECK_VERSION(2, 3, 2)
   size_t count = (size_t)m_checklstBox->GetCount();
#else
   size_t count = (size_t)m_checklstBox->Number();
#endif
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
   m_hasChanges = shownHeaders != READ_CONFIG(m_profile, MP_MSGVIEW_HEADERS);
   if ( m_hasChanges )
   {
      m_hasChanges = TRUE;

      m_profile->writeEntry(MP_MSGVIEW_HEADERS, shownHeaders);
   }

   // update the list of known entries too
   if ( allHeaders != READ_CONFIG(m_profile, MP_MSGVIEW_ALL_HEADERS) )
   {
      // it shouldn't count as "change" really, so we don't set the flag
      m_profile->writeEntry(MP_MSGVIEW_ALL_HEADERS, allHeaders);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxCustomHeaderDialog - edit the value of one custom header field
// ----------------------------------------------------------------------------

wxCustomHeaderDialog::wxCustomHeaderDialog(Profile *profile,
                                           wxWindow *parent,
                                           bool letUserChooseType)
                     : wxOptionsPageSubdialog(profile, parent,
                                              _("Edit custom headers"),
                                              _T("CustomHeader"))
{
   // init the member vars
   // --------------------

   m_headerType = CustomHeader_Invalid;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // [Ok], [Cancel] and a box around everything else
   wxString foldername = profile->GetFolderName();
   wxString labelBox;
   if ( !foldername.empty() )
      labelBox.Printf(_("Custom header for folder '%s'"), foldername.c_str());
   else
      labelBox.Printf(_("Default custom header"));
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
   m_textctrlName = new wxPTextEntry(_T("CustomHeaderName"), this);

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
   m_textctrlValue = new wxPTextEntry(_T("CustomHeaderValue"), this);

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
      static const wxChar *radioItems[CustomHeader_Max] =
      {
         gettext_noop("news postings"),
         gettext_noop("mail messages"),
         gettext_noop("all messages")
      };

      wxString *radioStrings = new wxString[ (size_t) CustomHeader_Max];
      for ( size_t n = 0; n < CustomHeader_Max; n++ )
      {
         radioStrings[n] = wxGetTranslation(radioItems[n]);
      }

      // in this mode we always save the value in the profile, so we don't need
      // the checkbox - but we have instead a radio box allowing the user to
      // choose for which kind of message he wants to use this header
      m_radioboxType = new wxPRadioBox(_T("CurstomHeaderType"),
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

   // check that this is a valid header name
   for ( const wxChar *pc = m_headerName.c_str(); *pc; pc++ )
   {
      // RFC 822 allows '/' but we don't because it has a special meaning for
      // the profiles/wxConfig; other characters are excluded in accordance
      // with the definition of the header name in the RFC
      wxChar c = *pc;
      if ( c < 32 || c > 126 || c == ':' || c == '/' )
      {
         wxLogError(_("The character '%c' is invalid in the header name, "
                      "please replace or remove it."), c);
         return false;
      }
   }

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
                                              _T("CustomHeaders"))
{
   // init member data
   // ----------------
   m_profile = profile;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // [Ok], [Cancel] and a box around everything else
   wxString foldername = profile->GetFolderName();
   wxString labelBox;
   if ( !foldername.empty() )
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
   m_listctrl = new wxPListCtrl(_T("HeaderEditList"), this, -1,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxBORDER_SUNKEN);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnDelete, 3*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listctrl->SetConstraints(c);

   // listctrl initialization
   // -----------------------

   // create the imagelist
   static const wxChar *iconNames[] =
   {
      _T("image_news"),
      _T("image_mail"),
      _T("image_both")
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
      FAIL_MSG(_T("can't set item info in listctrl"));
   }

   if ( m_listctrl->SetItem(index, 1, headerValue) == -1 )
   {
      FAIL_MSG(_T("can't set item info in listctrl"));
   }

   wxListItem li;
   li.m_mask = wxLIST_MASK_IMAGE;
   li.m_itemId = index;
   li.m_col = 0;
   if ( !m_listctrl->SetItem(li) )
   {
      FAIL_MSG(_T("can't change items icon in listctrl"));
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
      FAIL_MSG(_T("can't insert item into listctrl"));
   }

   if ( m_listctrl->SetItem(index, 1, headerValue) == -1 )
   {
      FAIL_MSG(_T("can't set item info in listctrl"));
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
      FAIL_MSG(_T("can't get header value from listctrl"));
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
   // not really efficient - we could update only the entries which really
   // changed instead of rewriting everything - but hardly crucial

   // delete all old entries
   wxArrayString entriesToDelete;
   String name;
   Profile::EnumData cookie;
   bool cont = m_profile->GetFirstEntry(name, cookie);
   while ( cont )
   {
      if ( name.StartsWith(CUSTOM_HEADERS_PREFIX) )
      {
         // don't delete entries while enumerating them, this confused the
         // enumeration
         entriesToDelete.Add(name);
      }

      cont = m_profile->GetNextEntry(name, cookie);
   }

   size_t count = entriesToDelete.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_profile->DeleteEntry(entriesToDelete[n]);
   }

   // write the values of the new ones
   String pathBase = CUSTOM_HEADERS_PREFIX;
   pathBase += ':';

   // remember if we have the Organization header among them
   bool hasOrg = false;

   wxArrayString headersFor[CustomHeader_Max];
   String headerName, headerValue;
   int nItems = m_listctrl->GetItemCount();
   for ( int nItem = 0; nItem < nItems; nItem++ )
   {
      // retrieve the info about this header
      GetHeader(nItem, &headerName, &headerValue);
      int type = m_headerTypes[(size_t)nItem];

      // don't write back the default organization header, it is stored in its
      // own config setting
      if ( headerName == ORGANIZATION )
      {
         hasOrg = true;

         if ( type == CustomHeader_Both )
         {
            continue;
         }
         //else: keep this one as it has a more restricted type than
         //      MP_ORGANIZATION and will override it in the future
      }

      // write the corresponding entry
      wxString path;
      path << pathBase << headerName << ':' << gs_customHeaderSubgroups[type];

      m_profile->writeEntry(path, headerValue);

      // and remember that we have got some custom headers of this type
      headersFor[type].Add(headerName);
   }

   // finally write the names of the headers to use
   pathBase = CUSTOM_HEADERS_PREFIX;
   for ( size_t type = 0; type < CustomHeader_Max; type++ )
   {
      const String name = pathBase + gs_customHeaderSubgroups[type];
      const String value = strutil_flatten_array(headersFor[type]);
      if ( m_profile->readEntry(name, _T("")) != value )
         m_profile->writeEntry(name, value);
   }

   // remove the organization header if it hadn't been specified: we must do it
   // or otherwise deleting this header in the dialog wouldn't work as it is
   // added to the list of the initial dialog items by GetCustomHeaders()
   // implicitly
   if ( !hasOrg )
   {
      m_profile->DeleteEntry(MP_ORGANIZATION);
   }

   return TRUE;
}

void wxCustomHeadersDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   event.Enable( m_listctrl->GetSelectedItemCount() > 0 );
}

void wxCustomHeadersDialog::OnEdit(wxCommandEvent& WXUNUSED(event))
{
   size_t sel = 0; // inititialize it to fix compiler warnings
   CHECK_RET( GetSelection(&sel), _T("button should be disabled") );

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
   size_t sel = 0; // inititialize it to fix compiler warnings
   CHECK_RET( GetSelection(&sel), _T("button should be disabled") );

   m_listctrl->DeleteItem(sel);
   m_headerTypes.RemoveAt(sel);
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

bool ConfigureCustomHeader(Profile *profile,
                           wxWindow *parent,
                           String *headerName, String *headerValue,
                           bool *storedInProfile,
                           CustomHeaderType type)
{
   bool letUserChooseType = type == CustomHeader_Invalid;
   wxCustomHeaderDialog dlg(profile, parent, letUserChooseType);

   if ( dlg.ShowModal() != wxID_OK )
   {
      // cancelled
      return false;
   }

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
      // update the value of this headers
      String path;
      path << CUSTOM_HEADERS_PREFIX << ':' << *headerName << ':'
            << gs_customHeaderSubgroups[type];

      profile->writeEntry(path, *headerValue);

      // and add this header to thel ist of headers to use
      path.clear();
      path << CUSTOM_HEADERS_PREFIX << gs_customHeaderSubgroups[type];
      wxArrayString
         headerNames = strutil_restore_array(profile->readEntry(path, _T("")));
      if ( headerNames.Index(*headerName) == wxNOT_FOUND )
      {
         headerNames.Add(*headerName);
         profile->writeEntry(path, strutil_flatten_array(headerNames));
      }
      //else: it's already there
   }

   return true;
}

bool ConfigureCustomHeaders(Profile *profile, wxWindow *parent)
{
   wxCustomHeadersDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK;
}

size_t GetCustomHeaders(Profile *profile,
                        CustomHeaderType typeWanted,
                        wxArrayString *names,
                        wxArrayString *values,
                        wxArrayInt *types)
{
   CHECK( profile && names && values, (size_t)-1, _T("invalid parameter") );

   // init
   names->Empty();
   values->Empty();
   if ( types )
      types->Empty();

   // examine all custom header types
   String pathBase = CUSTOM_HEADERS_PREFIX;
   for ( int type = 0; type < CustomHeader_Max; type++ )
   {
      // check whether we're interested in the entries of this type at all
      // (CustomHeader_Max means to take all entries)
      if ( (typeWanted != CustomHeader_Max) &&
           (typeWanted != CustomHeader_Both) &&
           (typeWanted != type) )
      {
         // no, we want only "Mail" entries and the current type is "News" (or
         // vice versa), skip this type
         continue;
      }

      // get the names of custom headers for this type
      String hdrs
          = profile->readEntry(pathBase + gs_customHeaderSubgroups[type], _T(""));

      // if we have any ...
      if ( !hdrs.empty() )
      {
         // ... add all these headers
         wxArrayString headerNames = strutil_restore_array(hdrs);

         size_t count = headerNames.GetCount();
         for ( size_t n = 0; n < count; n++ )
         {
            String name = headerNames[n];
            String path = pathBase;
            path << ':' << name
                 << ':' << gs_customHeaderSubgroups[type];

            names->Add(name);
            values->Add(profile->readEntry(path, _T("")));
            if ( types )
               types->Add(type);
         }
      }
   }

   // treat the organization field specially - it's the most common custom
   // header so we have a separate config setting for it
   if ( names->Index(ORGANIZATION) == wxNOT_FOUND )
   {
      String organization = READ_CONFIG(profile, MP_ORGANIZATION);
      if ( !organization.empty() )
      {
         names->Add(ORGANIZATION);
         values->Add(organization);
         if ( types )
            types->Add(CustomHeader_Both);
      }
   }

   return names->GetCount();
}

bool ConfigureInReplyToHeader(String *messageid, wxWindow *parent)
{
   CHECK( messageid, false, _T("NULL messageid") );

   wxTextEntryDialog dlg
                     (
                      parent,
                      _("Message is a reply to (empty string means that this\n"
                         "message is a start of new thread and not a reply at "
                         "all):"),
                      _("Is this message a reply?"),
                      *messageid
                     );

   if ( dlg.ShowModal() != wxID_OK )
      return false;

   *messageid = dlg.GetValue();

   return true;
}

