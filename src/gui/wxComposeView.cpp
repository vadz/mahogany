///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxComposeView.cpp - composer GUI code
// Purpose:     shows a frame containing the header controls and editor window
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#  pragma  implementation "wxComposeView.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "sysutil.h"

#  include "PathFinder.h"
#  include "Profile.h"
#  include "MHelp.h"
#  include "MFrame.h"

#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include <wx/process.h>
#  include <wx/sizer.h>
#  include <wx/menu.h>
#  include <wx/stattext.h>
#  include <wx/textctrl.h>

#  include <ctype.h>          // for isspace()
#ifdef __CYGWIN__
#  include <sys/unistd.h>     // for getpid()
#endif
#endif // USE_PCH

#include <wx/filename.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include <wx/process.h>
#include <wx/mimetype.h>
#include <wx/tokenzr.h>
#include <wx/textbuf.h>
#include <wx/fontutil.h>      // for wxNativeFontInfo
// windows.h included from wx/fontutil.h under Windows #defines this
#ifdef __CYGWIN__
#  undef SendMessage
#endif

#include "wx/persctrl.h"

#include "Mdefaults.h"
#include "Mpers.h"

#include "Address.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "Message.h"
#include "SendMessage.h"

#include "MDialogs.h"
#include "HeadersDialogs.h"

#include "gui/wxIconManager.h"

#include "gui/wxIdentityCombo.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxComposeView.h"

#include "MessageEditor.h"

#include "adb/AdbEntry.h"
#include "adb/AdbManager.h"

#include "MessageTemplate.h"
#include "TemplateDialog.h"
#include "AttachDialog.h"

#include "MModule.h"
#include "modules/Calendar.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADB_SUBSTRINGEXPANSION;
extern const MOption MP_ALWAYS_USE_EXTERNALEDITOR;
extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_CC;
extern const MOption MP_COMPOSE_SHOW_FROM;
extern const MOption MP_COMPOSE_TO;
extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_CVIEW_BGCOLOUR;
extern const MOption MP_CVIEW_COLOUR_HEADERS;
extern const MOption MP_CVIEW_FGCOLOUR;
extern const MOption MP_CVIEW_FONT;
extern const MOption MP_CVIEW_FONT_DESC;
extern const MOption MP_CVIEW_FONT_SIZE;
extern const MOption MP_DRAFTS_AUTODELETE;
extern const MOption MP_DRAFTS_FOLDER;
extern const MOption MP_EXTERNALEDITOR;
extern const MOption MP_HOSTNAME;
extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_OUTGOINGFOLDER;
extern const MOption MP_SENDMAILCMD;
extern const MOption MP_SMTPHOST;
extern const MOption MP_USEOUTGOINGFOLDER;
extern const MOption MP_USEVCARD;
extern const MOption MP_USE_SENDMAIL;
extern const MOption MP_VCARD;

#ifdef OS_UNIX
extern const MOption MP_USE_SENDMAIL;
#endif // OS_UNIX

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ASK_FOR_EXT_EDIT;
extern const MPersMsgBox *M_MSGBOX_ASK_VCARD;
extern const MPersMsgBox *M_MSGBOX_CONFIG_NET_FROM_COMPOSE;
extern const MPersMsgBox *M_MSGBOX_DRAFT_AUTODELETE;
extern const MPersMsgBox *M_MSGBOX_DRAFT_SAVED;
extern const MPersMsgBox *M_MSGBOX_FIX_TEMPLATE;
extern const MPersMsgBox *M_MSGBOX_MIME_TYPE_CORRECT;
extern const MPersMsgBox *M_MSGBOX_SEND_EMPTY_SUBJECT;
extern const MPersMsgBox *M_MSGBOX_ASK_SAVE_HEADERS;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the header used to indicate that a message is our draft
#define HEADER_IS_DRAFT "X-M-Draft"

// the header used for storing the composer geometry
#define HEADER_GEOMETRY "X-M-Geometry"

// the possible values for HEADER_GEOMETRY
#define GEOMETRY_ICONIZED "I"
#define GEOMETRY_MAXIMIZED "M"
#define GEOMETRY_FORMAT "%dx%d-%dx%d"

// the composer frame title
#define COMPOSER_TITLE _("Message Composition")

// separate multiple addresses with commas
#define CANONIC_ADDRESS_SEPARATOR   ", "

// code here was written with assumption that x and y margins are the same
#define LAYOUT_MARGIN LAYOUT_X_MARGIN

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

M_LIST(ComposerList, wxComposeView *);

static ComposerList gs_listOfAllComposers;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return the string containing the full mime type for the given filename (uses
// its extension)
static wxString GetMimeTypeFromFilename(const wxString& filename)
{
   wxString strExt;
   wxSplitPath(filename, NULL, NULL, &strExt);

   wxString strMimeType;
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
   wxFileType *fileType = mimeManager.GetFileTypeFromExtension(strExt);
   if ( (fileType == NULL) || !fileType->GetMimeType(&strMimeType) )
   {
      // can't find MIME type from file extension, set some default one
      strMimeType = "APPLICATION/OCTET-STREAM";
   }

   delete fileType;  // may be NULL, ok

   return strMimeType;
}

// return a transparent bitmap
static wxBitmap GetTransparentBitmap(const char *name)
{
   wxBitmap bmp = mApplication->GetIconManager()->GetBitmap(name);

#ifdef OS_WIN
   bmp.SetMask(new wxMask(bmp, wxColour(0xc6, 0xc6, 0xc6)));
#endif // OS_WIN

   return bmp;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/*
   A wxRcptControl allows to enter one or several recipient addresses but they
   are all limited to the same type (i.e. all "To" or all "Cc"). It consists of
   a choice control allowing to select the type, the text entry for the address
   and some buttons.

   There are 2 types of wxRcptControl: wxRcptMainControl is the [only] one
   which is shown initially and it cannot be removed. When the address is
   entered into it, the user may press "Add" button to the right of it to add a
   new recipient control group, a wxRcptExtraControl. This type of recipient
   controls can be removed (using a special "Delete" button near it) and it
   can't add any other controls itself.

   Both kinds of the controls may be expanded by using the "Expand" button they
   have in common.
 */

class wxRcptAddButton;
class wxRcptRemoveButton;
class wxRcptExpandButton;
class wxAddressTextCtrl;
class wxRcptTypeChoice;

// ----------------------------------------------------------------------------
// wxRcptControl: all controls in the row corresponding to one recipient
// ----------------------------------------------------------------------------

class wxRcptControl
{
public:
   wxRcptControl(wxComposeView *cv)
   {
      m_composeView = cv;

      m_choice = NULL;
      m_text = NULL;
      m_btnExpand = NULL;
   }

   virtual ~wxRcptControl();

   // create the controls and return a sizer containing them
   virtual wxSizer *CreateControls(wxWindow *parent);

   // initialize the controls
   void InitControls(const String& value, wxComposeView::RecipientType rt);

   // get our text control
   wxAddressTextCtrl *GetText() const { return m_text; }

   // get the currently selected address type
   wxComposeView::RecipientType GetType() const;

   // change the address type
   void SetType(wxComposeView::RecipientType rcptType);

   // get the current value of the text field
   wxString GetValue() const;

   // starting from now, all methods are for the wxRcptXXX controls only

   // change type of this one -- called by choice
   virtual void OnTypeChange(wxComposeView::RecipientType rcptType);

   // expand our text: called by the "Expand" button and
   // wxAddressTextCtrl::OnChar()
   void OnExpand();

   // is it enabled (disabled if type == none)?
   bool IsEnabled() const;

   // get the composer
   wxComposeView *GetComposer() const { return m_composeView; }

protected:
   // called from CreateControls()
   virtual wxAddressTextCtrl *CreateText(wxWindow *parent) = 0;

   // access the choice control
   wxRcptTypeChoice *GetChoice() const { return m_choice; }

   // the back pointer to the composer
   wxComposeView *m_composeView;

private:
   // our controls
   wxRcptTypeChoice *m_choice;
   wxAddressTextCtrl *m_text;
   wxRcptExpandButton *m_btnExpand;
};

// ----------------------------------------------------------------------------
// wxRcptMainControl: this one has "add" button and can't be removed
// ----------------------------------------------------------------------------

class wxRcptMainControl : public wxRcptControl
{
public:
   wxRcptMainControl(wxComposeView *cv) : wxRcptControl(cv)
   {
      m_btnAdd = NULL;
   }

   virtual wxSizer *CreateControls(wxWindow *parent);

   // notify the composer that the default recipient type changed
   virtual void OnTypeChange(wxComposeView::RecipientType rcptType);

   // callback for "add new recipient" button
   void OnAdd();

   virtual ~wxRcptMainControl();

protected:
   virtual wxAddressTextCtrl *CreateText(wxWindow *parent);

private:
   wxRcptAddButton *m_btnAdd;
};

// ----------------------------------------------------------------------------
// wxRcptExtraControl: an additional recipient, may be removed
// ----------------------------------------------------------------------------

class wxRcptExtraControl : public wxRcptControl
{
public:
   wxRcptExtraControl(wxComposeView *cv) : wxRcptControl(cv)
   {
      // we're always inserted in the beginning, see AddRecipientControls()
      m_index = 0;
      m_btnRemove = NULL;
   }

   // remove this one - called by button
   void OnRemove() { m_composeView->OnRemoveRcpt(m_index); }

   // increment our index (called when another control is inserted before us)
   void IncIndex()
   {
      m_index++;
   }

   // decrement our index (presumably because another control was deleted
   // before us)
   void DecIndex()
   {
      ASSERT_MSG( m_index > 0, _T("shouldn't be called") );

      m_index--;
   }

   virtual wxSizer *CreateControls(wxWindow *parent);

   virtual ~wxRcptExtraControl();

protected:
   virtual wxAddressTextCtrl *CreateText(wxWindow *parent);

private:
   wxRcptRemoveButton *m_btnRemove;

   // our index in m_composeView->m_rcptControls
   size_t m_index;
};

// ----------------------------------------------------------------------------
// wxRcptTypeChoice is first part of wxRcptControl
// ----------------------------------------------------------------------------

class wxRcptTypeChoice : public wxChoice
{
public:
   wxRcptTypeChoice(wxRcptControl *rcptControl, wxWindow *parent);

   // callbacks
   void OnChoice(wxCommandEvent& event);

private:
   // the back pointer to the entire group of controls
   wxRcptControl *m_rcptControl;

   static const wxChar *ms_addrTypes[wxComposeView::Recipient_Max];

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxTextCtrlProcessingEnter: simple base class for wxSubjectTextCtrl and
//                            wxAddressTextCtrl which handles Enter in the same
//                            way as Tab
// ----------------------------------------------------------------------------

class wxTextCtrlProcessingEnter : public wxTextCtrl
{
public:
   wxTextCtrlProcessingEnter(wxWindow *parent, long style = 0)
      : wxTextCtrl(parent,
                   -1,
                   _T(""),
                   wxDefaultPosition,
                   wxDefaultSize,
                   style | wxTE_PROCESS_ENTER)
      {
      }

protected:
   void OnEnter(wxCommandEvent& event);

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxSubjectTextCtrl: text control for the subject
// ----------------------------------------------------------------------------

class wxSubjectTextCtrl : public wxTextCtrlProcessingEnter
{
public:
   wxSubjectTextCtrl(wxWindow *parent) : wxTextCtrlProcessingEnter(parent) { }
};

// ----------------------------------------------------------------------------
// wxAddressTextCtrl: specialized text control which processes TABs to expand
// the text it contains and also notifies parent (i.e. wxComposeView) when it
// is modified.
//
// This class is now an ABC with 2 derivations below - this is done more for
// conveniene of having 2 different classes for the controls which play
// different roles rather than because it is really needed.
// ----------------------------------------------------------------------------

class wxAddressTextCtrl : public wxTextCtrlProcessingEnter
{
public:
   // ctor
   wxAddressTextCtrl(wxWindow *parent, wxRcptControl *rcptControl);

   // expand the text in the control using the address book(s)
   Composer::RecipientType DoExpand();

   // callbacks
   void OnChar(wxKeyEvent& event);

protected:
   // accessors for the derived classes
   wxComposeView *GetComposer() const { return m_rcptControl->GetComposer(); }
   wxRcptControl *GetRcptControl() const { return m_rcptControl; }

private:
   // the recipient control we're part of
   wxRcptControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxMainAddressTextCtrl: wxAddressTextCtrl which allows the user to add new
// recipients (i.e. the unique top address entry in the compose frame)
// ----------------------------------------------------------------------------

class wxMainAddressTextCtrl : public wxAddressTextCtrl
{
public:
   // we do need wxRcptMainControl and not any wxRcptControl here, see OnEnter
   wxMainAddressTextCtrl(wxWindow *parent, wxRcptMainControl *rcptControl)
      : wxAddressTextCtrl(parent, rcptControl)
      {
      }

   // callbacks
   void OnEnter(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxExtraAddressTextCtrl: part of wxRcptExtraControl
// ----------------------------------------------------------------------------

class wxExtraAddressTextCtrl : public wxAddressTextCtrl
{
public:
   wxExtraAddressTextCtrl(wxRcptControl *rcptControl, wxWindow *parent)
      : wxAddressTextCtrl(parent, rcptControl)
      {
      }

   // callbacks
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxRcptExpandButton: small button used to expand the address entered
// ----------------------------------------------------------------------------

class wxRcptExpandButton : public wxBitmapButton
{
public:
   wxRcptExpandButton(wxRcptControl *rcptControl, wxWindow *parent)
      : wxBitmapButton(parent,
                       -1,
                       GetTransparentBitmap("tb_lookup"),
                       wxDefaultPosition,
                       wxDefaultSize,
                       wxBORDER_NONE)
      {
         m_rcptControl = rcptControl;
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnExpand(); }

private:
   // the back pointer to the entire group of controls
   wxRcptControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxRcptAddButton: small button which is used to add a new recipient
// ----------------------------------------------------------------------------

class wxRcptAddButton : public wxBitmapButton
{
public:
   wxRcptAddButton(wxRcptMainControl *rcptControl, wxWindow *parent)
      : wxBitmapButton(parent,
                       -1,
                       GetTransparentBitmap("tb_new"),
                       wxDefaultPosition,
                       wxDefaultSize,
                       wxBORDER_NONE)
      {
         m_rcptControl = rcptControl;

         SetToolTip(_("Create a new recipient entry"));
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnAdd(); }

private:
   // the back pointer to the entire group of controls
   wxRcptMainControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxRcptRemoveButton: small button which is used to remove previously added
// recipient controls
// ----------------------------------------------------------------------------

class wxRcptRemoveButton : public wxBitmapButton
{
public:
   wxRcptRemoveButton(wxRcptExtraControl *rcptControl, wxWindow *parent)
      : wxBitmapButton(parent,
                       -1,
                       GetTransparentBitmap("tb_trash"),
                       wxDefaultPosition,
                       wxDefaultSize,
                       wxBORDER_NONE)
      {
         m_rcptControl = rcptControl;

         SetToolTip(_("Delete this address from the message recipients list"));
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnRemove(); }

private:
   // the back pointer to the entire group of controls
   wxRcptExtraControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables &c
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxComposeView, wxMFrame)

BEGIN_EVENT_TABLE(wxComposeView, wxMFrame)
   // process termination notification
   EVT_END_PROCESS(HelperProcess_Editor, wxComposeView::OnExtEditorTerm)

   // identity combo notification
   EVT_CHOICE(IDC_IDENT_COMBO, wxComposeView::OnIdentChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptTypeChoice, wxChoice)
   EVT_CHOICE(-1, wxRcptTypeChoice::OnChoice)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxTextCtrlProcessingEnter, wxTextCtrl)
   EVT_TEXT_ENTER(-1, wxTextCtrlProcessingEnter::OnEnter)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAddressTextCtrl, wxTextCtrlProcessingEnter)
   EVT_CHAR(wxAddressTextCtrl::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMainAddressTextCtrl, wxAddressTextCtrl)
   EVT_TEXT_ENTER(-1, wxMainAddressTextCtrl::OnEnter)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxExtraAddressTextCtrl, wxAddressTextCtrl)
   EVT_UPDATE_UI(-1, wxExtraAddressTextCtrl::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptExpandButton, wxButton)
   EVT_BUTTON(-1, wxRcptExpandButton::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptAddButton, wxButton)
   EVT_BUTTON(-1, wxRcptAddButton::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptRemoveButton, wxButton)
   EVT_BUTTON(-1, wxRcptRemoveButton::OnButton)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Composer::Options
// ----------------------------------------------------------------------------

Composer::Options::Options()
{
   m_fontFamily =
   m_fontSize = -1;
}

void Composer::Options::Read(Profile *profile)
{
   CHECK_RET( profile, _T("NULL profile in Composer::Options::Read") );

   // colours
   GetColourByName(&m_fg, READ_CONFIG(profile, MP_CVIEW_FGCOLOUR), "black");
   GetColourByName(&m_bg, READ_CONFIG(profile, MP_CVIEW_BGCOLOUR), "white");

   // font
   m_font = READ_CONFIG_TEXT(profile, MP_CVIEW_FONT_DESC);
   if ( m_font.empty() )
   {
      m_fontFamily = GetFontFamilyFromProfile(profile, MP_CVIEW_FONT);
      m_fontSize = READ_CONFIG(profile, MP_CVIEW_FONT_SIZE);
   }
}

wxFont Composer::Options::GetFont() const
{
   wxFont font;
   if ( !m_font.empty() )
   {
      wxNativeFontInfo fontInfo;
      if ( fontInfo.FromString(m_font) )
      {
         font.SetNativeFontInfo(fontInfo);
      }
   }

   if ( !font.Ok() )
   {
      font = wxFont(m_fontSize,
                    m_fontFamily,
                    wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL);
   }

   return font;
}

// ----------------------------------------------------------------------------
// EditorContentPart
// ----------------------------------------------------------------------------

void EditorContentPart::Init()
{
   m_Data = NULL;
}

void EditorContentPart::SetMimeType(const String& mimeType)
{
   m_MimeType = mimeType;
}

void EditorContentPart::SetData(void *data,
                                size_t length,
                                const char *name,
                                const char *filename)
{
   ASSERT_MSG( data != NULL, _T("NULL data is invalid in EditorContentPart::SetData!") );

   m_Data = data;
   m_Length = length;
   m_Type = Type_Data;

   if ( name )
   {
      m_Name = name;
      m_FileName = name;
   }
   if ( filename )
   {
      if ( !name )
         m_Name = filename;
      m_FileName = filename;
   }

   SetDisposition("INLINE");
}

void EditorContentPart::SetFile(const String& filename)
{
   ASSERT_MSG( !filename.empty(), _T("a file attachment must have a valid file") );

   m_Name =
   m_FileName = filename;
   m_Type = Type_File;

   if ( m_Disposition.empty() )
      SetDisposition("ATTACHMENT");
}

void EditorContentPart::SetName(const String& name)
{
   m_Name = name;
}

void EditorContentPart::SetDisposition(const String& disposition)
{
   m_Disposition = disposition;
}

void EditorContentPart::SetText(const String& text)
{
   SetMimeType("TEXT/PLAIN");

   m_Type = Type_Text;
   m_Text = text;

   SetDisposition("INLINE");
}

EditorContentPart::~EditorContentPart()
{
   if ( m_Type == Type_Data )
   {
      free(m_Data);
   }
}

// ----------------------------------------------------------------------------
// wxRcptControl
// ----------------------------------------------------------------------------

wxSizer *wxRcptControl::CreateControls(wxWindow *parent)
{
   // create controls
   m_text = CreateText(parent);
   m_choice = new wxRcptTypeChoice(this, parent);
   m_btnExpand = new wxRcptExpandButton(this, parent);

   // add them to sizer
   wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
   sizer->Add(m_choice, 0, wxRIGHT | wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
   sizer->Add(m_text, 1, wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
   sizer->Add(m_btnExpand, 0, wxLEFT, LAYOUT_MARGIN);

   m_composeView->SetTextAppearance(m_text);

   return sizer;
}

void wxRcptControl::InitControls(const String& value,
                                  wxComposeView::RecipientType rt)
{
   CHECK_RET( m_choice, _T("must call CreateControls first") );

   m_choice->SetSelection(rt);
   m_text->SetValue(value);

   // init the m_btnExpand state
   OnTypeChange(rt);
}

wxRcptControl::~wxRcptControl()
{
   delete m_choice;
   delete m_text;
   delete m_btnExpand;
}

void wxRcptControl::OnTypeChange(wxComposeView::RecipientType rcptType)
{
   switch ( rcptType )
   {
      case Composer::Recipient_To:
      case Composer::Recipient_Cc:
      case Composer::Recipient_Bcc:
         m_btnExpand->Enable();
         m_btnExpand->SetToolTip(_("Expand the address using address books"));
         break;

      case Composer::Recipient_Newsgroup:
         // TODO-NEWS: we can't browse for newsgroups yet
         m_btnExpand->Disable();
         break;

      case Composer::Recipient_Fcc:
         // browse for folder now
         m_btnExpand->Enable();
         m_btnExpand->SetToolTip(_("Browse for folder"));
         break;

      case Composer::Recipient_None:
         m_btnExpand->Disable();
         break;

      case Composer::Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected rcpt type on wxRcptControl") );
   }
}

void wxRcptControl::OnExpand()
{
   switch ( GetType() )
   {
      case Composer::Recipient_To:
      case Composer::Recipient_Cc:
      case Composer::Recipient_Bcc:
         {
            Composer::RecipientType rcptType = m_text->DoExpand();
            if ( rcptType != Composer::Recipient_None &&
                  rcptType != Composer::Recipient_Max )
            {
               // update the type of the choice control
               SetType(rcptType);
            }
         }
         break;

      case Composer::Recipient_Fcc:
         // browse for folder
         {
            MFolder_obj folder(MDialog_FolderChoose
                               (
                                 m_composeView->GetFrame(),
                                 NULL,
                                 MDlg_Folder_NoFiles
                               ));
            if ( !folder )
            {
               // cancelled by user
               break;
            }

            // separate the new folder with a comma from the previous ones
            String fcc = m_text->GetValue();
            fcc.Trim();
            if ( !fcc.empty() && fcc[fcc.length() - 1] != ',' )
            {
               fcc += CANONIC_ADDRESS_SEPARATOR;
            }

            fcc += folder->GetFullName();

            m_text->SetValue(fcc);
         }
         break;

      case Composer::Recipient_Newsgroup:
      case Composer::Recipient_None:
      case Composer::Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected wxRcptControl::OnExpand() call") );
   }
}

bool wxRcptControl::IsEnabled() const
{
   return m_choice->GetSelection() != Composer::Recipient_None;
}

Composer::RecipientType wxRcptControl::GetType() const
{
   return (Composer::RecipientType)m_choice->GetSelection();
}

void wxRcptControl::SetType(Composer::RecipientType rcptType)
{
   m_choice->SetSelection(rcptType);

   OnTypeChange(rcptType);
}

wxString wxRcptControl::GetValue() const
{
   return m_text->GetValue();
}

// ----------------------------------------------------------------------------
// wxRcptMainControl
// ----------------------------------------------------------------------------

wxAddressTextCtrl *wxRcptMainControl::CreateText(wxWindow *parent)
{
   return new wxMainAddressTextCtrl(parent, this);
}

wxSizer *wxRcptMainControl::CreateControls(wxWindow *parent)
{
   wxSizer *sizer = wxRcptControl::CreateControls(parent);

   m_btnAdd = new wxRcptAddButton(this, parent);

   sizer->Add(m_btnAdd, 0, wxLEFT, LAYOUT_MARGIN);

   // TODO-NEWS: set to Newsgroup for News
   GetChoice()->Delete(Composer::Recipient_None);
   GetChoice()->SetSelection(Composer::Recipient_To);

   // init the m_btnExpand button here
   wxRcptControl::OnTypeChange(GetType());

   return sizer;
}

void wxRcptMainControl::OnTypeChange(Composer::RecipientType rcptType)
{
   wxRcptControl::OnTypeChange(rcptType);

   GetComposer()->OnRcptTypeChange(rcptType);
}

void wxRcptMainControl::OnAdd()
{
   // expand before adding (make this optional?)
   Composer::RecipientType addrType = GetText()->DoExpand();
   if ( addrType == Composer::Recipient_None )
   {
      // cancelled or address is invalid
      return;
   }

   // add new recipient(s)
   m_composeView->AddRecipients(GetText()->GetValue(), addrType);

   // clear the entry zone as recipient(s) were moved elsewhere
   GetText()->SetValue("");
}

wxRcptMainControl::~wxRcptMainControl()
{
   delete m_btnAdd;
}

// ----------------------------------------------------------------------------
// wxRcptExtraControl
// ----------------------------------------------------------------------------

wxAddressTextCtrl *wxRcptExtraControl::CreateText(wxWindow *parent)
{
   return new wxExtraAddressTextCtrl(this, parent);
}

wxSizer *wxRcptExtraControl::CreateControls(wxWindow *parent)
{
   wxSizer *sizer = wxRcptControl::CreateControls(parent);

   m_btnRemove = new wxRcptRemoveButton(this, parent);

   sizer->Add(m_btnRemove, 0, wxLEFT, LAYOUT_MARGIN);

   return sizer;
}

wxRcptExtraControl::~wxRcptExtraControl()
{
   delete m_btnRemove;
}

// ----------------------------------------------------------------------------
// wxRcptTypeChoice
// ----------------------------------------------------------------------------

const wxChar *wxRcptTypeChoice::ms_addrTypes[] =
{
   gettext_noop("To"),
   gettext_noop("Cc"),
   gettext_noop("Bcc"),
   gettext_noop("Newsgroup"),
   gettext_noop("Fcc"),
   gettext_noop("None"),
};

wxRcptTypeChoice::wxRcptTypeChoice(wxRcptControl *rcptControl, wxWindow *parent)
{
   // translate the strings
   wxString addrTypes[WXSIZEOF(ms_addrTypes)];
   for ( size_t n = 0; n < WXSIZEOF(ms_addrTypes); n++ )
   {
      addrTypes[n] = _(ms_addrTypes[n]);
   }

   Create(parent, -1,
          wxDefaultPosition, wxDefaultSize,
          WXSIZEOF(addrTypes), addrTypes);

   m_rcptControl = rcptControl;
}

void wxRcptTypeChoice::OnChoice(wxCommandEvent& event)
{
   // notify the others (including the composer indirectly)
   m_rcptControl->OnTypeChange((Composer::RecipientType)event.GetSelection());

   event.Skip();
}

// ----------------------------------------------------------------------------
// wxTextCtrlProcessingEnter
// ----------------------------------------------------------------------------

void wxTextCtrlProcessingEnter::OnEnter(wxCommandEvent& /* event */)
{
   // pass to the next control when <Enter> is pressed
   wxNavigationKeyEvent event;
   event.SetDirection(TRUE);       // forward
   event.SetWindowChange(FALSE);   // control change
   event.SetEventObject(this);

   GetParent()->GetEventHandler()->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// wxAddressTextCtrl
// ----------------------------------------------------------------------------

wxAddressTextCtrl::wxAddressTextCtrl(wxWindow *parent,
                                     wxRcptControl *rcptControl)
                 : wxTextCtrlProcessingEnter(parent, wxTE_PROCESS_TAB)
{
   m_rcptControl = rcptControl;
}

// expand the address when <TAB> is pressed
void wxAddressTextCtrl::OnChar(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   // we're only interested in TABs and only it's not a second TAB in a row
   if ( event.GetKeyCode() == WXK_TAB )
   {
      if ( IsModified() &&
           !event.ControlDown() && !event.ShiftDown() && !event.AltDown() )
      {
         // mark control as being "not modified" - if the user presses TAB
         // the second time go to the next window immediately after having
         // expanded the entry
         DiscardEdits();

         // DoExpand() normally checks if the text is empty and gives an error
         // message if it's true - because it doesn't make sense to press the
         // [Expand] button then. This behaviour is not desirable when the user
         // presses TAB, however, so check it here and just treat TAB normally
         // if the text is empty instead.
         if ( !GetValue().empty() )
         {
            m_rcptControl->OnExpand();

            // don't call event.Skip()
            return;
         }
         //else: text is empty, treat the TAB normally
      }
      //else: nothing because we're not interested in Ctrl-TAB, Shift-TAB &c -
      //      and also in the TABs if the last one was already a TAB
   }

   // let the text control process it normally: if it's a TAB this will make
   // the focus go to the next window
   event.Skip();
}

Composer::RecipientType wxAddressTextCtrl::DoExpand()
{
   Composer::RecipientType rcptType;

   switch ( m_rcptControl->GetType() )
   {
      case Composer::Recipient_To:
      case Composer::Recipient_Cc:
      case Composer::Recipient_Bcc:
         {
            String text = GetValue();

            rcptType = GetComposer()->ExpandRecipient(&text);

            if ( rcptType != Composer::Recipient_None )
            {
               SetValue(text);
               SetInsertionPointEnd();
            }
         }
         break;

      case Composer::Recipient_Newsgroup:
         // TODO-NEWS: we should expand the newsgroups too
         rcptType = Composer::Recipient_Newsgroup;
         break;

      case Composer::Recipient_Fcc:
         // TODO: we could expand the folder names -- but for now we don't
         rcptType = Composer::Recipient_Fcc;
         break;

      case Composer::Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected wxRcptControl type") );
         // fall through

      case Composer::Recipient_None:
         // nothing to do
         rcptType = Composer::Recipient_None;
         break;
   }

   return rcptType;
}

// ----------------------------------------------------------------------------
// wxMainAddressTextCtrl
// ----------------------------------------------------------------------------

void wxMainAddressTextCtrl::OnEnter(wxCommandEvent& /* event */)
{
   // if there is nothing in the address field, start editing instead, this is
   // handy to be able to switch to the composer window rapidly - tabbing to it
   // can take ages if there are a lot of recipient controls below us
   if ( GetValue().empty() )
   {
      GetComposer()->SetFocusToComposer();
   }
   else // add the contents of the control as a new recipient
   {
      // cast is safe as our rcpt control is always of this class (see ctor
      // signature)
      ((wxRcptMainControl *)GetRcptControl())->OnAdd();
   }
}

// ----------------------------------------------------------------------------
// wxExtraAddressTextCtrl
// ----------------------------------------------------------------------------

void wxExtraAddressTextCtrl::OnUpdateUI(wxUpdateUIEvent& event)
{
   // enable the text only if it has a valid type
   event.Enable(GetRcptControl()->IsEnabled());
}

// ----------------------------------------------------------------------------
// wxComposeView creation: static creator functions
// ----------------------------------------------------------------------------

wxComposeView *
CreateComposeView(Profile *profile,
                  const MailFolder::Params& params,
                  wxComposeView::Mode mode,
                  wxComposeView::MessageKind kind)
{
   wxWindow *parent = mApplication->TopLevelFrame();
   wxComposeView *cv = new wxComposeView
                           (
                              "ComposeViewNews",
                              mode,
                              kind,
                              parent
                           );

   cv->SetTemplate(params.templ);
   cv->SetTitle(COMPOSER_TITLE);
   cv->Create(parent, profile);

   return cv;
}

Composer *
Composer::CreateNewArticle(const MailFolder::Params& params,
                           Profile *profile)
{
   wxComposeView *cv = CreateComposeView(profile, params,
                                         wxComposeView::Mode_News,
                                         wxComposeView::Message_New);

   CHECK( cv, NULL, _T("failed to create composer for a new article") );

   cv->Launch();

   return cv;
}

Composer *
Composer::CreateFollowUpArticle(const MailFolder::Params& params,
                                Profile *profile,
                                Message *original)
{
   wxComposeView *cv = CreateComposeView(profile, params,
                                         wxComposeView::Mode_News,
                                         wxComposeView::Message_Reply);

   cv->SetOriginal(original);

   return cv;
}

Composer *
Composer::CreateNewMessage(const MailFolder::Params& params, Profile *profile)
{
   wxComposeView *cv = CreateComposeView(profile, params,
                                         wxComposeView::Mode_Mail,
                                         wxComposeView::Message_New);

   CHECK( cv, NULL, _T("failed to create composer for a new message") );

   cv->Launch();

   return cv;
}

Composer *
Composer::CreateReplyMessage(const MailFolder::Params& params,
                             Profile *profile,
                             Message *original)
{
   wxComposeView *cv = CreateComposeView(profile, params,
                                         wxComposeView::Mode_Mail,
                                         wxComposeView::Message_Reply);

   cv->SetOriginal(original);

   return cv;
}

Composer *
Composer::CreateFwdMessage(const MailFolder::Params& params,
                           Profile *profile,
                           Message *original)
{
   wxComposeView *cv = CreateComposeView(profile, params,
                                         wxComposeView::Mode_Mail,
                                         wxComposeView::Message_Forward);

   cv->SetOriginal(original);

   return cv;
}

Composer *
Composer::EditMessage(Profile *profile, Message *msg)
{
   CHECK( msg, NULL, _T("no message to edit?") );

   // first, create the composer

   // create dummy params object as we need one for CreateNewMessage()
   MailFolder::Params params("");

   wxComposeView *cv = (wxComposeView *)CreateNewMessage(params, profile);

   // next, import the message body in it
   cv->InsertMimePart(msg->GetTopMimePart());

   cv->ResetDirty();

   // finally, also import all headers except the ones in ignoredHeaders:
   //
   // first ignore those which are generated by the transport layer as it
   // doesn't make sense to generate them at all a MUA
   wxSortedArrayString ignoredHeaders;
   ignoredHeaders.Add("RECEIVED");
   ignoredHeaders.Add("RETURN-PATH");
   ignoredHeaders.Add("DELIVERED-TO");

   // second, ignore some headers which we always generate ourselves and don't
   // allow the user to override anyhow
   ignoredHeaders.Add("MESSAGE-ID");
   ignoredHeaders.Add("MIME-VERSION");
   ignoredHeaders.Add("CONTENT-TYPE");
   ignoredHeaders.Add("CONTENT-DISPOSITION");
   ignoredHeaders.Add("CONTENT-TRANSFER-ENCODING");

   wxString nameWithCase, value;
   HeaderIterator hdrIter = msg->GetHeaderIterator();
   while ( hdrIter.GetNext(&nameWithCase, &value) )
   {
      wxString name = nameWithCase.Upper();
      value = MailFolder::DecodeHeader(value);

      // test for some standard headers which need special treatment
      if ( name == "SUBJECT" )
         cv->SetSubject(value);
      else if ( name == "FROM" )
         cv->SetFrom(value);
      else if ( name == "TO" )
         cv->AddTo(value);
      else if ( name == "CC" )
         cv->AddCc(value);
      else if ( name == "BCC" )
         cv->AddBcc(value);
      else if ( ignoredHeaders.Index(name) == wxNOT_FOUND )
      {
         // compare case sensitively here as we always write HEADER_IS_DRAFT in
         // the same case
         if ( nameWithCase == HEADER_IS_DRAFT )
         {
            cv->SetDraft(msg);

            // the default value is "on", don't give the message if the user
            // had already changed it to "off" - this means he knows what he is
            // doing
            if ( !READ_CONFIG(profile, MP_DRAFTS_AUTODELETE) )
            {
               MDialog_Message
               (
                  _("You are starting to edit a draft message. When you\n"
                    "send it, the draft will be deleted. If you don't want\n"
                    "this to happen you may either do \"Send and keep\" and\n"
                    "\"Save as draft\" later or change the value of the\n"
                    "corresponding option in the \"Folders\" page of the\n"
                    "preferences dialog."),
                  cv->GetFrame(),
                  M_MSGBOX_DRAFT_AUTODELETE,
                  M_DLG_DISABLE
               );
            }
         }
         else if ( nameWithCase == HEADER_GEOMETRY )
         {
            // restore the composer geometry
            wxFrame *frame = cv->GetFrame();
            if ( value == GEOMETRY_ICONIZED )
            {
               frame->Iconize();
            }
            else if ( value == GEOMETRY_MAXIMIZED )
            {
               frame->Maximize();
            }
            else // not iconized, not maximized
            {
               int x, y, w, h;
               if ( sscanf(value, GEOMETRY_FORMAT, &x, &y, &w, &h) == 4 )
               {
                  frame->SetSize(x, y, w, h);
               }
               else // bad header format
               {
                  wxLogDebug(_T("Corrupted " HEADER_GEOMETRY " header '%s'."),
                             value.c_str());
               }
            }
         }
         else // just another header
         {
            cv->AddHeaderEntry(nameWithCase, value);
         }
      }
      //else: we ignore this one
   }

   // use the same language as we had used before
   cv->SetEncodingToSameAs(msg);

   msg->DecRef();

   return cv;
}

// ----------------------------------------------------------------------------
// wxComposeView ctor/dtor
// ----------------------------------------------------------------------------

wxComposeView::wxComposeView(const String &name,
                             Mode mode,
                             MessageKind kind,
                             wxWindow *parent)
             : wxMFrame(name, parent)
{
   gs_listOfAllComposers.push_back(this);

   m_mode = mode;
   m_kind = kind;
   m_pidEditor = 0;
   m_procExtEdit = NULL;
   m_alreadyExtEdited =
   m_sending =
   m_closing = false;
   m_OriginalMessage = NULL;
   m_DraftMessage = NULL;
   m_msgviewOrig = NULL;

   // by default new recipients are "to"
   m_rcptTypeLast = Recipient_To;

   m_isModified = false;

   m_editor = NULL;
   m_encoding = wxFONTENCODING_SYSTEM;
}

void wxComposeView::SetOriginal(Message *original)
{
   CHECK_RET( original, _T("no original message in composer") );

   m_OriginalMessage = original;
   m_OriginalMessage->IncRef();

   // write reply by default in the same encoding as the original message
   SetEncodingToSameAs(original);
}

void wxComposeView::SetDraft(Message *msg)
{
   if ( m_DraftMessage )
   {
      FAIL_MSG( _T("Duplicate X-M-Draft?") );

      m_DraftMessage->DecRef();
   }

   m_DraftMessage = msg;
   m_DraftMessage->IncRef();
}

wxComposeView::~wxComposeView()
{
   delete m_rcptMain;
   WX_CLEAR_ARRAY(m_rcptExtra);

   SafeDecRef(m_Profile);

   delete m_editor;

   SafeDecRef(m_OriginalMessage);
   SafeDecRef(m_DraftMessage);

   for ( ComposerList::iterator i = gs_listOfAllComposers.begin(); ; ++i )
   {
      if ( i == gs_listOfAllComposers.end() )
      {
         FAIL_MSG( _T("composer not in the list of all composers?") );

         break;
      }

      if ( *i == this )
      {
         gs_listOfAllComposers.erase(i);

         break;
      }
   }

   if ( !m_filenameAutoSave.empty() )
   {
      if ( !wxRemoveFile(m_filenameAutoSave) )
      {
         wxLogSysError(_("Failed to remove stale composer autosave file '%s'"),
                       m_filenameAutoSave.c_str());
      }
   }
}

// ----------------------------------------------------------------------------
// wxComposeView menu/tool/status bar initialization
// ----------------------------------------------------------------------------

void
wxComposeView::CreateMenu()
{
   AddFileMenu();
   AddEditMenu();
   WXADD_MENU(GetMenuBar(), COMPOSE, _("&Compose"));
   AddLanguageMenu();
   AddHelpMenu();

   // currently unused
   //
   // TODO: provide some visual feedback for them, like enabling/disabling
#if 0
   m_MItemCut = GetMenuBar()->FindItem(WXMENU_EDIT_CUT);
   m_MItemCopy = GetMenuBar()->FindItem(WXMENU_EDIT_COPY);
   m_MItemPaste = GetMenuBar()->FindItem(WXMENU_EDIT_PASTE);
#endif // 0

   // check if we can schedule messages:
   MModule *module = MModule::GetProvider(MMODULE_INTERFACE_CALENDAR);
   if ( module == NULL )
   {
      // if menu is !NULL, it will be filled with wxMenu this item belongs to
      wxMenu *menu = NULL;
      GetMenuBar()->FindItem(WXMENU_COMPOSE_SEND_LATER, &menu);
      if ( menu )
      {
         menu->Delete(WXMENU_COMPOSE_SEND_LATER);
      }
      else // unexpected
      {
         FAIL_MSG( _T("didn't find \"Send later\" in compose menu, why?") );
      }
   }
   else // we do have calendar module
   {
      module->DecRef(); // we don't need it yet
   }
}

void
wxComposeView::CreateToolAndStatusBars()
{
   CreateMToolbar(this, WXFRAME_COMPOSE);

   CreateStatusBar(2);
   static const int s_widths[] = { -1, 90 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);
}

// ----------------------------------------------------------------------------
// wxComposeView "real" creation: here we create the controls and lay them out
// ----------------------------------------------------------------------------

void wxComposeView::CreatePlaceHolder()
{
   CHECK_RET( m_sizerRcpts && m_rcptExtra.IsEmpty(),
              _T("can't or shouldn't create the place holder now!") );

   m_sizerRcpts->Add(0, 0, 1);
   m_sizerRcpts->Add(new wxStaticText
                         (
                           m_panelRecipients->GetCanvas(),
                           -1,
                           _("No more recipients")
                         ),
                     0, wxALIGN_CENTRE | wxALL, LAYOUT_MARGIN);
   m_sizerRcpts->Add(0, 0, 1);
}

void wxComposeView::DeletePlaceHolder()
{
   CHECK_RET( m_sizerRcpts && m_rcptExtra.IsEmpty(),
              _T("can't or shouldn't delete the place holder now!") );

   // remove the spacers and the static text we had added to it
   m_sizerRcpts->Remove(0); // remove first spacer by position

   wxSizerItem *item =
      (wxSizerItem *)m_sizerRcpts->GetChildren().GetFirst()->GetData();

   ASSERT_MSG( item->IsWindow(), _T("should be the static text") );

   wxWindow *win = item->GetWindow();
   m_sizerRcpts->Remove(0); // remove static text by position

   m_sizerRcpts->Remove(0); // remove second spacer by position
   delete win;
}

wxSizer *wxComposeView::CreateHeaderFields()
{
   // top level vertical (box) sizer
   wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);

   // leave number of rows unspecified, it can be calculated from number of
   // columns (2)
   wxFlexGridSizer *sizerHeaders =
      new wxFlexGridSizer(0, 2, LAYOUT_MARGIN, LAYOUT_MARGIN);

   sizerHeaders->AddGrowableCol(1);

   // add "From" header if configured to show it
   if ( READ_CONFIG(m_Profile, MP_COMPOSE_SHOW_FROM) )
   {
      sizerHeaders->Add(new wxStaticText(m_panel, -1, _("&From:")),
                        0, wxALIGN_CENTRE_VERTICAL);

      wxSizer *sizerFrom = new wxBoxSizer(wxHORIZONTAL);

      m_txtFrom = new wxTextCtrl(m_panel, -1, _T(""));
      sizerFrom->Add(m_txtFrom, 1, wxALIGN_CENTRE_VERTICAL);
      SetTextAppearance(m_txtFrom);

      wxChoice *choiceIdent = CreateIdentCombo(m_panel);
      if ( choiceIdent )
      {
         sizerFrom->Add(choiceIdent, 0,
                        wxLEFT | wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
      }
      //else: no identities configured

      sizerHeaders->Add(sizerFrom, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL);
   }
   else // no from line
   {
      m_txtFrom = NULL;
   }

   // subject
   sizerHeaders->Add(new wxStaticText(m_panel, -1, _("&Subject:")),
                     0, wxALIGN_CENTRE_VERTICAL);

   m_txtSubject = new wxSubjectTextCtrl(m_panel);
   sizerHeaders->Add(m_txtSubject, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL);
   SetTextAppearance(m_txtSubject);

   sizerTop->Add(sizerHeaders, 0, wxALL | wxEXPAND, LAYOUT_MARGIN);

   // main recipient line
   m_rcptMain = new wxRcptMainControl(this);
   wxSizer *sizerRcpt = m_rcptMain->CreateControls(m_panel);

   sizerTop->Add(sizerRcpt, 0, wxEXPAND | (wxALL & ~wxBOTTOM), LAYOUT_MARGIN/2);

   // the spare space for already entered recipients below: we use an extra
   // sizer because we keep it to add more stuff to it later
   m_panelRecipients = new wxEnhancedPanel(m_panel);

   m_sizerRcpts = new wxBoxSizer(wxVERTICAL);
   CreatePlaceHolder();

   m_panelRecipients->GetCanvas()->SetSizer(m_sizerRcpts);
   m_panelRecipients->GetCanvas()->SetAutoLayout(TRUE);

   sizerTop->Add(m_panelRecipients, 1, wxEXPAND);

   // this number is completely arbitrary
   sizerTop->SetItemMinSize(m_panelRecipients, 0, 80);

   return sizerTop;
}

void
wxComposeView::Create(wxWindow * WXUNUSED(parent), Profile *parentProfile)
{
   // first create the profile: we must have one, so find a non NULL one
   m_Profile = parentProfile ? parentProfile : mApplication->GetProfile();
   m_Profile->IncRef();

   // sometimes this profile had been created before the identity changed:
   // make sure we use the current identity for the new message composition
   m_Profile->SetIdentity(READ_APPCONFIG(MP_CURRENT_IDENTITY));

   m_options.Read(m_Profile);

   // build menu
   CreateMenu();

   // and tool/status bars
   CreateToolAndStatusBars();

   // create the child controls

   // the panel which fills all the frame client area and contains all children
   // (we use it to make the tab navigation work)
   m_splitter = new wxPSplitterWindow("ComposeSplit", this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D);

   m_panel = new wxPanel(m_splitter, -1);
   m_panel->SetAutoLayout(TRUE);

   // the sizer containing all header fields
   wxSizer *sizerHeaders = CreateHeaderFields();

   // associate this sizer with the window
   m_panel->SetSizer(sizerHeaders);
   sizerHeaders->Fit(m_panel);

   // create the composer window itself
   CreateEditor();

   // configure splitter
   wxCoord heightHeaders = m_panel->GetSize().y;
   m_splitter->SplitHorizontally(m_panel, m_editor->GetWindow(), heightHeaders);
   m_splitter->SetMinimumPaneSize(heightHeaders);

   // note that we must show and layout the frame before setting the control
   // values or the text would be scrolled to the right in the text fields as
   // they initially don't have enough space to show it...
   Show();
   Layout();

   // initialize the controls
   // -----------------------

   // set def values for all headers
   SetDefaultFrom();

   AddTo(READ_CONFIG(m_Profile, MP_COMPOSE_TO));
   AddCc(READ_CONFIG(m_Profile, MP_COMPOSE_CC));
   AddBcc(READ_CONFIG(m_Profile, MP_COMPOSE_BCC));

   if ( READ_CONFIG(m_Profile, MP_USEOUTGOINGFOLDER) )
      AddFcc(READ_CONFIG_TEXT(m_Profile, MP_OUTGOINGFOLDER));
}

/// create the compose window itself
void
wxComposeView::CreateEditor(void)
{
   wxASSERT_MSG( m_editor == NULL, _T("creating the editor twice?") );

   MModuleListing *listing =
      MModule::ListAvailableModules(MESSAGE_EDITOR_INTERFACE);

   if ( !listing  )
   {
      wxLogError(_("No message editor plugins found. You will have to "
                   "use an external editor to compose the messages."));
   }
   else // have at least one editor, load it
   {
      // TODO: make it configurable
      String name = (*listing)[0].GetName();

      MModule *editorFactory = MModule::LoadModule(name);
      if ( !editorFactory ) // failed to load the configured editor
      {
         // try any other
         String nameFirst = (*listing)[0].GetName();

         if ( name != nameFirst )
         {
            wxLogWarning(_("Failed to load the configured message editor '%s'.\n"
                           "\n"
                           "Reverting to the default message editor."),
                         name.c_str());

            editorFactory = MModule::LoadModule(nameFirst);
         }

         if ( !editorFactory )
         {
            wxLogError(_("Failed to load the default message editor '%s'.\n"
                         "\n"
                         "Builtin message editing will not work!"),
                       nameFirst.c_str());
         }
      }

      if ( editorFactory )
      {
         m_editor = ((MessageEditorFactory *)editorFactory)->Create();
         editorFactory->DecRef();
      }

      listing->DecRef();
   }

   // TODO: have a DummyEditor as we have a DummyViewer to avoid crashing
   //       if we failed to create any editor?
   m_editor->Create(this, m_splitter);
}

void
wxComposeView::InitAppearance()
{
   m_options.Read(m_Profile);
}

void wxComposeView::SetTextAppearance(wxTextCtrl *text)
{
   CHECK_RET( text, _T("NULL text in wxComposeView::SetTextAppearance") );

   if ( READ_CONFIG(m_Profile, MP_CVIEW_COLOUR_HEADERS) )
   {
      text->SetForegroundColour(m_options.m_fg);
      text->SetBackgroundColour(m_options.m_bg);
      text->SetFont(m_options.GetFont());

      wxSizer *sizer = text->GetContainingSizer();
      if ( sizer )
      {
         // we need to relayout as the text height with the new font could be
         // different
         sizer->SetItemMinSize(text, text->GetBestSize());
      }
   }
}

void wxComposeView::DoClear()
{
   m_editor->Clear();

   // set the default encoding if any
   m_editor->SetEncoding(wxFONTENCODING_DEFAULT);

   // we're not modified [any more]
   ResetDirty();
}

// ----------------------------------------------------------------------------
// wxComposeView address headers stuff
// ----------------------------------------------------------------------------

Composer::RecipientType
wxComposeView::ExpandRecipient(String *textAddress)
{
   // don't do anything for the newsgroups
   //
   // TODO-NEWS: expand using .newsrc?
   if ( m_mode == wxComposeView::Mode_News )
   {
      return Composer::Recipient_Newsgroup;
   }

   // try to expand the last component
   String& text = *textAddress;

   // trim spaces from left, but not (all) from right -- this allows "Dan " to
   // be expanded into "Dan Black" but not in "Danish Guy"
   text.Trim(FALSE);
   bool hasSpaceAtEnd = !text.empty() && text.Last() == ' ';
   text.Trim(TRUE);
   if ( !text.empty() && hasSpaceAtEnd )
      text += ' ';

   // check for the lone '"' simplifies the code for finding the starting
   // position below: it should be done here, otherwise the following loop
   // will crash!
   if ( text.empty() || text == '"' )
   {
      // don't do anything
      wxLogStatus(GetFrame(),
                  _("Nothing to expand - please enter something."));

      return Composer::Recipient_None;
   }

   // find the starting position of the last address in the address list
   size_t nLastAddr;
   bool quoted = text.Last() == '"';
   if ( quoted )
   {
      // just find the matching quote (not escaped)
      const char *pStart = text.c_str();
      const char *p;
      for ( p = pStart + text.length() - 2; p >= pStart; p-- )
      {
         if ( *p == '"' )
         {
            // check that it's not escaped
            if ( (p == pStart) || (*(p - 1) != '\\') )
            {
               // ok, found it!
               break;
            }
         }
      }

      nLastAddr = p - pStart;
   }
   else // unquoted
   {
      // search back until the last address separator
      for ( nLastAddr = text.length() - 1; nLastAddr > 0; nLastAddr-- )
      {
         char c = text[nLastAddr];
         if ( (c == ',') || (c == ';') )
            break;
      }

      if ( nLastAddr > 0 )
      {
         // move beyond the ',' or ';' which stopped the scan
         nLastAddr++;
      }

      // the address will probably never start with spaces but if it does, it
      // will be enough to just take it into quotes
      while ( isspace(text[nLastAddr]) )
      {
         nLastAddr++;
      }
   }

   // so now we've got the text we'll be trying to expand
   String textOrig = text.c_str() + nLastAddr;

   // do we have an explicit address type specifier
   Composer::RecipientType addrType = Composer::Recipient_Max;
   if ( textOrig.length() > 3 )
   {
      // check for to:, cc: or bcc: prefix
      if ( textOrig[2u] == ':' )
      {
         if ( toupper(textOrig[0u]) == 'T' && toupper(textOrig[1u]) == 'O' )
            addrType = Composer::Recipient_To;
         else if ( toupper(textOrig[0u]) == 'C' && toupper(textOrig[1u]) == 'C' )
            addrType = Composer::Recipient_Cc;
      }
      else if ( textOrig[3u] == ':' && textOrig(0, 3).Upper() == "BCC" )
      {
         addrType = Composer::Recipient_Bcc;
      }

      if ( addrType != Composer::Recipient_Max )
      {
         // erase the colon as well
         textOrig.erase(0, addrType == Composer::Recipient_Bcc ? 4 : 3);
      }
   }

   // remove "mailto:" prefix if it's there - this is convenient when you paste
   // in an URL from the web browser
   //
   // TODO: add support for the mailto URL parameters, i.e. should support
   //       things like "mailto:foo@bar.com?subject=Please%20help"
   String newText;
   if ( !textOrig.StartsWith("mailto:", &newText) )
   {
      // if the text already has a '@' inside it and looks like a full email
      // address assume that it doesn't need to be expanded (this saves a lot
      // of time as expanding a non existing address looks through all address
      // books...)
      size_t pos = textOrig.find('@');
      if ( pos != String::npos && pos > 0 && pos < textOrig.length() )
      {
         // also check that the host part of the address is expanded - it
         // should contain at least one dot normally
         if ( strchr(textOrig.c_str() + pos + 1, '.') )
         {
            // looks like a valid address - nothing to do
            newText = textOrig;
         }
      }

      if ( newText.empty() )
      {
         wxArrayString expansions;

         if ( !AdbExpand(expansions,
                         textOrig,
                         READ_CONFIG(GetProfile(), MP_ADB_SUBSTRINGEXPANSION)
                           ? AdbLookup_Substring
                           : AdbLookup_StartsWith,
                         this) )
         {
            // cancelled, don't do anything
            return Composer::Recipient_None;
         }

         // construct the replacement string(s)
         size_t nExpCount = expansions.GetCount();
         for ( size_t nExp = 0; nExp < nExpCount; nExp++ )
         {
            if ( nExp > 0 )
               newText += CANONIC_ADDRESS_SEPARATOR;

            newText += expansions[nExp];
         }
      }
   }

   // find the end of the previous address
   size_t nPrevAddrEnd;
   if ( nLastAddr > 0 )
   {
      // undo "++" above
      nLastAddr--;
   }

   for ( nPrevAddrEnd = nLastAddr; nPrevAddrEnd > 0; nPrevAddrEnd-- )
   {
      char c = text[nPrevAddrEnd];
      if ( !isspace(c) && (c != ',') && (c != ';') )
      {
         // this character is a part of previous string, leave it there
         nPrevAddrEnd++;

         break;
      }
   }

   // keep the text up to the address we expanded/processed
   wxString oldText(text, nPrevAddrEnd);  // first nPrevAddrEnd chars
   if ( !oldText.empty() )
   {
      // there was something before, add separator
      oldText += CANONIC_ADDRESS_SEPARATOR;
   }

   text = oldText + newText;

   return addrType;
}

void
wxComposeView::AddRecipientControls(const String& value, RecipientType rt)
{
   // remove the place holder we had there before
   if ( m_rcptExtra.IsEmpty() )
   {
      DeletePlaceHolder();
   }

   // create the controls and add them to the sizer
   wxRcptExtraControl *rcpt = new wxRcptExtraControl(this);

   wxSizer *sizerRcpt = rcpt->CreateControls(m_panelRecipients->GetCanvas());

   rcpt->InitControls(value, rt);

   // insert the control in the beginning, like this the controls inserted
   // later stay visible even if the controls added earlier might be scrolled
   // off
   m_rcptExtra.Insert(rcpt, 0);

   m_sizerRcpts->Prepend(sizerRcpt, 0, wxALL | wxEXPAND, LAYOUT_MARGIN / 2);
   m_sizerRcpts->Layout();
   m_panelRecipients->RefreshScrollbar(m_panelRecipients->GetClientSize());

   // adjust the indexes of all the existing controls
   const size_t count = m_rcptExtra.GetCount();
   for ( size_t n = 1; n < count; n++ )
   {
      m_rcptExtra[n]->IncIndex();
   }
}

void
wxComposeView::OnRemoveRcpt(size_t index)
{
   // we can just remove the sizer by position because AddRecipientControls()
   // above adds one sizer for each new recipient - if it changes, this code
   // must change too!
   m_sizerRcpts->Remove(index);

   // and delete the controls
   delete m_rcptExtra[index];

   // remove them from the arrays too
   m_rcptExtra.RemoveAt(index);

   // and don't forget to adjust the indices of all the others
   size_t count = m_rcptExtra.GetCount();
   while ( index < count )
   {
      m_rcptExtra[index++]->DecIndex();
   }

   if ( count == 0 )
   {
      CreatePlaceHolder();
   }

   m_sizerRcpts->Layout();
   m_panelRecipients->RefreshScrollbar(m_panelRecipients->GetClientSize());
}

void
wxComposeView::AddRecipients(const String& address, RecipientType addrType)
{
   // if there is no address, nothing to do
   if ( address.empty() )
      return;

   // invalid rcpt type means to reuse the last one
   if ( addrType == Recipient_Max )
   {
      addrType = m_rcptTypeLast;
   }

   switch ( addrType )
   {
      case Recipient_Newsgroup:
         {
            // tokenize the string possibly containing several newsgroups
            const wxArrayString
               groups = wxStringTokenize(address, ",; \t", wxTOKEN_STRTOK);

            size_t count = groups.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
               AddRecipient(groups[n], Recipient_Newsgroup);
            }
         }
         break;

         // note that we may have recipients of Recipient_None type: this
         // happens when we're replying to a sender of a message with many
         // recipients  as we still add all of them to the new composer but
         // disabled initially
      case Recipient_None:
      case Recipient_To:
      case Recipient_Cc:
      case Recipient_Bcc:
         // an email address
         {
            // split the string in addreses and add all of them
            AddressList_obj addrList(address,
                                     READ_CONFIG(m_Profile, MP_HOSTNAME));
            if ( addrList )
            {
               Address *addr = addrList->GetFirst();
               while ( addr )
               {
                  String address = addr->GetAddress();
                  if ( !address.empty() )
                  {
                     AddRecipient(address, addrType);
                  }

                  addr = addrList->GetNext(addr);
               }
            }
         }
         break;

      case Recipient_Fcc:
         // hack alert: there is a magic value here which, instead of adding a
         // new folder to Fcc, removes Fcc entirely. This is very clumsy but
         // too useful not to have it because like this it may be used in the
         // (especially forward) templates to avoid saving messages in SentMail
         // folder automatically

         if ( address == _T("none") )
         {
            size_t count = m_rcptExtra.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
               wxRcptControl * const rcpt = m_rcptExtra[n];
               if ( rcpt->GetType() == Recipient_Fcc )
               {
                  rcpt->SetType(Recipient_None);
               }
            }
         }
         else
         {
            // add another folder
            AddRecipient(address, Recipient_Fcc);
         }
         break;

      case Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected wxRcptControl type") );
   }
}

void
wxComposeView::AddRecipient(const String& addr, RecipientType addrType)
{
   // this is a private function, AddRecipients() above is the public one and it
   // does the parameter validation
   CHECK_RET( !addr.empty() && addrType != Recipient_Max,
              _T("invalid parameter in AddRecipient()") );

   // look if we don't already have it
   size_t count = m_rcptExtra.GetCount();

   for ( size_t n = 0; n < count; n++ )
   {
      if ( Message::CompareAddresses(m_rcptExtra[n]->GetValue(), addr) )
      {
         // ok, we already have this address - is it of the same type?
         RecipientType existingType = m_rcptExtra[n]->GetType();
         if ( existingType == addrType )
         {
            // yes, don't add it again
            wxLogStatus(this,
                        _("Address '%s' is already in the recipients list, "
                          "not added."),
                        addr.c_str());
         }
         else // found with a different type
         {
            // use the "stronger" recipient type, i.e. if the address was
            // previously in "Cc" but is now added as "To", make the existing
            // address "To"
            if ( existingType > addrType )
            {
               wxLogStatus(this,
                           _("Address '%s' was already in the recipients list "
                             "with a different type, just changed the type."),
                           addr.c_str());

               m_rcptExtra[n]->SetType(addrType);
            }
            else
            {
               wxLogStatus(this,
                           _("Address '%s' was already in the recipients list "
                             "with a different type, not added."),
                           addr.c_str());
            }
         }

         // don't add it again
         return;
      }
   }

   // do add it if not found
   AddRecipientControls(addr, addrType);
}

bool wxComposeView::IsRecipientEnabled(size_t index) const
{
   return m_rcptExtra[index]->IsEnabled();
}

// helper of GetRecipients()
static void
GetRecipientFromControl(wxComposeView::RecipientType type,
                        wxRcptControl *rcpt,
                        wxString& address)
{
   if ( rcpt->GetType() == type )
   {
      if ( !address.empty() )
         address += CANONIC_ADDRESS_SEPARATOR;

      address += rcpt->GetValue();
   }
}

String wxComposeView::GetRecipients(RecipientType type) const
{
   String address;

   GetRecipientFromControl(type, m_rcptMain, address);

   size_t count = m_rcptExtra.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      GetRecipientFromControl(type, m_rcptExtra[n], address);
   }

   return address;
}

String wxComposeView::GetFrom() const
{
   String from;

   // be careful here: m_txtFrom might be not shown
   if ( m_txtFrom )
   {
      from = m_txtFrom->GetValue();
   }
   else
   {
      from = m_from;
   }

   return from;
}

String wxComposeView::GetSubject() const
{
   return m_txtSubject->GetValue();
}

// ----------------------------------------------------------------------------
// filling wxComposeView with text initially: takes care of stuff like
// signatures, vcards and, more generally, compose templates
// ----------------------------------------------------------------------------

void
wxComposeView::InitText(Message *msg, const MessageView *msgview)
{
   // store it for the future use in DoInitText()
   m_msgviewOrig = msgview;

   if ( m_kind == Message_New )
   {
      // writing a new message/article: wait until the headers are filled
      // before evacuating the template
      if ( !m_template )
      {
         m_template = GetMessageTemplate(m_mode == Mode_Mail
                                          ? MessageTemplate_NewMessage
                                          : MessageTemplate_NewArticle,
                                         m_Profile);
      }

      // if we can evaluate the template right now, do it
      if ( !TemplateNeedsHeaders(m_template) )
      {
         DoInitText();
      }
      else // we have a template involving the header values
      {
         // but we can't evaluate it yet because the headers values are
         // unknown, delay the evaluation
         InsertText(_("--- Please choose evaluate template from ---\n"
                      "--- the menu after filling the headers or ---\n"
                      "--- just type something in the composer ---"));
         ResetDirty();
      }
   }
   else // replying or forwarding - evaluate the template right now
   {
      ASSERT_MSG( m_OriginalMessage, _T("what do we reply to?") );

      DoInitText(msg);
   }
}

void wxComposeView::Launch()
{
   // we also use this method to initialize the focus as we can't do it before
   // the composer text is inited

   // if the subject is already not empty (which it is when
   // replying/forwarding), put the cursor directly into the compose window,
   // otherwise let the user enter the subject first
   switch ( m_kind )
   {
      default:
         FAIL_MSG( _T("unknown message kind") );
         // fall through

      case Message_New:
         m_txtSubject->SetFocus();
         break;

      case Message_Reply:
         SetFocusToComposer();
         break;

      case Message_Forward:
         m_rcptMain->GetText()->SetFocus();
         break;
   }
}

void
wxComposeView::DoInitText(Message *msgOrig)
{
   // position the cursor correctly and separate us from the previous message
   // if we're replying to several messages at once
   if ( !IsEmpty() )
   {
      InsertText("\n");
   }

   // deal with templates: first decide what kind of template do we need
   MessageTemplateKind kind;
   switch ( m_kind )
   {
      case Message_Reply:
         kind = m_mode == Mode_Mail ? MessageTemplate_Reply
                                    : MessageTemplate_Followup;
         break;

      case Message_Forward:
         ASSERT_MSG( m_mode == Mode_Mail, _T("can't forward article in news") );

         kind = MessageTemplate_Forward;
         break;

      default:
         FAIL_MSG(_T("unknown message kind"));
         // still fall through

      case Message_New:
         kind = m_mode == Mode_Mail ? MessageTemplate_NewMessage
                                    : MessageTemplate_NewArticle;
         break;
   }

   // loop until the template is correct or the user abandons the idea to fix
   // it
   bool templateChanged;
   do
   {
      // get the template value
      String templateValue = !m_template ? GetMessageTemplate(kind, m_Profile)
                                         : m_template;

      if ( !templateValue )
      {
         // if there is no template just don't do anything
         break;
      }

      if ( m_msgviewOrig == MailFolder::Params::NO_QUOTE )
      {
         // we don't want to quote anything at all, so remove all occurences of
         // $QUOTE and/or $TEXT templates in the string -- and also remove
         // anything preceding them assuming that it can only be the
         // attribution line which shouldn't be left if we don't quote anything
         //
         // this is surely not ideal, but I don't see how to do what we want
         // otherwise with the existing code

         const char *pcEnd = NULL;
         const char *pcStart = templateValue.c_str();
         for ( const char *pc = pcStart; ; )
         {
            // find and skip over the next macro occurence
            pc = strchr(pc, '$');
            if ( !pc++ )
               break;

            static const char *QUOTE = "quote";
            static const size_t LEN_QUOTE = strlen(QUOTE);

            static const char *TEXT = "text";
            static const size_t LEN_TEXT = strlen(TEXT);

            if ( wxStrnicmp(pc, QUOTE, LEN_QUOTE) == 0 )
            {
               pc += LEN_QUOTE;
               pcEnd = pc;
            }
            else if ( wxStrnicmp(pc, TEXT, LEN_TEXT) == 0 )
            {
               pc += LEN_TEXT;
               pcEnd = pc;
            }
         }

         if ( pcEnd )
         {
            templateValue.erase(0, pcEnd - pcStart);
         }
      }

      // we will only run this loop once unless there are erros in the template
      // and the user changed it
      templateChanged = FALSE;

      // do parse the template
      if ( !ExpandTemplate
            (
               *this,
               m_Profile,
               templateValue,
               msgOrig ? msgOrig : m_OriginalMessage,
               m_msgviewOrig
            ) )
      {
         // first show any errors which the call to Parse() above could
         // generate
         wxLog::FlushActive();

         if ( MDialog_YesNoDialog(_("There were errors in the template. Would "
                                    "you like to edit it now?"),
                                    this,
                                    MDIALOG_YESNOTITLE,
                                    M_DLG_YES_DEFAULT,
                                    M_MSGBOX_FIX_TEMPLATE) )
         {
            // depending on whether we had the template from the beginning or
            // whether we use the standard one, we want to propose different
            // fixes

            if ( !m_template )
            {
               // invoke the template configuration dialog and if something
               // changed...
               if ( ConfigureTemplates(m_Profile, this) )
               {
                  // ...restart the loop
                  templateChanged = TRUE;
               }
            }
            else
            {
               // invoke the dialog for all templates
               String templNew = ChooseTemplateFor(kind, this);
               if ( m_template != templNew )
               {
                  templateChanged = TRUE;

                  m_template = templNew;
               }
            }
         }
      }
   } while ( templateChanged );

   // NB: the signature is now inserted by the template parser

   // finally, attach a vCard if requested
   //
   // TODO: should this be handled by the template as well?
   if ( READ_CONFIG(m_Profile, MP_USEVCARD) )
   {
      // read the vCard from the file specified in config
      wxString vcard,
               filename = READ_CONFIG(m_Profile, MP_VCARD);

      bool hasCard = false;
      while ( !hasCard )
      {
         wxString msg;
         if ( !filename )
         {
            msg = _("You haven't configured your vCard file.");
         }
         else // have file name
         {
            wxFFile file(filename);
            if ( file.IsOpened() && file.ReadAll(&vcard) )
            {
               hasCard = true;
            }
            else
            {
               wxLog::FlushActive();

               msg.Printf(_("Couldn't read vCard file '%s'."),
                          filename.c_str());
            }
         }

         if ( !hasCard )
         {
            msg += _("\n\nWould you like to choose your vCard now?");
            if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                                     M_DLG_YES_DEFAULT,
                                     M_MSGBOX_ASK_VCARD) )
            {
               wxString pattern;
               pattern << _("vCard files (*.vcf)|*.vcf") << '|' << wxALL_FILES;
               filename = wxPFileSelector("vcard",
                                          _("Choose vCard file"),
                                          NULL, "vcard.vcf", NULL,
                                          pattern,
                                          0, this);
            }
            else
            {
               // user doesn't want to use signature file
               break;
            }

            if ( !filename )
            {
               // the file selection dialog was cancelled
               break;
            }
         }
      }

      // now do attach it
      if ( hasCard )
      {
         InsertData(strdup(vcard), vcard.length(), "text/x-vcard", filename);
      }
   }

   // the text hasn't been modified by the user
   ResetDirty();
}

void wxComposeView::OnFirstTimeModify()
{
   // don't clear the text below if we already have something there -- but
   // normally we shouldn't
   CHECK_RET( !IsModified(),
              _T("shouldn't be called if we had been already changed!") );

   // evaluate the template right now if we have one and it hadn't been done
   // yet (for messages of kind other than new it is done on creation)
   if ( m_kind == Message_New && !m_template.empty() )
   {
      DoClear();

      DoInitText();
   }
}

// ----------------------------------------------------------------------------
// wxComposeView parameters setting
// ----------------------------------------------------------------------------

void wxComposeView::SetEncoding(wxFontEncoding encoding)
{
   if ( encoding == wxFONTENCODING_DEFAULT )
   {
      encoding = (wxFontEncoding)(long)
                  READ_CONFIG(m_Profile, MP_MSGVIEW_DEFAULT_ENCODING);

      // no default encoding specified by user, use the system default one
      if ( encoding == wxFONTENCODING_DEFAULT )
      {
         encoding = wxFONTENCODING_SYSTEM;
      }
   }

   m_encoding = encoding;
   m_editor->SetEncoding(encoding);

   // check "Default" menu item if we use the system default encoding in absence
   // of any user-configured default
   CheckLanguageInMenu(this, m_encoding == wxFONTENCODING_SYSTEM
                                             ? wxFONTENCODING_DEFAULT
                                             : m_encoding);
}

void wxComposeView::SetEncodingToSameAs(Message *msg)
{
   CHECK_RET( msg, _T("no message in SetEncodingToSameAs") );

   // find the first text part with non default encoding
   int count = msg->CountParts();
   for ( int part = 0; part < count; part++ )
   {
      if ( msg->GetPartType(part) == MimeType::TEXT )
      {
         wxFontEncoding enc = msg->GetTextPartEncoding(part);
         if ( enc != wxFONTENCODING_SYSTEM )
         {
            SetEncoding(enc);
            break;
         }
      }
   }
}

void wxComposeView::EnableEditing(bool enable)
{
   // indicate the current state in the status bar
   SetStatusText(enable ? "" : _("RO"), 1);

   m_editor->Enable(enable);
}

// ----------------------------------------------------------------------------
// wxComposeView callbacks
// ----------------------------------------------------------------------------

// use the newly specified identity
void
wxComposeView::OnIdentChange(wxCommandEvent& event)
{
   // get the new identity
   const wxString ident = event.GetString();

   // is it the same as the current one?
   if ( ident != m_Profile->GetIdentity() )
   {
      if ( event.GetInt() == 0 )
      {
         // first one is always the default identity
         m_Profile->ClearIdentity();
      }
      else // new identity chosen
      {
         m_Profile->SetIdentity(ident);
      }

      SetDefaultFrom();

      // the signature could have changed, reinitalize the composer if it
      // hadn't been modified by the user yet
      if ( !IsModified() )
      {
         DoClear();

         DoInitText();
      }
   }
}

void
wxComposeView::OnRcptTypeChange(RecipientType type)
{
   // remember it as the last type and reuse it for the next recipient
   m_rcptTypeLast = type;
}

// can we close the window now? check the modified flag
bool
wxComposeView::CanClose() const
{
   bool canClose = true;

   // if the msg is not empty, we're going to show a msg box to the user
   String msg;
   const MPersMsgBox *persMsgBox = NULL;

   // we can't close while the external editor is running (I think it will
   // lead to a nice crash later)
   if ( m_procExtEdit )
   {
      wxLogError(_("Please terminate the external editor (PID %d) before "
                   "closing this window."), m_pidEditor);

      canClose = false;
   }
   else if ( IsModified() )
   {
      // the text of the new message has been modified, ask the user before
      // closing -- this message box can't be suppressed
      msg = _("There are unsaved changes, would you like to save the "
              "message as a draft?");
   }
   else if ( m_txtSubject->IsModified() ||
               (m_txtFrom && m_txtFrom->IsModified()) ||
                  m_rcptMain->GetText()->IsModified() )
   {
      // the headers of the new message have been modified -- ask the user if
      // he wants to save but let him disable this msg box
      msg = _("You have modified some of the headers of this message,\n"
              "would you like to save the message as a draft?");

      persMsgBox = M_MSGBOX_ASK_SAVE_HEADERS;
   }
   else // nothing changed at all
   {
      canClose = true;
   }

   if ( !msg.empty() )
   {
      // bring the frame to the top to show user which window are we asking him
      // about
      wxComposeView *self = (wxComposeView *)this;
      self->GetFrame()->Raise();

      // ask the user if he wants to save the changes
      msg << _T("\n\n")
          << _("You may also choose \"Cancel\" to not close this "
               "window at all.");

      MDlgResult rc = MDialog_YesNoCancel
                      (
                        msg,
                        this,                // parent
                        MDIALOG_YESNOTITLE,
                        M_DLG_YES_DEFAULT,
                        persMsgBox
                      );

      switch ( rc )
      {
         case MDlg_No:
            canClose = true;
            break;

         case MDlg_Yes:
            if ( SaveAsDraft() )
               break;
            //else: fall through and don't close the window

         case MDlg_Cancel:
            canClose = false;
      }
   }

   if ( canClose )
   {
      ((wxComposeView*)this)->m_closing = true; // const_cast
   }
   return canClose;
}

// process all our menu commands
void
wxComposeView::OnMenuCommand(int id)
{
   switch(id)
   {
      case WXMENU_COMPOSE_INSERTFILE:
         {
            wxArrayString filenames;
            size_t nFiles = wxPFilesSelector
                            (
                             filenames,
                             "MsgInsert",
                             _("Please choose files to insert."),
                             NULL, "dead.letter", NULL,
                             _(wxALL_FILES),
                             wxOPEN | wxHIDE_READONLY | wxFILE_MUST_EXIST,
                             this
                            );
            for ( size_t n = 0; n < nFiles; n++ )
            {
               InsertFile(filenames[n]);
            }
         }
         break;


      case WXMENU_COMPOSE_SEND:
      case WXMENU_COMPOSE_SEND_KEEP_OPEN:
      case WXMENU_COMPOSE_SEND_LATER:
         if ( IsReadyToSend() )
            if ( Send( (id == WXMENU_COMPOSE_SEND_LATER) ) )
            {
               if ( id != WXMENU_COMPOSE_SEND_KEEP_OPEN )
               {
                  Close();
               }
            }
         break;

      case WXMENU_COMPOSE_SAVE_AS_DRAFT:
         if ( SaveAsDraft() )
         {
            Close();
         }
         break;

      case WXMENU_COMPOSE_PREVIEW:
         {
            SendMessage *msg = BuildMessage();
            if ( !msg )
            {
               wxLogError(_("Failed to create the message to preview."));
            }
            else
            {
               msg->Preview();

               delete msg;
            }
         }
         break;

      case WXMENU_COMPOSE_PRINT:
         Print();
         break;

      case WXMENU_COMPOSE_CLEAR:
         DoClear();
         break;

      case WXMENU_COMPOSE_EVAL_TEMPLATE:
         if ( !IsModified() )
         {
            // remove our prompt
            DoClear();
         }

         DoInitText();
         break;

      case WXMENU_COMPOSE_LOADTEXT:
         {
            String filename = wxPFileSelector
                              (
                               "MsgInsertText",
                               _("Please choose a file to insert."),
                               NULL, "dead.letter", NULL,
                               _(wxALL_FILES),
                               wxOPEN | wxHIDE_READONLY | wxFILE_MUST_EXIST,
                               this
                              );

            if ( filename.empty() )
            {
               wxLogStatus(this, _("Cancelled"));
            }
            else if ( InsertFileAsText(filename, MessageEditor::Insert_Append) )
            {
               wxLogStatus(this, _("Inserted file '%s'."), filename.c_str());
            }
            else
            {
               wxLogError(_("Failed to insert the text file '%s'."),
                          filename.c_str());
            }
         }
         break;

      case WXMENU_COMPOSE_SAVETEXT:
         {
            String filename = wxPFileSelector
                              (
                               "MsgSaveText",
                               _("Choose file to append message to"),
                               NULL, "dead.letter", NULL,
                               _(wxALL_FILES),
                               wxSAVE,
                               this
                              );

            if ( filename.empty() )
            {
               wxLogStatus(this, _("Cancelled"));
            }
            else if ( SaveMsgTextToFile(filename) )
            {
               // we have been saved
               ResetDirty();

               wxLogStatus(this, _("Message text saved to file '%s'."),
                           filename.c_str());
            }
            else
            {
               wxLogError(_("Failed to save the message."));
            }
         }
         break;

      case WXMENU_COMPOSE_EXTEDIT:
         StartExternalEditor();
         break;

      case WXMENU_COMPOSE_CUSTOM_HEADERS:
         {
            String headerName, headerValue;
            bool storedInProfile;
            if ( ConfigureCustomHeader(m_Profile, this,
                                       &headerName, &headerValue,
                                       &storedInProfile,
                                       m_mode == Mode_News ? CustomHeader_News
                                                           : CustomHeader_Mail) )
            {
               if ( !storedInProfile )
               {
                  AddHeaderEntry(headerName, headerValue);
               }
               //else: we will take it from the profile when we will send the msg

               wxLogStatus(this, _("Added custom header '%s' to the message."),
                           headerName.c_str());
            }
            //else: cancelled
         }
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(
            (m_mode == Mode_Mail)?
            MH_COMPOSE_MAIL : MH_COMPOSE_NEWS, this);
         break;

      case WXMENU_EDIT_PASTE:
         m_editor->Paste();
         break;

      case WXMENU_EDIT_COPY:
         m_editor->Copy();
         break;

      case WXMENU_EDIT_CUT:
         m_editor->Cut();
         break;

      default:
         if ( WXMENU_CONTAINS(LANG, id) && id != WXMENU_LANG_SET_DEFAULT )
         {
            SetEncoding(GetEncodingFromMenuCommand(id));
         }
         else
         {
            wxMFrame::OnMenuCommand(id);
         }
   }
}

// ----------------------------------------------------------------------------
// external editor support
// ----------------------------------------------------------------------------

bool wxComposeView::OnFirstTimeFocus()
{
   // if we had already edited the text, don't launch the editor
   if ( m_alreadyExtEdited )
      return true;

   // it may happen that the message is sent before the composer gets focus,
   // avoid starting the external editor by this time!
   if ( m_sending )
      return true;

   // and it may also happen that the user asked for closing the composer when
   // it gets focus
   if ( m_closing )
      return true;

   // now we can launch the ext editor if configured to do it
   if ( READ_CONFIG(m_Profile, MP_ALWAYS_USE_EXTERNALEDITOR) )
   {
      // init the composer text if not done yet
      if ( !IsModified() )
      {
         OnFirstTimeModify();
      }

      if ( !StartExternalEditor() )
      {
         // disable it for the next time: it would be too annoying to
         // automatically fail to start the editor every time
         m_Profile->writeEntry(MP_ALWAYS_USE_EXTERNALEDITOR, 0l);
      }

      // don't call OnFirstTimeModify() as we already did
      return true;
   }

   return false;
}

unsigned long wxComposeView::ComputeTextHash() const
{
   return m_editor->ComputeHash();
}

bool wxComposeView::StartExternalEditor()
{
   if ( m_procExtEdit )
   {
      wxLogError(_("External editor is already running (PID %d)"),
                 m_pidEditor);

      // ext editor is running
      return true;
   }

   // even if we fail to launch it, don't even attempt to do it when we first
   // get the focus (see OnFirstTimeFocus())
   m_alreadyExtEdited = true;

   bool launchedOk = false;

   // if the editor can't be started we ask the user if he wantes to
   // reconfigure it and in this case we retry with the new setting
   bool tryAgain = true;
   while ( !m_procExtEdit && tryAgain )
   {
      tryAgain = false;

      // this entry is supposed to contain '%s' - not to be expanded
      ProfileEnvVarSave envVarDisable(mApplication->GetProfile());

      String extEdit = READ_APPCONFIG(MP_EXTERNALEDITOR);
      if ( !extEdit )
      {
         wxLogStatus(this, _("External editor not configured."));
      }
      else
      {
         // first write the text we already have into a temp file
         MTempFileName tmpFileName;
         if ( !tmpFileName.IsOk() )
         {
            wxLogSysError(_("Cannot create temporary file"));

            break;
         }

         if ( !SaveMsgTextToFile(tmpFileName.GetName()) )
         {
            wxLogError(_("Failed to pass message to external editor."));

            break;
         }

         // save it to be able to check if the text was externally modified
         // later
         m_oldTextHash = ComputeTextHash();

         // we have a handy function in wxFileType which will replace
         // '%s' with the file name or add the file name at the end if
         // there is no '%s'
         wxFileType::MessageParameters params(tmpFileName.GetName(), "");
         String command = wxFileType::ExpandCommand(extEdit, params);

         // do start the external process
         m_procExtEdit = new wxProcess(this, HelperProcess_Editor);
         m_pidEditor = wxExecute(command, FALSE, m_procExtEdit);

         if ( !m_pidEditor  )
         {
            wxLogError(_("Execution of '%s' failed."), command.c_str());
         }
         else // editor launched
         {
            tmpFileName.Ok();
            m_tmpFileName = tmpFileName.GetName();

            wxLogStatus(this, _("Started external editor (PID %d)"),
                        m_pidEditor);

            // disable editing with the internal editor to avoid
            // interference with the external one
            EnableEditing(false);

            // and reflect it in the title
            wxFrame *frame = GetFrame();
            if ( frame )
            {
               wxString title;
               title << COMPOSER_TITLE
                     << _(" (frozen: external editor running)");

               frame->SetTitle(title);
            }
            else
            {
               FAIL_MSG( _T("composer without frame?") );
            }

            launchedOk = true;
         }
      }

      if ( !m_pidEditor  )
      {
         if ( m_procExtEdit )
         {
            delete m_procExtEdit;
            m_procExtEdit = NULL;
         }

         // either it wasn't configured at all, or the configured editor
         // couldn't be started - propose to change it
         String msg = _("Would you like to change the external "
                        "editor setting now?");
         if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                                  M_DLG_YES_DEFAULT,
                                  M_MSGBOX_ASK_FOR_EXT_EDIT) )
         {
            if ( MInputBox(&extEdit,
                           _("Set up external editor"),
                           _("Enter the command name (%s will be "
                             "replaced with the name of the file):"),
                           this,
                           NULL,
                           extEdit) )
            {
               // the ext editor setting is global, don't write it in
               // our profile
               mApplication->GetProfile()->writeEntry(MP_EXTERNALEDITOR,
                                                      extEdit);

               tryAgain = true;
            }
            //else: the dialog was cancelled, don't retry
         }
         //else: user doesn't want to set up ext editor, don't retry
      }
   }

   return launchedOk;
}

void wxComposeView::OnExtEditorTerm(wxProcessEvent& event)
{
   CHECK_RET( event.GetPid() == m_pidEditor , _T("unknown program terminated") );

   // reenable editing the text in the built-in editor
   EnableEditing(true);

   wxFrame *frame = GetFrame();
   if ( frame )
   {
      frame->SetTitle(COMPOSER_TITLE);
   }
   else
   {
      FAIL_MSG( _T("composer without frame?") );
   }

   bool ok = false;

   // check the return code of the editor process
   int exitcode = event.GetExitCode();
   if ( exitcode != 0 )
   {
      wxLogError(_("External editor terminated with non null exit code %d."),
                 exitcode);
   }
   else // exit code ok
   {
      bool wasModified = m_editor->IsModified();

      if ( !InsertFileAsText(m_tmpFileName, MessageEditor::Insert_Replace) )
      {
         wxLogError(_("Failed to insert back the text from external editor."));
      }
      else // InsertFileAsText() succeeded
      {
         if ( remove(m_tmpFileName) != 0 )
         {
            wxLogDebug(_T("Stale temp file '%s' left."), m_tmpFileName.c_str());
         }

         ok = true;

         // check if the text was really changed
         if ( ComputeTextHash() == m_oldTextHash )
         {
            // assume it wasn't and so restore the previous "modified" flag (if
            // it had been modified before the ext editor was launched, it is
            // still dirty, of course, but if it wasn't, it didn't become so)
            if ( !wasModified )
            {
               m_editor->ResetDirty();
            }

            // show it to the user so that he knows that there will be no
            // question before closing the window
            wxLogStatus(this, _("External editor terminated, text unchanged."));
         }
         else // text was changed
         {
            wxLogStatus(this, _("Inserted text from external editor."));
         }
      }
   }

   if ( !ok )
   {
      wxLogError(_("The text was left in the file '%s'."),
                 m_tmpFileName.c_str());
   }

   m_pidEditor = 0;
   m_tmpFileName.Empty();

   delete m_procExtEdit;
   m_procExtEdit = NULL;

   // show the frame which might had been obscured by the other windows
   Raise();
}

// ----------------------------------------------------------------------------
// inserting stuff into wxComposeView
// ----------------------------------------------------------------------------

// common part of InsertData() and InsertFile()
void
wxComposeView::DoInsertAttachment(EditorContentPart *mc, const char *mimetype)
{
   mc->SetMimeType(mimetype);

   String ext;
   wxSplitPath(mc->GetFileName(), NULL, NULL, &ext);

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxIcon icon = iconManager->GetIconFromMimeType(mimetype, ext);

   m_editor->InsertAttachment(icon, mc);
}

// insert any data
void
wxComposeView::InsertData(void *data,
                          size_t length,
                          const char *mimetype,
                          const char *name, 
                          const char *filename)
{
   String mt = mimetype;
   if ( mt.empty() )
   {
      if ( !strutil_isempty(filename) )
      {
         mt = GetMimeTypeFromFilename(filename);
      }
      else // default
      {
         mt = "APPLICATION/OCTET-STREAM";
      }
   }

   EditorContentPart *mc = new EditorContentPart();
   mc->SetData(data, length, name, filename);

   DoInsertAttachment(mc, mimetype);
}

// insert file data
void
wxComposeView::InsertFile(const char *fileName, const char *mimetype)
{
   CHECK_RET( !strutil_isempty(fileName), _T("filename can't be empty") );

   // if there is a slash after the dot, it is not extension (otherwise it
   // might be not an extension too, but consider that it is - how can we
   // decide otherwise?)
   String filename(fileName);
   String strExt = filename.AfterLast('.');
   if ( strExt == filename || strchr(strExt, '/') )
      strExt.Empty();

   String strMimeType;
   if ( strutil_isempty(mimetype) )
   {
      strMimeType = GetMimeTypeFromFilename(filename);
   }
   else
   {
      strMimeType = mimetype;
   }

   // create the new attachment
   EditorContentPart *mc = new EditorContentPart();

   AttachmentProperties props;
   props.filename = filename;
   props.name = wxFileNameFromPath(filename);
   props.mimetype = strMimeType;
   mc->SetFile(props.filename);

   // by default propose to send the images and text parts inline but all the
   // rest as attachment
   const MimeType::Primary primaryType = MimeType(strMimeType).GetPrimary();
   props.disposition = primaryType == MimeType::TEXT ||
                        primaryType == MimeType::IMAGE
                        ? AttachmentProperties::Disposition_Inline
                        : AttachmentProperties::Disposition_Attachment;

   // show the attachment properties dialog automatically?
   String configPath = GetPersMsgBoxName(M_MSGBOX_MIME_TYPE_CORRECT);
   if ( !wxPMessageBoxIsDisabled(configPath) )
   {
      bool dontShowAgain = false;

      ShowAttachmentDialog(m_editor->GetWindow(), &props, &dontShowAgain);

      mc->SetName(props.name);
      mc->SetDisposition(props.GetDisposition());

      if ( dontShowAgain )
         wxPMessageBoxDisable(configPath, wxNO);
   }


   strMimeType = props.mimetype.GetFull();
   DoInsertAttachment(mc, strMimeType);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"),
               filename.c_str(), strMimeType.c_str());

}

/// insert a text file at the current cursor position
bool
wxComposeView::InsertFileAsText(const String& filename,
                                MessageEditor::InsertMode insMode)
{
   // read the text from the file
   wxString text;
   off_t lenFile = 0;      // suppress warning
   wxFile file(filename);

   bool ok = file.IsOpened();
   if ( ok )
   {
      lenFile = file.Length();
      if ( lenFile == 0 )
      {
         if ( insMode != MessageEditor::Insert_Replace )
         {
            wxLogVerbose(_("File '%s' is empty, no text to insert."),
                         filename.c_str());
            return true;
         }
         //else: replace old text with new (empty) one
      }
      else // non empty file
      {
         char *p = text.GetWriteBuf(lenFile + 1);
         p[lenFile] = '\0';

         ok = file.Read(p, lenFile) != wxInvalidOffset;

         text.UngetWriteBuf();
      }
   }

   if ( !ok )
   {
      wxLogError(_("Cannot insert text file into the message."));

      return false;
   }

   m_editor->InsertText(text, insMode);

   return true;
}

/// inserts a text
void
wxComposeView::InsertText(const String &text)
{
   m_editor->InsertText(text, MessageEditor::Insert_Append);
}

void
wxComposeView::InsertMimePart(const MimePart *mimePart)
{
   CHECK_RET( mimePart, _T("no top MIME part in the message to edit?") );

   MimeType type = mimePart->GetType();
   switch ( type.GetPrimary() )
   {
      case MimeType::MULTIPART:
         {
            const MimePart *partChild = mimePart->GetNested();

            String subtype = type.GetSubType();
            if ( subtype == "ALTERNATIVE" )
            {
               // assume that we can edit the first subpart, this must be the
               // most "rough" one and as we only support editing text, if we
               // can edit anything at all it's going to be this one
               InsertMimePart(partChild);
            }
            else // assume MIXED for all others
            {
               // and so simply insert all parts in turn
               while ( partChild )
               {
                  InsertMimePart(partChild);

                  partChild = partChild->GetNext();
               }
            }
         }
         break;

      case MimeType::TEXT:
         InsertText(mimePart->GetTextContent());
         break;

      case MimeType::MESSAGE:
      case MimeType::APPLICATION:
      case MimeType::AUDIO:
      case MimeType::IMAGE:
      case MimeType::VIDEO:
      case MimeType::MODEL:
      case MimeType::OTHER:
         // anything else gets inserted as an attachment
         {
            // we need to make a copy of the data as we're going to free() it
            unsigned long len;
            const void *data = mimePart->GetContent(&len);
            CHECK_RET( data, _T("failed to retrieve the message data") );

            void *data2 = malloc(len);
            memcpy(data2, data, len);

            InsertData(data2, len,
                       mimePart->GetType().GetFull(),
                       mimePart->GetFilename(),
                       mimePart->GetParam(_T("NAME")));
         }
         break;

      case MimeType::CUSTOM1:
      case MimeType::CUSTOM2:
      case MimeType::CUSTOM3:
      case MimeType::CUSTOM4:
      case MimeType::CUSTOM5:
      case MimeType::CUSTOM6:
      default:
         FAIL_MSG( _T("unknown MIME type") );
   }
}

// ----------------------------------------------------------------------------
// wxComposeView sending the message
// ----------------------------------------------------------------------------

/// verify that the message is ready to be sent
bool
wxComposeView::IsReadyToSend() const
{
   // verify that the external editor has terminated
   if ( m_procExtEdit )
   {
      // we don't give the user a choice because it will later leave us with
      // an orphan ext editor process and it's easier this way
      wxLogError(_("The external editor is still opened, please close "
                   "it before sending the message."));

      return false;
   }

   // verify that the network is configured
   bool networkSettingsOk = false;
   while ( !networkSettingsOk )
   {
      // we should have either SMTP host or a sendmail command
      String hostOrCmd;
#ifdef OS_UNIX
      if ( READ_CONFIG(m_Profile, MP_USE_SENDMAIL) )
         hostOrCmd = READ_CONFIG_TEXT(m_Profile, MP_SENDMAILCMD);
      else
#endif // OS_UNIX
         hostOrCmd = READ_CONFIG_TEXT(m_Profile, MP_SMTPHOST);

      if ( hostOrCmd.empty() )
      {
         if ( MDialog_YesNoDialog(
                  _("The message can not be sent because the network settings "
                    "are either not configured or incorrect. Would you like to "
                    "change them now?"),
                  this,
                  MDIALOG_YESNOTITLE,
                  M_DLG_YES_DEFAULT,
                  M_MSGBOX_CONFIG_NET_FROM_COMPOSE) )
         {
            ShowOptionsDialog((wxComposeView *)this, OptionsPage_Network);
         }
         else
         {
            wxLogError(_("Cannot send message - network is not configured."));

            return FALSE;
         }
      }
      else
      {
         // TODO any other vital settings to check?
         networkSettingsOk = true;
      }
   }

   // did we forget the recipients?
   //
   // NB: it is valid to have only BCC: recipients, c-client will deal with
   //     this case by adding a dummy to header
   if ( GetRecipients(Recipient_To).empty() &&
         GetRecipients(Recipient_Newsgroup).empty() &&
           GetRecipients(Recipient_Bcc).empty() )
   {
      wxLogError(_("Please specify at least one \"To:\" or "
                   "\"Newsgroup:\" recipient!"));

      return false;
   }

   // did we forget the subject?
   if ( GetSubject().empty() )
   {
      // this one is not strictly speaking mandatory, so just ask the user
      // about it (giving him a chance to get rid of annoying msg box)
      if ( MDialog_YesNoDialog(
               _("The subject of your message is empty, would you like "
                 "to change it?"),
               this,
               MDIALOG_YESNOTITLE,
               M_DLG_YES_DEFAULT,
               M_MSGBOX_SEND_EMPTY_SUBJECT) )
      {
         m_txtSubject->SetFocus();

         return false;
      }
   }

   // everything is ok
   return true;
}

SendMessage *
wxComposeView::BuildMessage() const
{
   // create the message object
   // -------------------------

   Protocol proto;
   switch ( m_mode )
   {
      default:
         FAIL_MSG( _T("unknown protocol") );
         // fall through nevertheless

      case Mode_Mail:
         proto = Prot_Default;
         break;

      case Mode_News:
         proto = Prot_NNTP;
         break;
   }

   // Create the message to be composed
   SendMessage *msg = SendMessage::Create(m_Profile, proto, ::GetFrame(this));
   if ( !msg )
   {
      // can't do anything more
      return NULL;
   }

   // compose the body
   // ----------------

   for ( EditorContentPart *part = m_editor->GetFirstPart();
         part;
         part = m_editor->GetNextPart() )
   {
      switch ( part->GetType() )
      {
         case EditorContentPart::Type_Text:
            msg->AddPart(
                           MimeType::TEXT,
                           part->GetText(),
                           part->GetLength(),
                           "PLAIN",
                           "INLINE",   // disposition
                           NULL,       // disposition parameters
                           NULL,       // other parameters
                           m_encoding
                        );
            break;

         case EditorContentPart::Type_File:
            {
               String filename = part->GetFileName();
               wxFile file;
               if ( file.Open(filename) )
               {
                  size_t size = file.Length();
                  char *buffer = new char[size + 1];
                  if ( file.Read(buffer, size) )
                  {
                     // always NUL terminate it
                     buffer[size] = '\0';

                     // use the user provided name instead of local filename if
                     // it was given
                     String name = part->GetName();
                     if ( name.empty() )
                     {
                        name = filename;
                     }

                     MessageParameterList plist, dlist;
                     MessageParameter *p;

                     // some mailers want "FILENAME" in disposition parameters
                     // (where only file name, i.e. without path, should be
                     // used for obvious security reasons)
                     p = new MessageParameter("FILENAME",
                                              wxFileNameFromPath(name));
                     dlist.push_back(p);

                     // and some mailers want "NAME" in parameters (we can use
                     // the full name here)
                     p = new MessageParameter("NAME", filename);
                     plist.push_back(p);

                     const MimeType& mt = part->GetMimeType();
                     msg->AddPart
                          (
                            mt.GetPrimary(),
                            buffer, size,
                            mt.GetSubType(),
                            part->GetDisposition(),
                            &dlist, &plist
                          );
                  }
                  else
                  {
                     wxLogError(_("Cannot read file '%s' included in "
                                  "this message!"), filename.c_str());
                  }

                  delete [] buffer;
               }
               else
               {
                  wxLogError(_("Cannot open file '%s' included in "
                               "this message!"), filename.c_str());
               }
            }
            break;

         case EditorContentPart::Type_Data:
            {
               MessageParameterList dlist, plist;

               String filename = part->GetFileName();
               String name = part->GetName();

               if ( !name.empty() )
               {
                  MessageParameter *p;

                  p = new MessageParameter("FILENAME", wxFileNameFromPath(name));
                  dlist.push_back(p);
               }

               if ( !filename.empty() )
               {
                  MessageParameter *p;

                  p = new MessageParameter("NAME", filename);
                  plist.push_back(p);
               }

               const MimeType& mt = part->GetMimeType();
               msg->AddPart
                    (
                      mt.GetPrimary(),
                      (char *)part->GetData(),
                      part->GetSize(),
                      mt.GetSubType(),
                      part->GetDisposition(),
                      &dlist,
                      &plist
                    );
            }
            break;

         default:
            FAIL_MSG( _T("Unknown editor content part type!") );
      }

      part->DecRef();
   }

   // setup the headers
   // -----------------

   if ( m_encoding != wxFONTENCODING_DEFAULT )
   {
      msg->SetHeaderEncoding(m_encoding);
   }

   // first the standard ones
   msg->SetSubject(GetSubject());

   // recipients
   String rcptTo = GetRecipients(Recipient_To),
          rcptCC = GetRecipients(Recipient_Cc),
          rcptBCC = GetRecipients(Recipient_Bcc);
   msg->SetAddresses(rcptTo, rcptCC, rcptBCC);

   String newsgroups = GetRecipients(Recipient_Newsgroup);
   if ( !newsgroups.empty() )
   {
      msg->SetNewsgroups(newsgroups);
   }

   // from
   String from = GetFrom();
   if ( !from.empty() )
   {
      msg->SetFrom(from);
   }

   // fcc
   msg->SetFcc(GetRecipients(Recipient_Fcc));

   // add any additional header lines: first for this time only and then also
   // the headers stored in the profile
   kbStringList::iterator i = m_ExtraHeaderLinesNames.begin();
   kbStringList::iterator i2 = m_ExtraHeaderLinesValues.begin();
   for ( ; i != m_ExtraHeaderLinesNames.end(); i++, i2++ )
   {
      msg->AddHeaderEntry(**i, **i2);
   }

   wxArrayString headerNames, headerValues;
   size_t nHeaders = GetCustomHeaders(m_Profile,
                                      m_mode == Mode_Mail ? CustomHeader_Mail
                                                          : CustomHeader_News,
                                      &headerNames,
                                      &headerValues);
   for ( size_t nHeader = 0; nHeader < nHeaders; nHeader++ )
   {
      msg->AddHeaderEntry(headerNames[nHeader], headerValues[nHeader]);
   }

   return msg;
}

bool
wxComposeView::Send(bool schedule)
{
   CHECK( !m_sending, false, _T("wxComposeView::Send() reentered") );

   m_sending = true;

   wxBusyCursor bc;
   wxLogStatus(this, m_mode == Mode_News ? _("Posting message")
                                         : _("Sending message..."));
   Disable();

   SendMessage *msg = BuildMessage();
   if ( !msg )
   {
      wxLogError(_("Failed to create the message to send."));

      return false;
   }

   // and now do send the message
   bool success;
   if ( schedule )
   {
      MModule_Calendar *calmod =
         (MModule_Calendar *) MModule::GetProvider(MMODULE_INTERFACE_CALENDAR);
      if(calmod)
      {
         success = calmod->ScheduleMessage(msg);
         calmod->DecRef();
      }
      else
      {
         wxLogError(_("Cannot schedule message for sending later because "
                      "the calendar module is not available."));

         success = FALSE;
      }
   }
   else
   {
      success = msg->SendOrQueue();
   }

   delete msg;

   if ( success )
   {
      if ( m_OriginalMessage != NULL )
      {
         // we mark the original message as "answered"
         MailFolder *mf = m_OriginalMessage->GetFolder();
         if(mf)
         {
            mf->SetMessageFlag(m_OriginalMessage->GetUId(),
                               MailFolder::MSG_STAT_ANSWERED, true);
         }
      }

      ResetDirty();
      mApplication->UpdateOutboxStatus();

      // show the recipients of the message
      String msg;
      if ( m_mode == Mode_News )
      {
         msg.Printf(_("Message has been posted to %s"),
                    GetRecipients(Recipient_Newsgroup).c_str());
      }
      else // email message
      {
         // NB: don't show BCC as the message might be saved in the log file
         msg.Printf(_("Message has been sent to %s"),
                    GetRecipients(Recipient_To).c_str());

         String rcptCC = GetRecipients(Recipient_Cc);
         if ( !rcptCC.empty() )
         {
            msg += String::Format(_(" (with courtesy copy sent to %s)"),
                                  rcptCC.c_str());
         }
         else // no CC
         {
            msg += '.';
         }
      }

      // avoid crashes if the msg has any stray '%'s
      wxLogStatus(this, "%s", msg.c_str());
   }
   else // message not sent
   {
      if ( mApplication->GetLastError() != M_ERROR_CANCEL )
      {
         wxLogError(_("The message couldn't be sent."));
      }
      //else: cancelled by user, don't give the error

      wxLogStatus(this, _("Message was not sent."));
   }

   // reenable the window disabled previously
   Enable();

   m_sending = false;

   if ( !success )
   {
      return false;
   }

   // we can now safely remove the draft message, if any
   DeleteDraft();

   return true;
}

void
wxComposeView::AddHeaderEntry(const String &entry,
                              const String &ivalue)
{
   String
      *name = new String(entry),
      *value = new String(ivalue);

   m_ExtraHeaderLinesNames.push_back(name);
   m_ExtraHeaderLinesValues.push_back(value);
}

// -----------------------------------------------------------------------------
// simple GUI accessors
// -----------------------------------------------------------------------------

void
wxComposeView::SetFrom(const String& from)
{
   m_from = from;

   // be careful here: m_txtFrom might be not shown
   if ( m_txtFrom )
      m_txtFrom->SetValue(from);
}

/// sets From field using the current profile
void
wxComposeView::SetDefaultFrom()
{
   if ( m_txtFrom )
   {
      AddressList_obj addrList(AddressList::CreateFromAddress(m_Profile));

      Address *addr = addrList->GetFirst();
      if ( addr )
      {
         SetFrom(addr->GetAddress());
      }
   }
}

/// sets Subject field
void
wxComposeView::SetSubject(const String &subj)
{
   m_txtSubject->SetValue(subj);
}

void wxComposeView::ResetDirty()
{
   // ensure that CanClose() returns true now
   m_editor->ResetDirty();

   m_txtSubject->DiscardEdits();
   if ( m_txtFrom )
      m_txtFrom->DiscardEdits();
   m_rcptMain->GetText()->DiscardEdits();

   m_isModified = false;
}

void wxComposeView::SetDirty()
{
   m_isModified = true;
}

bool wxComposeView::IsModified() const
{
   return m_isModified || m_editor->IsModified();
}

bool wxComposeView::IsEmpty() const
{
   return m_editor->IsEmpty();
}

// ----------------------------------------------------------------------------
// other wxComposeView operations
// ----------------------------------------------------------------------------

void
wxComposeView::SetFocusToComposer()
{
   if ( m_editor )
      m_editor->SetFocus();
}

/// position the cursor in the composer window
void
wxComposeView::MoveCursorTo(int x, int y)
{
   m_editor->MoveCursorTo((unsigned long)x, (unsigned long)y);
}

void
wxComposeView::MoveCursorBy(int x, int y)
{
   m_editor->MoveCursorBy(x, y);
}

/// print the message
void
wxComposeView::Print(void)
{
   m_editor->Print();
}

// ----------------------------------------------------------------------------
// wxComposeView saving
// ----------------------------------------------------------------------------

// save the first text part of the message to the given file
bool
wxComposeView::SaveMsgTextToFile(const String& filename) const
{
   // TODO write (and read later...) headers too?

   // write all text parts of the message into a file
   wxFile file(filename, wxFile::write_append);
   if ( !file.IsOpened() )
   {
      wxLogError(_("Cannot open file for the message."));

      return false;
   }

   // iterate over all message parts
   for ( EditorContentPart *part = m_editor->GetFirstPart();
         part;
         part = m_editor->GetNextPart() )
   {
      if ( part->GetType() == EditorContentPart::Type_Text )
      {
         // call Translate() to ensure that the text has the native line
         // termination characters
         if ( !file.Write(wxTextBuffer::Translate(part->GetText())) )
         {
            wxLogError(_("Cannot write message to file."));

            // continue nevertheless with the other parts, we must finish
            // enumerating them
         }
      }

      part->DecRef();
   }

   return true;
}

// ----------------------------------------------------------------------------
// wxComposeView support for drafts folder
// ----------------------------------------------------------------------------

bool wxComposeView::DeleteDraft()
{
   if ( !m_DraftMessage )
      return false;

   if ( !READ_CONFIG(m_Profile, MP_DRAFTS_AUTODELETE) )
      return false;

   // NB: GetFolder() doesn't IncRef() the result, so don't DecRef() it
   MailFolder *mf = m_DraftMessage->GetFolder();
   CHECK( mf, false, _T("draft message without folder?") );

   if ( !mf->DeleteMessage(m_DraftMessage->GetUId()) )
   {
      wxLogError(_("Failed to delete the original draft message from "
                   "the folder '%s'."), mf->GetName().c_str());
      return false;
   }

   m_DraftMessage->DecRef();
   m_DraftMessage = NULL;

   return true;
}

SendMessage *wxComposeView::BuildDraftMessage() const
{
   SendMessage *msg = BuildMessage();
   if ( !msg )
   {
      wxLogError(_("Failed to create the message to save."));
   }
   else
   {
      // mark this message as our draft (the value doesn't matter)
      msg->AddHeaderEntry(HEADER_IS_DRAFT, "Yes");

      // save the composer geometry info
      String value;
      wxFrame *frame = ((wxComposeView *)this)->GetFrame();
      if ( frame->IsIconized() )
      {
         value = GEOMETRY_ICONIZED;
      }
      else if ( frame->IsMaximized() )
      {
         value = GEOMETRY_MAXIMIZED;
      }
      else // normal position
      {
         int x, y, w, h;
         frame->GetPosition(&x, &y);
         frame->GetSize(&w, &h);

         value.Printf(GEOMETRY_FORMAT, x, y, w, h);
      }

      msg->AddHeaderEntry(HEADER_GEOMETRY, value);
   }

   return msg;
}

// from upgrade.cpp, this forward decl will disappear once we move it somewhere
// else (most likely MFolder?)
extern int
VerifyStdFolder(const MOption& optName,
                const String& nameDefault,
                int flags,
                const String& comment,
                MFolderIndex idxInTree = MFolderIndex_Max,
                int icon = -1);

bool wxComposeView::SaveAsDraft() const
{
   SendMessage_obj msg = BuildDraftMessage();
   if ( !msg )
   {
      // error message already given by BuildDraftMessage()
      return false;
   }

   // ensure that the "Drafts" folder we're going to save the message to exists
   String nameDrafts = READ_CONFIG(m_Profile, MP_DRAFTS_FOLDER);
   if ( !nameDrafts.empty() )
   {
      if ( !MFolder_obj(nameDrafts) )
         nameDrafts.clear();
   }

   if ( nameDrafts.empty() )
   {
      // try the standard folder
      int rc = VerifyStdFolder
               (
                  MP_DRAFTS_FOLDER,
                  _("Drafts"),
                  0,
                  _("Folder where the postponed messages are stored,"),
                  MFolderIndex_Draft
               );

      if ( !rc )
      {
         wxLogError(_("Please try specifying another \"Drafts\" folder in "
                      "the preferences dialog."));
         return false;
      }

      // VerifyStdFolder() writes the name to profile
      nameDrafts = READ_APPCONFIG_TEXT(MP_DRAFTS_FOLDER);

      if ( rc == -1 )
      {
         // refresh the tree to show the newly created folder
         MEventManager::Send(
            new MEventFolderTreeChangeData(nameDrafts,
                                           MEventFolderTreeChangeData::Create)
         );
      }
   }

   if ( !msg->WriteToFolder(nameDrafts) )
   {
      wxLogError(_("Failed to save the message as a draft."));

      return false;
   }

   // do this so that we can close unconditionally now
   wxComposeView *self = (wxComposeView *)this;
   self->ResetDirty(); // const_cast

   // we can now safely remove the old draft message, if any
   self->DeleteDraft();

   MDialog_Message
   (
      String::Format
      (
         _("Your message has been saved in the folder '%s',\n"
           "simply open it and choose \"Message|Edit in composer\" to\n"
           "continue writing it."),
         nameDrafts.c_str()
      ),
      self->GetFrame(),
      M_MSGBOX_DRAFT_SAVED,
      M_DLG_DISABLE
   );

   return true;
}

// ----------------------------------------------------------------------------
// wxComposeView auto save
// ----------------------------------------------------------------------------

/**
  Returns the directory used for autosaved composer contents, with '/' at the
  end
 */
static String GetComposerAutosaveDir()
{
   String name = mApplication->GetLocalDir();
   name += "/composer/";

   return name;
}

bool
wxComposeView::AutoSave()
{
   if ( !m_editor->IsModified() )
   {
      // nothing to do
      return true;
   }

   SendMessage_obj msg = BuildDraftMessage();
   if ( !msg )
      return false;

   if ( m_filenameAutoSave.empty() )
   {
      // make sure the directory we use for these scratch files exists
      String name = GetComposerAutosaveDir();
      if ( !wxDir::Exists(name) )
      {
         if ( !wxMkdir(name, 0700) )
         {
            wxLogSysError(_("Failed to create the directory '%s' for the "
                            "temporary composer files"), name.c_str());
            return false;
         }
      }

      // we need a unique file name during the life time of this object as this
      // file is always going to be deleted if we're destroyed correctly, it
      // can only be left if the program crashes
      m_filenameAutoSave = name + String::Format("%05d%p", (int)getpid(), this);
   }

   String contents;
   if ( !msg->WriteToString(contents) )
   {
      // this is completely unexpected
      FAIL_MSG( _T("Failed to get the message text?") );

      return false;
   }

   if ( !MailFolder::SaveMessageAsMBOX(m_filenameAutoSave, contents) )
   {
      // TODO: disable autosaving? we risk to give many such messages if
      //       something is wrong...
      wxLogError(_("Failed to automatically save the message."));

      return false;
   }

   // mark the editor as not modified to avoid resaving it the next time
   // unnecessary but remember internally that it was modified (we didn't
   // really save it)
   m_isModified = true;
   m_editor->ResetDirty();

   return true;
}

int Composer::SaveAll()
{
   int rc = 0;
   for ( ComposerList::iterator i = gs_listOfAllComposers.begin();
         i != gs_listOfAllComposers.end();
         ++i )
   {
      if ( (*i)->AutoSave() )
         rc++;
      else
         rc = -1;
   }

   return rc;
}

bool Composer::RestoreAll()
{
   String name = GetComposerAutosaveDir();
   if ( !wxDir::Exists(name) )
   {
      // nothing to do
      return true;
   }

   wxDir dir;
   if ( !dir.Open(name) )
   {
      wxLogError(_("Failed to check for interrupted messages."));

      return false;
   }

   size_t nResumed = 0;

   wxString filename;
   bool cont = dir.GetFirst(&filename, "", wxDIR_FILES);
   while ( cont )
   {
      filename = name + filename;

      MFolder_obj folder(MFolder::CreateTempFile
                         (
                           String::Format(_("Interrupted message %d"),
                                          nResumed + 1),
                           filename
                         ));

      cont = dir.GetNext(&filename);

      if ( folder )
      {
         MailFolder_obj mf = MailFolder::OpenFolder(folder);
         if ( mf )
         {
            // FIXME: assume UID of the first message in a new MBX folder is
            //        always 1
            Message *msg = mf->GetMessage(1);
            if ( msg )
            {
               // EditMessage takes ownership of the pointer
               Composer *composer = EditMessage(mApplication->GetProfile(), msg);
               if ( composer )
               {
                  // ok!
                  nResumed++;

                  // mark the message as dirty to prevent the window from being
                  // closed without confirmation (thus loosing the message we
                  // had saved!)
                  composer->SetDirty();

                  continue;
               }
            }
         }

         // don't delete the file if we failed to restore the message!
         folder->ResetFlags(MF_FLAGS_TEMPORARY);
      }

      wxLogError(_("Failed to resume composing the message from file '%s'"),
                 filename.c_str());
   }

   if ( nResumed )
   {
      wxLogMessage(_("%d previously interrupted composer windows have been "
                     "restored"), nResumed);
   }

   return true;
}

// ----------------------------------------------------------------------------
// MessageEditor attachment properties editing dialog
// ----------------------------------------------------------------------------

void
MessageEditor::EditAttachmentProperties(EditorContentPart *part)
{
   CHECK_RET( part, _T("no attachment to edit in EditAttachmentProperties") );

   AttachmentProperties props;
   props.filename = part->GetFileName();
   props.name = part->GetName();
   props.SetDisposition(part->GetDisposition());
   props.mimetype = part->GetMimeType();

   if ( ShowAttachmentDialog(GetWindow(), &props) )
   {
      part->SetFile(props.filename);
      part->SetName(props.name);
      part->SetMimeType(props.mimetype.GetFull());
      part->SetDisposition(props.GetDisposition());
   }
   //else: cancelled by user or nothing changed
}

