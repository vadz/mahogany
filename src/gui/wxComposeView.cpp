///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxComposeView.cpp - composer GUI code
// Purpose:     shows a frame containing the header controls and editor window
// Author:      Karsten Ball�der, Vadim Zeitlin
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

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "sysutil.h"
#  include "strutil.h"
#  include "Mdefaults.h"
#  include "MHelp.h"
#  include "gui/wxIconManager.h"

#  include <wx/sizer.h>
#  include <wx/menu.h>
#  include <wx/filedlg.h>
#  include <wx/stattext.h>
#  include <wx/dataobj.h>
#  include <wx/wxchar.h>      // for wxPrintf/Scanf
#endif // USE_PCH

#ifdef __CYGWIN__
#  include <sys/unistd.h>     // for getpid()
#endif
#ifdef __MINGW32__
#  include <process.h>        // for getpid()
#endif

#ifdef OS_UNIX
   #include <sys/types.h>
   #include <unistd.h>
#endif

#include <wx/clipbrd.h>
#include <wx/process.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/dir.h>
#include <wx/process.h>
#include <wx/regex.h>
#include <wx/mimetype.h>
#include <wx/tokenzr.h>
#include <wx/textbuf.h>
#include <wx/fontmap.h>
#include <wx/scopeguard.h>
#ifdef __WINE__
// it includes wrapwin.h which includes windows.h which defines SendMessage under Windows
#undef SendMessage
#endif // __WINE__
#include <wx/fontutil.h>      // for wxNativeFontInfo

// windows.h included from wx/fontutil.h under Windows #defines this
#ifdef __CYGWIN__
#  undef SendMessage
#endif

#include "Mpers.h"

#include "HeadersDialogs.h"

#include "gui/AddressExpander.h"
#include "gui/wxIdentityCombo.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxComposeView.h"
#include "gui/wxMenuDefs.h"

#include "adb/AdbManager.h"

#include "mail/Header.h"
#include "mail/MimeDecode.h"

#include "ConfigSourcesAll.h"
#include "TemplateDialog.h"
#include "AttachDialog.h"
#include "MFolder.h"
#include "Address.h"
#include "SendMessage.h"
#include "Message.h"
#include "MessageView.h"
#include "Collect.h"
#include "ColourNames.h"
#include "QuotedText.h"

#include "modules/Calendar.h"

#include "Mdnd.h"
#include "UIdArray.h"


// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADB_SUBSTRINGEXPANSION;
extern const MOption MP_ALWAYS_USE_EXTERNALEDITOR;
extern const MOption MP_AUTOCOLLECT_INCOMING;
extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_AUTOCOLLECT_NAMED;
extern const MOption MP_AUTOCOLLECT_OUTGOING;
extern const MOption MP_CHECK_ATTACHMENTS_REGEX;
extern const MOption MP_CHECK_FORGOTTEN_ATTACHMENTS;
extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_CC;
extern const MOption MP_COMPOSE_PGPSIGN;
extern const MOption MP_COMPOSE_PGPSIGN_AS;
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
extern const MPersMsgBox *M_MSGBOX_ASK_SAVE_HEADERS;
extern const MPersMsgBox *M_MSGBOX_ASK_VCARD;
extern const MPersMsgBox *M_MSGBOX_CHECK_FORGOTTEN_ATTACHMENTS;
extern const MPersMsgBox *M_MSGBOX_CONFIG_NET_FROM_COMPOSE;
extern const MPersMsgBox *M_MSGBOX_DRAFT_AUTODELETE;
extern const MPersMsgBox *M_MSGBOX_DRAFT_SAVED;
extern const MPersMsgBox *M_MSGBOX_FIX_TEMPLATE;
extern const MPersMsgBox *M_MSGBOX_OPEN_ANOTHER_COMPOSER;
extern const MPersMsgBox *M_MSGBOX_MIME_TYPE_CORRECT;
extern const MPersMsgBox *M_MSGBOX_SEND_DIFF_ENCODING;
extern const MPersMsgBox *M_MSGBOX_SEND_EMPTY_SUBJECT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the header used to indicate that a message is our draft
#define HEADER_IS_DRAFT _T("X-M-Draft")

// the header used for storing the composer geometry
#define HEADER_GEOMETRY _T("X-M-Geometry")

// the possible values for HEADER_GEOMETRY
#define GEOMETRY_ICONIZED _T("I")
#define GEOMETRY_MAXIMIZED _T("M")
#define GEOMETRY_FORMAT _T("%dx%d-%dx%d")

// separate multiple addresses with commas
#define CANONIC_ADDRESS_SEPARATOR   _T(", ")

/// Ids for events sent by background threads we use.
enum
{
   /// Message sending thread terminated.
   SendThread_Done = 1
};

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

M_LIST(ComposerList, wxComposeView *);

static ComposerList gs_listOfAllComposers;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

namespace
{

// return the string containing the full mime type for the given filename (uses
// its extension)
wxString GetMimeTypeFromFilename(const wxString& filename)
{
   wxString strExt = wxFileName(filename).GetExt();

   wxString strMimeType;
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
   wxFileType *fileType = strExt.empty()
                           ? NULL
                           : mimeManager.GetFileTypeFromExtension(strExt);
   if ( !fileType || !fileType->GetMimeType(&strMimeType) )
   {
      // can't find MIME type from file extension, set some default one: use
      // TEXT/PLAIN for text files and APPLICATION/OCTET-STREAM for binary ones

      bool isBinary = true;
      if ( wxFile::Access(filename, wxFile::read) )
      {
         wxFile file(filename);

         // to check whether the file is text, just read its first 256 bytes
         // and check if there are any non-alnum characters among them and,
         // also, if there are any new lines
         unsigned char buf[256];
         ssize_t len = file.Read(buf, WXSIZEOF(buf));
         if ( len != -1 )
         {
            bool eol = false,
                 ctrl = false;
            for ( ssize_t n = 0; n < len && !ctrl; n++ )
            {
               const unsigned char ch = buf[n];
               switch ( ch )
               {
                  case '\r':
                  case '\n':
                     eol = true;
                     break;

                  case '\t':
                     break;

                  default:
                     if ( ch < ' ' || ch == 0x7f )
                        ctrl = true;
               }
            }

            isBinary = ctrl || !eol;
         }
      }

      strMimeType = isBinary ? _T("APPLICATION/OCTET-STREAM") : _T("TEXT/PLAIN");
   }

   delete fileType;  // may be NULL, ok

   return strMimeType;
}

/**
    Return clipboard contents if it looks like an email address.

    This function allows us to do something smart if the user had just selected
    an email address. Currently we pre-fill the composer recipient with the
    selected address in this case but it might be useful elsewhere in the
    future in which case it should be extracted in a public header.

    @return
      An email address string or empty string if clipboard contents doesn't
      look like an email address.
 */
String TryToGetAddressFromClipboard()
{
   wxClipboardLocker clipboardLock;
   if ( !clipboardLock )
      return String();

   wxTextDataObject data;
   if ( !wxTheClipboard->GetData(data) )
      return String();

   const wxString s = data.GetText();
   if ( s.empty() )
      return String();

   // this is simplistic but good enough for our needs
   #define RE_EMAIL "[a-z0-9._%+-]+@([a-z0-9-]{2,}\\.)+[a-z]{2,3}"

   // the alternatives correspond to: "Foo Bar <foo@bar.org>", "<foo@bar.org>"
   // and just "foo@bar.org"
   wxRegEx re("^(([^ ]+ ){1,4}<" RE_EMAIL ">|<" RE_EMAIL ">|" RE_EMAIL ")$",
              wxRE_EXTENDED | wxRE_ICASE);

   if ( !re.Matches(s) )
      return String();

   return s;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/**
    Information returned from message sending thread.
 */
class SendThreadResult
{
public:
   /**
       Create an object indicating successful message sending.
    */
   SendThreadResult()
   {
      success = true;
   }

   /**
       Create an object indicating a failure to send the message.

       This ctor is intentionally not explicit to allow easily calling
       OnSendResult() with a (error) string instead of this object.

       @param err The error message. If it's not empty, the object represents a
         failure. Otherwise it's a successful result.
    */
   SendThreadResult(const wxString& err)
      : errGeneral(err)
   {
      success = errGeneral.empty();
   }

   // default copy ctor, assignment operator and dtor are ok

   /**
       True if message was successfully sent or queued for later posting.

       If it is false, an error occurred or sending of the message was
       cancelled.
    */
   bool success;

   /**
       If not empty, the message was queued in this outbox.

       Otherwise it was sent directly.
    */
   wxString outbox;

   /**
       General error message if sending the message failed.

       It may be empty even if @c success is false if sending the message was
       simply cancelled.
    */
   wxString errGeneral;

   /**
       Detailed error message.

       This message is not always available but should be shown to the user if
       it is as it may contain detailed information about the error necessary
       for debugging it.
    */
   wxString errDetailed;
};

// ----------------------------------------------------------------------------
// SendThread: background thread for sending the message
// ----------------------------------------------------------------------------

class SendThread : public wxThread
{
public:
   /**
       Creates the thread object for sending the specified message.

       The thread is associated with the given wxComposeView object which will
       be notified about its success or failure by posting a wxThreadEvent to
       it.

       The ctor just initializes the thread object but doesn't create a new
       thread of execution, use the base class Run() method to really start the
       thread.

       @param composer The composer window to notify about the result of the
         operation. We assume that the composer remains opened (and hence this
         object valid) for as long as this thread is running, so the composer
         must refuse to close before we terminate.
       @param msg The message to send. This object should also have a lifetime
         longer than that of this object.
    */
   SendThread(wxComposeView& composer, SendMessage& msg)
      : m_composer(composer),
        m_msg(msg)
   {
   }

protected:
   /// The thread entry function.
   virtual void *Entry()
   {
      SendThreadResult res;
      if ( !m_msg.SendNow(&res.errGeneral, &res.errDetailed) )
         res.success = false;

      wxThreadEvent evt;
      evt.SetId(SendThread_Done);
      evt.SetPayload(res);

      wxQueueEvent(&m_composer, evt.Clone());

      return NULL;
   }

private:
   wxComposeView& m_composer;
   SendMessage& m_msg;

   SendMessage::Result m_result;

   wxDECLARE_NO_COPY_CLASS(SendThread);
};

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
   void InitControls(const String& value, RecipientType rt);

   // get our text control
   wxAddressTextCtrl *GetText() const { return m_text; }

   // get the currently selected address type
   RecipientType GetType() const;

   // change the address type
   void SetType(RecipientType rcptType);

   // get the current value of the text field
   wxString GetValue() const;


   // helper for layout
   int GetTypeControlWidth() const;


   // starting from now, all methods are for the wxRcptXXX controls only

   // change type of this one -- called by choice
   virtual void OnTypeChange(RecipientType rcptType);

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
   virtual void OnTypeChange(RecipientType rcptType);

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

   static const char *ms_addrTypes[Recipient_Max];

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxRcptTypeChoice)
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
                   wxEmptyString,
                   wxDefaultPosition,
                   wxDefaultSize,
                   style | wxTE_PROCESS_ENTER)
      {
      }

protected:
   void OnEnter(wxCommandEvent& event);

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxTextCtrlProcessingEnter)
};

// ----------------------------------------------------------------------------
// wxSubjectTextCtrl: text control for the subject
// ----------------------------------------------------------------------------

class wxSubjectTextCtrl : public wxTextCtrl
{
public:
   wxSubjectTextCtrl(wxWindow *parent, wxComposeView *composeView)
      : wxTextCtrl(parent, wxID_ANY, "",
                   wxDefaultPosition, wxDefaultSize,
                   wxTE_PROCESS_ENTER)
   {
      m_composeView = composeView;
   }

private:
   void OnChange(wxCommandEvent& WXUNUSED(event))
   {
      // update the subject shown in the title bar
      m_composeView->UpdateTitle();
   }

   void OnEnter(wxCommandEvent& WXUNUSED(event))
   {
      // starting to edit the message after the subject is entered is more
      // useful than going (back, i.e. above, as we're near the beginning of
      // the TAB chain) to the recipient controls
      m_composeView->SetFocusToComposer();
   }

   wxComposeView *m_composeView;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxSubjectTextCtrl)
};

BEGIN_EVENT_TABLE(wxSubjectTextCtrl, wxTextCtrl)
   EVT_TEXT(wxID_ANY, wxSubjectTextCtrl::OnChange)
   EVT_TEXT_ENTER(wxID_ANY, wxSubjectTextCtrl::OnEnter)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// IconButton: class used for small buttons in the header
// ----------------------------------------------------------------------------

class IconButton : public wxBitmapButton
{
public:
   IconButton(wxWindow *parent, wxBitmap bmp)
      : wxBitmapButton(parent,
                       wxID_ANY,
                       bmp,
                       wxDefaultPosition,
                       wxDefaultSize,
                       wxBORDER_NONE)
   {
   }

   // we don't want to get in the way when tabbing through header fields
   virtual bool AcceptsFocusFromKeyboard() const { return false; }
};

// ----------------------------------------------------------------------------
// ToggleIconButton: wxToggleButton with images
// ----------------------------------------------------------------------------

class ToggleIconButton : public IconButton
{
public:
   // NB: derived class ctor must call UpdateWithoutRefresh()
   ToggleIconButton(wxComposeView *composer,
                    wxWindow *parent,
                    const char *iconOn,
                    const char *iconOff,
                    const wxString& tooltipOn,
                    const wxString& tooltipOff)
      : IconButton(parent, wxNullBitmap),
        m_composer(composer),
        m_iconOn(iconOn),
        m_iconOff(iconOff),
        m_tooltipOn(tooltipOn),
        m_tooltipOff(tooltipOff)
   {
      Connect(wxEVT_COMMAND_BUTTON_CLICKED,
               wxCommandEventHandler(ToggleIconButton::OnClick));
   }

   void Update()
   {
      UpdateWithoutRefresh();
      Refresh();
   }

protected:
   wxComposeView * const m_composer;

   void UpdateWithoutRefresh()
   {
      m_isOn = DoGetValue();

      SetBitmapLabel(mApplication->GetIconManager()->
                        GetBitmap(m_isOn ? m_iconOn : m_iconOff));
      SetToolTip(m_isOn ? m_tooltipOn : m_tooltipOff);
   }

private:
   // perform the action when the button is clicked
   virtual void DoHandleClick() = 0;

   // return if we're in "on" or "off" state
   virtual bool DoGetValue() const = 0;


   void OnClick(wxCommandEvent& /* event */)
   {
      DoHandleClick();
      if ( DoGetValue() != m_isOn )
         Update();
   }

   const char * const m_iconOn;
   const char * const m_iconOff;

   const wxString m_tooltipOn; 
   const wxString m_tooltipOff; 

   bool m_isOn;

   DECLARE_NO_COPY_CLASS(ToggleIconButton)
};

// ----------------------------------------------------------------------------
// IsReplyButton: button indicating whether this is a reply
// ----------------------------------------------------------------------------

class IsReplyButton : public ToggleIconButton
{
public:
   IsReplyButton(wxComposeView *composer, wxWindow *parent)
      : ToggleIconButton(composer, parent,
                         "tb_mail_reply", "tb_mail_new",
                        _("This is a reply to another message"),
                        _("This is a start of new thread"))
   {
      UpdateWithoutRefresh();

      // AddHeaderEntry("In-Reply-To") is called after composer creation, so we
      // want to update our state a bit later
      CallAfter(&ToggleIconButton::Update);
   }

private:
   virtual void DoHandleClick() { m_composer->ConfigureInReplyTo(); }
   virtual bool DoGetValue() const { return m_composer->IsInReplyTo(); }

   DECLARE_NO_COPY_CLASS(IsReplyButton)
};

// ----------------------------------------------------------------------------
// PGPSignButton: allows the user to choose whether to sign the message or not
// ----------------------------------------------------------------------------

class PGPSignButton : public ToggleIconButton
{
public:
   PGPSignButton(wxComposeView *composer, wxWindow *parent)
      : ToggleIconButton(composer, parent,
                         "pgp_sign", "pgp_nosign",
                         _("Message will be cryptographically signed.\n"
                           "\n"
                           "You will need access to the private key to be "
                           "used for message signing."),
                         _("Message will not be signed"))
   {
      UpdateWithoutRefresh();
   }

private:
   virtual void DoHandleClick() { m_composer->TogglePGPSigning(); }
   virtual bool DoGetValue() const { return m_composer->IsPGPSigningEnabled(); }

   DECLARE_NO_COPY_CLASS(PGPSignButton)
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

class wxAddressTextCtrl : public wxTextCtrlProcessingEnter,
                          public AddressExpander
{
public:
   // ctor
   wxAddressTextCtrl(wxWindow *parent, wxRcptControl *rcptControl);

   // expand the text in the control using the address book(s)
   RecipientType DoExpand();

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
   DECLARE_NO_COPY_CLASS(wxAddressTextCtrl)
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
   DECLARE_NO_COPY_CLASS(wxMainAddressTextCtrl)
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
   DECLARE_NO_COPY_CLASS(wxExtraAddressTextCtrl)
};

// ----------------------------------------------------------------------------
// wxRcptExpandButton: small button used to expand the address entered
// ----------------------------------------------------------------------------

class wxRcptExpandButton : public IconButton
{
public:
   wxRcptExpandButton(wxRcptControl *rcptControl, wxWindow *parent)
      : IconButton(parent,
                   mApplication->GetIconManager()->GetBitmap(_T("tb_lookup")))
      {
         m_rcptControl = rcptControl;
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnExpand(); }

private:
   // the back pointer to the entire group of controls
   wxRcptControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxRcptExpandButton)
};

// ----------------------------------------------------------------------------
// wxRcptAddButton: small button which is used to add a new recipient
// ----------------------------------------------------------------------------

class wxRcptAddButton : public IconButton
{
public:
   wxRcptAddButton(wxRcptMainControl *rcptControl, wxWindow *parent)
      : IconButton(parent, 
                   mApplication->GetIconManager()->GetBitmap(_T("tb_new")))
      {
         m_rcptControl = rcptControl;

#if wxUSE_TOOLTIPS
         SetToolTip(_("Create a new recipient entry"));
#endif // wxUSE_TOOLTIPS
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnAdd(); }

private:
   // the back pointer to the entire group of controls
   wxRcptMainControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxRcptAddButton)
};

// ----------------------------------------------------------------------------
// wxRcptRemoveButton: small button which is used to remove previously added
// recipient controls
// ----------------------------------------------------------------------------

class wxRcptRemoveButton : public IconButton
{
public:
   wxRcptRemoveButton(wxRcptExtraControl *rcptControl, wxWindow *parent)
      : IconButton(parent, 
                   mApplication->GetIconManager()->GetBitmap(_T("tb_trash")))
      {
         m_rcptControl = rcptControl;

#if wxUSE_TOOLTIPS
         SetToolTip(_("Delete this address from the message recipients list"));
#endif // wxUSE_TOOLTIPS
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControl->OnRemove(); }

private:
   // the back pointer to the entire group of controls
   wxRcptExtraControl *m_rcptControl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxRcptRemoveButton)
};

// ----------------------------------------------------------------------------
// AttachmentMenu is the popup menu shown when an attachment is right clicked
// ----------------------------------------------------------------------------

class AttachmentMenu : public wxMenu
{
public:
   // win is the window we're shown in and the part is the attachment we're
   // shown for
   AttachmentMenu(wxWindow *win, EditorContentPart *part);

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

   // operations: this is static as it's also reused from
   // MessageEditor::EditAttachmentProperties()
   static void ShowAttachmentProperties(wxWindow *win, EditorContentPart *part);

private:
   wxWindow *m_window;
   EditorContentPart *m_part;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(AttachmentMenu)
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

   // sending thread notification
   EVT_THREAD(SendThread_Done, wxComposeView::OnSendThreadDone)

   EVT_IDLE(wxComposeView::OnIdle)
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
// AttachmentMenu implementation
// ============================================================================

BEGIN_EVENT_TABLE(AttachmentMenu, wxMenu)
   EVT_MENU(-1, AttachmentMenu::OnCommandEvent)
END_EVENT_TABLE()

AttachmentMenu::AttachmentMenu(wxWindow *win, EditorContentPart *part)
{
   m_window = win;
   m_part = part;

   // create the menu items
   Append(WXMENU_MIME_INFO, _("&Properties..."));
   AppendSeparator();
#if 0 // TODO
   Append(WXMENU_MIME_OPEN, _("&Open"));
   Append(WXMENU_MIME_OPEN_WITH, _("Open &with..."));
#endif
   Append(WXMENU_MIME_VIEW_AS_TEXT, _("&View as text"));
}

/* static */
void
AttachmentMenu::ShowAttachmentProperties(wxWindow *win, EditorContentPart *part)
{
   CHECK_RET( part, _T("no attachment to edit in ShowAttachmentProperties()") );

   AttachmentProperties props;
   props.filename = part->GetFileName();
   props.name = part->GetName();
   props.SetDisposition(part->GetDisposition());
   props.mimetype = part->GetMimeType();

   if ( EditAttachmentProperties(win, &props) )
   {
      part->SetFile(props.filename);
      part->SetName(props.name);
      part->SetMimeType(props.mimetype.GetFull());
      part->SetDisposition(props.GetDisposition());
   }
   //else: cancelled by user or nothing changed
}

void
AttachmentMenu::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      default:
         FAIL_MSG( _T("unexpected menu command in AttachmentMenu") );
         // fall through

      case WXMENU_MIME_INFO:
         ShowAttachmentProperties(m_window, m_part);
         break;

      case WXMENU_MIME_VIEW_AS_TEXT:
         {
            const String& filename = m_part->GetFileName();
            wxFile file(filename);
            wxString content;

            bool ok = file.IsOpened();
            if ( ok )
            {
               const wxFileOffset len = file.Length();
               if ( len == 0 )
               {
                  wxLogWarning(_("Attached file \"%s\" is empty"), filename);
                  break;
               }

               if ( len != wxInvalidOffset )
               {
                  wxCharBuffer buf(len);
                  ok = file.Read(buf.data(), len) != wxInvalidOffset;

                  if ( ok )
                  {
                     buf.data()[len] = '\0';
                     content = wxString::From8BitData(buf, len);
                  }
               }
               else // len == wxInvalidOffset
               {
                  ok = false;
               }
            }

            if ( !ok )
            {
               wxLogError(_("Failed to get data of attached file \"%s\"."), filename);
               break;
            }

            MDialog_ShowText
            (
               GetFrame(m_window),
               wxString::Format(_("Attached file \"%s\""), filename),
               content,
               "AttachView"
            );
         }
         break;
   }
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Composer::Options
// ----------------------------------------------------------------------------

ComposerOptions::ComposerOptions()
{
   m_fontFamily =
   m_fontSize = -1;
}

void ComposerOptions::Read(Profile *profile)
{
   CHECK_RET( profile, _T("NULL profile in Composer::Options::Read") );

   // colours
   ReadColour(&m_fg, profile, MP_CVIEW_FGCOLOUR);
   ReadColour(&m_bg, profile, MP_CVIEW_BGCOLOUR);

   // font
   m_font = READ_CONFIG_TEXT(profile, MP_CVIEW_FONT_DESC);
   if ( m_font.empty() )
   {
      m_fontFamily = GetFontFamilyFromProfile(profile, MP_CVIEW_FONT);
      m_fontSize = READ_CONFIG(profile, MP_CVIEW_FONT_SIZE);
   }

   // PGP options
   m_signWithPGP = READ_CONFIG_BOOL(profile, MP_COMPOSE_PGPSIGN);
   m_signWithPGPAs = READ_CONFIG_TEXT(profile, MP_COMPOSE_PGPSIGN_AS);
}

wxFont ComposerOptions::GetFont() const
{
   return CreateFontFromDesc(m_font, m_fontSize, m_fontFamily);
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
                                const wxChar *name,
                                const wxChar *filename)
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

   if ( m_Disposition.empty() )
      SetDisposition(_T("ATTACHMENT"));
}

void EditorContentPart::SetFile(const String& filename)
{
   ASSERT_MSG( !filename.empty(), _T("a file attachment must have a valid file") );

   m_Name =
   m_FileName = filename;
   m_Type = Type_File;

   if ( m_Disposition.empty() )
      SetDisposition(_T("ATTACHMENT"));
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
   SetMimeType(_T("TEXT/PLAIN"));

   m_Type = Type_Text;
   m_Text = text;

   SetDisposition(_T("INLINE"));
}

EditorContentPart::~EditorContentPart()
{
   if ( m_Type == Type_Data )
   {
      free(m_Data);
   }
}

#ifdef DEBUG

String EditorContentPart::DebugDump() const
{
   String s = MObjectRC::DebugDump();

   s << _T("name = \"") << m_Name
     << _T("\", filename = \"") << m_FileName << _T('"');

   return s;
}

#endif // DEBUG

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
                                  RecipientType rt)
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

void wxRcptControl::OnTypeChange(RecipientType rcptType)
{
   switch ( rcptType )
   {
      case Recipient_To:
      case Recipient_Cc:
      case Recipient_Bcc:
         m_btnExpand->Enable();
#if wxUSE_TOOLTIPS
         m_btnExpand->SetToolTip(_("Expand the address using address books"));
#endif // wxUSE_TOOLTIPS
         break;

      case Recipient_Newsgroup:
         // TODO-NEWS: we can't browse for newsgroups yet
         m_btnExpand->Disable();
         break;

      case Recipient_Fcc:
         // browse for folder now
         m_btnExpand->Enable();
#if wxUSE_TOOLTIPS
         m_btnExpand->SetToolTip(_("Browse for folder"));
#endif // wxUSE_TOOLTIPS
         break;

      case Recipient_None:
         m_btnExpand->Disable();
         break;

      case Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected rcpt type on wxRcptControl") );
   }
}

int wxRcptControl::GetTypeControlWidth() const
{
   return m_choice->GetBestSize().x;
}

void wxRcptControl::OnExpand()
{
   switch ( GetType() )
   {
      case Recipient_To:
      case Recipient_Cc:
      case Recipient_Bcc:
         {
            RecipientType rcptType = m_text->DoExpand();
            if ( rcptType != Recipient_None &&
                  rcptType != Recipient_Max )
            {
               // update the type of the choice control
               SetType(rcptType);
            }
         }
         break;

      case Recipient_Fcc:
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

      case Recipient_Newsgroup:
      case Recipient_None:
      case Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected wxRcptControl::OnExpand() call") );
   }
}

bool wxRcptControl::IsEnabled() const
{
   return m_choice->GetSelection() != Recipient_None;
}

RecipientType wxRcptControl::GetType() const
{
   return (RecipientType)m_choice->GetSelection();
}

void wxRcptControl::SetType(RecipientType rcptType)
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
   GetChoice()->Delete(Recipient_None);
   GetChoice()->SetSelection(Recipient_To);

   // init the m_btnExpand button here
   wxRcptControl::OnTypeChange(GetType());

   return sizer;
}

void wxRcptMainControl::OnTypeChange(RecipientType rcptType)
{
   wxRcptControl::OnTypeChange(rcptType);

   GetComposer()->OnRcptTypeChange(rcptType);
}

void wxRcptMainControl::OnAdd()
{
   // expand before adding (make this optional?)
   wxArrayString addresses;
   wxArrayInt rcptTypes;
   String subject;
   const int count = GetText()->ExpandAll(addresses, rcptTypes, &subject);
   if ( count == -1 )
   {
      // cancelled by user
      return;
   }

   if ( !subject.empty() )
      m_composeView->SetSubject(subject);

   for ( unsigned n = 0; n < (unsigned)count; n++ )
   {
      // add new recipient(s)
      RecipientType rcptType = (RecipientType)rcptTypes[n];
      if ( rcptType == Recipient_None )
      {
         // use the default one
         rcptType = GetType();
      }

      // don't [try to] expand the address again here
      m_composeView->AddRecipients(addresses[n], rcptType,
                                   !Composer::AddRcpt_Expand);
   }

   // clear the entry zone as recipient(s) were moved elsewhere
   GetText()->SetValue(wxEmptyString);
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

const char *wxRcptTypeChoice::ms_addrTypes[] =
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
      addrTypes[n] = wxGetTranslation(ms_addrTypes[n]);
   }

   Create(parent, -1,
          wxDefaultPosition, wxDefaultSize,
          WXSIZEOF(addrTypes), addrTypes);

   m_rcptControl = rcptControl;
}

void wxRcptTypeChoice::OnChoice(wxCommandEvent& event)
{
   // notify the others (including the composer indirectly)
   m_rcptControl->OnTypeChange((RecipientType)event.GetSelection());

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
                 : wxTextCtrlProcessingEnter(parent, wxTE_PROCESS_TAB),
                   AddressExpander(this,
                                     rcptControl->GetComposer()->GetProfile())
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
      if ( event.ControlDown() || event.AltDown() )
         return;

      if ( event.ShiftDown() || !IsModified() )
      {
         Navigate(event.ShiftDown() ? 0 : wxNavigationKeyEvent::IsForward);
      }
      else
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
         }
      }

      // don't call event.Skip()
      return;
   }

   // let the text control process it normally: if it's a TAB this will make
   // the focus go to the next window
   event.Skip();
}

RecipientType wxAddressTextCtrl::DoExpand()
{
   RecipientType rcptType;

   switch ( m_rcptControl->GetType() )
   {
      case Recipient_To:
      case Recipient_Cc:
      case Recipient_Bcc:
         {
            String subject;
            rcptType = ExpandLast(&subject);

            if ( !subject.empty() )
               m_rcptControl->GetComposer()->SetSubject(subject);
         }
         break;

      case Recipient_Newsgroup:
         // TODO-NEWS: we should expand the newsgroups too
         rcptType = Recipient_Newsgroup;
         break;

      case Recipient_Fcc:
         // TODO: we could expand the folder names -- but for now we don't
         rcptType = Recipient_Fcc;
         break;

      case Recipient_Max:
      default:
         FAIL_MSG( _T("unexpected wxRcptControl type") );
         // fall through

      case Recipient_None:
         // nothing to do
         rcptType = Recipient_None;
         break;
   }

   return rcptType;
}

// ----------------------------------------------------------------------------
// wxMainAddressTextCtrl
// ----------------------------------------------------------------------------

void wxMainAddressTextCtrl::OnEnter(wxCommandEvent& event)
{
   if ( GetValue().empty() )
   {
      // let Enter act like TAB and so go to the subject field
      event.Skip();
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
                              _T("ComposeViewNews"),
                              mode,
                              kind,
                              parent
                           );

   cv->SetTemplate(params.templ);
   cv->UpdateTitle();
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
   wxComposeView *cv = CreateComposeView
                       (
                        profile,
                        params,
                        wxComposeView::Mode_News,
                        wxComposeView::Message_Reply
                       );

   CHECK( cv, NULL, _T("failed to create composer for a followup article") );

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
   wxComposeView *cv = CreateComposeView
                       (
                        profile,
                        params,
                        wxComposeView::Mode_Mail,
                        wxComposeView::Message_Reply
                       );

   CHECK( cv, NULL, _T("failed to create composer for a reply message") );

   cv->SetOriginal(original);

   return cv;
}

Composer *
Composer::CreateFwdMessage(const MailFolder::Params& params,
                           Profile *profile,
                           Message *original)
{
   wxComposeView *cv = CreateComposeView
                       (
                        profile,
                        params,
                        wxComposeView::Mode_Mail,
                        wxComposeView::Message_Forward
                       );

   CHECK( cv, NULL, _T("failed to create composer for a forward message") );

   cv->SetOriginal(original);

   return cv;
}

Composer *
Composer::EditMessage(Profile *profile, Message *msg)
{
   CHECK( msg, NULL, _T("no message to edit?") );

   // first, create the composer

   // create dummy params object as we need one for CreateNewMessage()
   MailFolder::Params params(wxEmptyString);

   wxComposeView *cv = (wxComposeView *)CreateNewMessage(params, profile);

   // next, import the message body in it
   cv->InsertMimePart(msg->GetTopMimePart());

   cv->ResetDirty();

   // finally, also import headers
   wxString nameWithCase, value;
   HeaderIterator hdrIter = msg->GetHeaderIterator();
   while ( hdrIter.GetNext(&nameWithCase, &value) )
   {
      const HeaderName name(nameWithCase);
      value = MIME::DecodeHeader(value);

      // test for some standard headers which need special treatment
      if ( name == "Subject" )
         cv->SetSubject(value);
      else if ( name == "From" )
         cv->SetFrom(value);
      else if ( name == "To" )
         cv->AddTo(value);
      else if ( name == "CC" )
         cv->AddCc(value);
      else if ( name == "BCC" )
         cv->AddBcc(value);
      else if ( name == "FCC" )
      {
         // if we have any FCC recipients, they should replace the default ones
         // instead of being appended to them as then the default recipients
         // would magically reappear after postponing a message and resuming it
         cv->AddFcc(_T("none"));

         cv->AddFcc(value);
      }
      else if ( name.CanBeSetByUser() )
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
               if ( wxSscanf(value, GEOMETRY_FORMAT, &x, &y, &w, &h) == 4 )
               {
                  frame->SetSize(x, y, w, h);
               }
               else // bad header format
               {
                  wxLogDebug(_T("Corrupted ") HEADER_GEOMETRY _T(" header '%s'."),
                             value);
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

/* static */
Composer *Composer::CheckForExistingReply(Message *original)
{
   CHECK( original, NULL, _T("original message is NULL") );

   for ( ComposerList::iterator i = gs_listOfAllComposers.begin(),
                              end = gs_listOfAllComposers.end(); i!= end ; ++i )
   {
      wxComposeView *cv = *i;
      if ( cv->IsReplyTo(*original) )
      {
         if ( !MDialog_YesNoDialog
               (
                  _("There is already an existing composer window for this\n"
                    "message, do you want to reuse it?"),
                  cv->GetFrame(),
                  MDIALOG_YESNOTITLE,
                  M_DLG_YES_DEFAULT | M_DLG_DISABLE,
                  M_MSGBOX_OPEN_ANOTHER_COMPOSER
               ) )
         {
            // if the user doesn't want to reuse this one, he probably really
            // wants a new composer so do open it without asking again
            break;
         }

         // show the existing composer window
         cv->GetFrame()->Raise();

         return cv;
      }
   }

   return NULL;
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
   SetIcon(ICON("ComposeFrame"));

   gs_listOfAllComposers.push_back(this);

   m_mode = mode;
   m_kind = kind;
   m_pidEditor = 0;
   m_procExtEdit = NULL;
   m_alreadyExtEdited =
   m_closeAfterSending =
   m_closing = false;
   m_customTemplate = false;
   m_OriginalMessage = NULL;
   m_DraftMessage = NULL;

   m_msgBeingSent = NULL;

   // by default new recipients are "to"
   m_rcptTypeLast = Recipient_To;

   m_isModified = false;
   m_numNewRcpts = 0;

   m_okToConvertOnSend = false;

   m_editor = NULL;
   m_encoding = wxFONTENCODING_SYSTEM;

   m_txtSubject = NULL;

   m_btnIsReply = NULL;
   m_btnPGPSign = NULL;
}

bool wxComposeView::IsReplyTo(const Message& original) const
{
   if ( !m_OriginalMessage )
      return false;

   // can't compose the messages by pointer, so do a "deep comparison" instead

   // first do quick UIDs comparison
   if ( m_OriginalMessage->GetUId() != original.GetUId() )
      return false;

   // for them to be really the same messages, UIDs must belong to the same
   // folder
   MailFolder *mf1 = original.GetFolder(),
              *mf2 = m_OriginalMessage->GetFolder();
   CHECK( mf1 && mf2, false, _T("messages without mail folder?") );

   return mf1->GetName() == mf2->GetName();
}

void wxComposeView::SetOriginal(Message *original)
{
   CHECK_RET( original, _T("no original message in composer") );

   m_OriginalMessage = original;
   m_OriginalMessage->IncRef();

   // enable "Paste Quoted" command now that we have message to get the
   // attribution from
   GetMenuBar()->Enable(WXMENU_EDIT_PASTE_QUOTED, true);

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
   if ( m_msgBeingSent )
   {
      FAIL_MSG( "shouldn't be in process of sending a message yet" );

      // at least avoid the memory leaks
      delete m_msgBeingSent;
      m_msgBeingSent = NULL;
   }

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

   // clean up the autosave file: if m_filenameAutoSave is set we must have it,
   // otherwise AutoSave() wouldn't have initialized it
   if ( !m_filenameAutoSave.empty() )
   {
      if ( !wxRemoveFile(m_filenameAutoSave) )
      {
         wxLogSysError(_("Failed to remove stale composer autosave file '%s'"),
                       m_filenameAutoSave);
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
   AddViewMenu();
   AddHelpMenu();

   GetMenuBar()->Enable(WXMENU_EDIT_PASTE_QUOTED, m_OriginalMessage != NULL);

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

void wxComposeView::DoCreateToolBar()
{
   CreateMToolbar(this, WXFRAME_COMPOSE);
}

void wxComposeView::DoCreateStatusBar()
{
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

// small helper class for labels created in CreateHeaderFields()
class RightAlignedLabel : public wxStaticText
{
public:
   RightAlignedLabel(wxWindow *parent, const wxString& label)
      : wxStaticText(parent, wxID_ANY, label,
                     wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT)
   {
   }
};

wxSizer *wxComposeView::CreateHeaderFields()
{
   // top level vertical (box) sizer
   wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);

   const wxSizerFlags flagsCenter(wxSizerFlags().Align(wxALIGN_CENTRE_VERTICAL));
   const wxSizerFlags flagsLBorder(wxSizerFlags(flagsCenter).Border(wxLEFT));

   // add "From" header if configured to show it
   wxSizer *sizerFrom;
   if ( READ_CONFIG(m_Profile, MP_COMPOSE_SHOW_FROM) )
   {
      sizerFrom = new wxBoxSizer(wxHORIZONTAL);
      sizerFrom->Add(new RightAlignedLabel(m_panel, _("&From:")), flagsCenter);

      m_txtFrom = new wxTextCtrl(m_panel, -1, wxEmptyString);
      sizerFrom->Add(m_txtFrom, wxSizerFlags(flagsLBorder).Proportion(1));
      SetTextAppearance(m_txtFrom);

      wxChoice * const choiceIdent = CreateIdentCombo(m_panel);
      if ( choiceIdent )
      {
         sizerFrom->Add(choiceIdent, flagsLBorder);
      }
      //else: no identities configured

      m_btnPGPSign = new PGPSignButton(this, m_panel);
      sizerFrom->Add(m_btnPGPSign, flagsLBorder);

      sizerTop->Add(sizerFrom, wxSizerFlags().Expand().Border(wxALL & ~wxBOTTOM));
   }
   else // no from line
   {
      sizerFrom = NULL;
   }

   // main recipient line
   m_rcptMain = new wxRcptMainControl(this);
   wxSizer * const sizerRcpt = m_rcptMain->CreateControls(m_panel);
   sizerTop->Add(sizerRcpt, wxSizerFlags().Expand().Border(wxALL & ~wxBOTTOM));

   // the spare space for already entered recipients below: we use an extra
   // sizer because we keep it to add more stuff to it later
   m_panelRecipients = new wxEnhancedPanel(m_panel);

   m_sizerRcpts = new wxBoxSizer(wxVERTICAL);
   CreatePlaceHolder();

   m_panelRecipients->GetCanvas()->SetSizer(m_sizerRcpts);

   // this number is completely arbitrary
   sizerTop->SetItemMinSize(m_panelRecipients, 0, 80);

   sizerTop->Add(m_panelRecipients, wxSizerFlags(1).Expand().Border());


   // subject
   wxSizer * const sizerSubj = new wxBoxSizer(wxHORIZONTAL);
   sizerSubj->Add(new RightAlignedLabel(m_panel, _("&Subject:")), flagsCenter);

   m_txtSubject = new wxSubjectTextCtrl(m_panel, this);
   SetTextAppearance(m_txtSubject);
   sizerSubj->Add(m_txtSubject, wxSizerFlags(flagsLBorder).Proportion(1));

   m_btnIsReply = new IsReplyButton(this, m_panel);
   sizerSubj->Add(m_btnIsReply, flagsLBorder);

   sizerTop->Add(sizerSubj, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));


   // adjust the layout a bit so that things align better
   static const size_t FIRST_ITEM = 0;
   const int widthFirstCol = sizerRcpt->GetItem(FIRST_ITEM)->GetMinSize().x;
   if ( sizerFrom )
      sizerFrom->SetItemMinSize(FIRST_ITEM, widthFirstCol, -1);
   sizerSubj->SetItemMinSize(FIRST_ITEM, widthFirstCol, -1);

   // arrange the tab order to be more convenient by putting the most useful
   // controls first
   m_txtSubject->MoveAfterInTabOrder(m_rcptMain->GetText());

   return sizerTop;
}



//
// wxDataObjectCompositeEx: written by Gunnar Roth <gunnar.roth <at> gmx.de>
// See http://article.gmane.org/gmane.comp.lib.wxwidgets.general/22384
//
// The problem with the wxDataObjectComposite is that it is not possible
// to know which wxDataObject did receive the data...  Let's store it.
//
class wxDataObjectCompositeEx : public wxDataObjectComposite
{
private:
  wxDataObjectSimple *m_dataObjectLast;

public:
  wxDataObjectCompositeEx()
  {
    m_dataObjectLast = NULL;
  }

  bool SetData(const wxDataFormat& format, size_t len, const void *buf)
  {
    m_dataObjectLast = GetObject(format);
    wxCHECK_MSG( m_dataObjectLast, FALSE, wxT("unsupported format in wxDataObjectCompositeEx"));
    return m_dataObjectLast->SetData(len, buf);
  }

  wxDataObjectSimple *GetActualDataObject()
  {
    return m_dataObjectLast;
  }
};

#if wxUSE_DRAG_AND_DROP

class wxComposeViewDropTarget : public wxDropTarget
{
private:
  wxComposeView* m_composeView;
  wxFileDataObject* m_fileDataObject;
  MMessagesDataObject* m_messageDataObject;

public:
  wxComposeViewDropTarget(wxComposeView* composeView)
    : wxDropTarget()
    , m_composeView(composeView)
    , m_fileDataObject(new wxFileDataObject())
    , m_messageDataObject(new MMessagesDataObject())
  {
    wxDataObjectComposite* dataObjectComposite = new wxDataObjectCompositeEx();
    dataObjectComposite->Add(m_messageDataObject);
    dataObjectComposite->Add(m_fileDataObject);
    SetDataObject(dataObjectComposite);
  }

  virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) {
    if ( !GetData() )
    {
      wxLogDebug(_T("Failed to get drag and drop data"));
      return wxDragNone;
    }

    // Let's see which wxDataObject got the data...
    wxDataObjectSimple* dobj = ((wxDataObjectCompositeEx*)GetDataObject())->GetActualDataObject();
    if ( dobj == (wxDataObjectSimple *)m_messageDataObject )
    {
      // We got messages dropped (from inside M itself)
      return OnDropMessages(x, y, m_messageDataObject->GetFolder(), m_messageDataObject->GetMessages()) ? def : wxDragNone;
    }
    else if ( dobj == m_fileDataObject )
    {
      // We got files dropped from outside M
      return OnDropFiles(x, y, m_fileDataObject->GetFilenames()) ? def : wxDragNone;
    }
    return wxDragNone;
  }


  bool OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames) {
    size_t countFiles = filenames.GetCount();
    if ( countFiles <= 0 )
    {
      return false;
    }
    for ( size_t nFiles = 0; nFiles < countFiles; nFiles++ )
    {
      m_composeView->InsertFile(filenames[nFiles]);
    }
    return true;
  }

  bool OnDropMessages(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), MailFolder* folder, const UIdArray& messages) {
    if ( messages.Count() <= 0 )
    {
      return false;
    }
    for ( size_t i = 0; i < messages.Count(); i++ )
    {
      Message_obj msg(folder->GetMessage(messages[i]));
      wxString str;
      msg->WriteToString(str);
      m_composeView->InsertData(wxStrdup(str), str.Length(), _T("message/rfc822"), wxEmptyString);
    }
    return true;
  }
};

#endif // wxUSE_DRAG_AND_DROP

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

   // and tool/status bars if needed
   CreateToolAndStatusBars();

#if wxUSE_DRAG_AND_DROP
   // Create the wxDropTarget subclass that allows to
   // drop files or messages in the Composer window
   SetDropTarget(new wxComposeViewDropTarget(this));
#endif // wxUSE_DRAG_AND_DROP

   // create the child controls

   // the panel which fills all the frame client area and contains all children
   // (we use it to make the tab navigation work)
   m_splitter = new wxPSplitterWindow(_T("ComposeSplit"), this, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_NO_XP_THEME);

   m_panel = new wxPanel(m_splitter, -1);

   // the sizer containing all header fields
   wxSizer * const sizerHeaders = CreateHeaderFields();

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
   Layout();
   ShowInInitialState();

   // initialize the controls
   // -----------------------

   // set def values for all headers
   SetDefaultFrom();

   // do it in the reverse order so that the most important header (To) is
   // added last and so appears first/top-most in the UI
   if ( READ_CONFIG(m_Profile, MP_USEOUTGOINGFOLDER) )
      AddFcc(READ_CONFIG_TEXT(m_Profile, MP_OUTGOINGFOLDER));

   AddBcc(READ_CONFIG(m_Profile, MP_COMPOSE_BCC));
   AddCc(READ_CONFIG(m_Profile, MP_COMPOSE_CC));

   // it doesn't make sense to use the default recipient when replying (or if
   // it does, we need a separate option for this because it's definitely
   // inconvenient by default)
   if ( m_kind == Message_New )
   {
      AddTo(READ_CONFIG(m_Profile, MP_COMPOSE_TO));

      // if the user selected something that looks like an email address before
      // opening the composer, chances are that he intended to use this address
      // as a recipient so pre-fill the main control with this text but also
      // preselect it to make it possible to easily overwrite it if we guessed
      // wrong
      const String address = TryToGetAddressFromClipboard();
      if ( !address.empty() )
      {
         wxTextCtrl * const text = m_rcptMain->GetText();
         CHECK_RET( text, "must have the main recipient text control" );

         text->SetValue(address);
         text->SelectAll();
      }
   }
}

void
wxComposeView::CreateEditor()
{
   wxASSERT_MSG( m_editor == NULL, _T("creating the editor twice?") );

   MessageEditor *editor = NULL;

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
      String editorname = _T("BareBonesEditor");

      MModule *editorFactory = MModule::LoadModule(editorname);
      if ( !editorFactory ) // failed to load the configured editor
      {
         // try any other
         String nameFirst = (*listing)[0].GetName();

         if ( editorname != nameFirst )
         {
            wxLogWarning(_("Failed to load the configured message editor '%s'.\n"
                           "\n"
                           "Reverting to the default message editor."),
                         editorname);

            editorFactory = MModule::LoadModule(nameFirst);
         }

         if ( !editorFactory )
         {
            wxLogError(_("Failed to load the default message editor '%s'.\n"
                         "\n"
                         "Builtin message editing will not work!"),
                       nameFirst);
         }
      }

      if ( editorFactory )
      {
         editor = ((MessageEditorFactory *)editorFactory)->Create();
         editorFactory->DecRef();
      }

      listing->DecRef();
   }

   // TODO: have a DummyEditor as we have a DummyViewer to avoid crashing
   //       if we failed to create any editor?
   editor->Create(this, m_splitter);

   // Commit
   m_editor = editor;
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
      wxFont font(m_options.GetFont());
      if ( font.Ok() )
         text->SetFont(font);

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
   SetEncoding(wxFONTENCODING_DEFAULT);

   // we're not modified [any more]
   ResetDirty();
}

void wxComposeView::UpdateTitle()
{
   String title(_("Composer"));
   if ( m_txtSubject && !m_txtSubject->IsEmpty() )
   {
      title << _(" - ") << m_txtSubject->GetValue();
   }

   if ( IsExternalEditorRunning() )
      title << _(" (frozen: external editor running)");

   wxFrame *frame = GetFrame();
   CHECK_RET( frame, _T("composer without frame?") );
   if ( title != frame->GetTitle() )
      frame->SetTitle(title);
}

// ----------------------------------------------------------------------------
// wxComposeView address headers stuff
// ----------------------------------------------------------------------------

void
wxComposeView::AddRecipientControls(const String& value, RecipientType rt)
{
   // remove the place holder we had there before
   if ( m_rcptExtra.IsEmpty() )
   {
      DeletePlaceHolder();
   }

   if ( !m_numNewRcpts )
   {
      // before adding a new receipient control, prevent the updates: this is
      // especially useful when many recepients are added at once as is the
      // case when replying to a message with a lot of addressees, as we avoid
      // a lot of flicker
      Freeze();
   }

   // create the controls and add them to the sizer
   wxRcptExtraControl *rcpt = new wxRcptExtraControl(this);

   wxSizer *sizerRcpt = rcpt->CreateControls(m_panelRecipients->GetCanvas());

   rcpt->InitControls(value, rt);

   // insert the control in the beginning, like this the controls inserted
   // later stay visible even if the controls added earlier might be scrolled
   // off
   m_rcptExtra.Insert(rcpt, 0);

   m_sizerRcpts->Prepend(sizerRcpt, wxSizerFlags().Expand());

   // hide the newly added controls for now and set the flag telling our
   // OnIdle() to show it, after laying it out, later
   m_sizerRcpts->Hide(sizerRcpt);
   m_numNewRcpts++;

   // adjust the indexes of all the existing controls after adding a new one
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
wxComposeView::AddRecipients(const String& addressOrig,
                             RecipientType addrType,
                             int flags)
{
   // if there is no address, nothing to do
   if ( addressOrig.empty() )
      return;

   String address = addressOrig;

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
               groups = wxStringTokenize(address, _T(",; \t"), wxTOKEN_STRTOK);

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
         if ( flags & AddRcpt_Expand )
         {
            String subject;
            AdbExpandSingleAddress(&address, &subject, GetProfile(), this);

            if ( !subject.empty() )
               SetSubject(subject);
         }

         AddRecipient(address, addrType);
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
      if ( Address::Compare(m_rcptExtra[n]->GetValue(), addr) )
      {
         // ok, we already have this address - is it of the same type?
         RecipientType existingType = m_rcptExtra[n]->GetType();
         if ( existingType == addrType )
         {
            // yes, don't add it again
            wxLogStatus(this,
                        _("Address '%s' is already in the recipients list, "
                          "not added."),
                        addr);
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
                           addr);

               m_rcptExtra[n]->SetType(addrType);
            }
            else
            {
               wxLogStatus(this,
                           _("Address '%s' was already in the recipients list "
                             "with a different type, not added."),
                           addr);
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

void
wxComposeView::SaveRecipientsAddresses(MAction collect,
                                       const String& book,
                                       const String& group)
{
   if ( collect == M_ACTION_NEVER )
      return;

   static RecipientType types[] = { Recipient_To, Recipient_Cc, Recipient_Bcc };

   const bool namedOnly = READ_CONFIG_BOOL(m_Profile, MP_AUTOCOLLECT_NAMED);

   for ( size_t n = 0; n < WXSIZEOF(types); ++n )
   {
      wxArrayString list;
      GetRecipients(types[n], list);

      for ( size_t address = 0; address < list.GetCount(); ++address )
      {
         RefCounter<AddressList> parser(AddressList::Create(list[address]));

         for ( Address *field = parser->GetFirst();
               field;
               field = parser->GetNext(field) )
         {
            AutoCollectAddress
            (
               field->GetEMail(),
               field->GetName(),
               collect,
               namedOnly,
               book,
               group
            );
         }
      }
   }
}

// helper of GetRecipients(): add the address from this control if it is of
// correct type, to the address array
static void
GetRecipientFromControl(RecipientType type,
                        wxRcptControl *rcpt,
                        wxArrayString& list)
{
   if ( rcpt->GetType() == type )
   {
      wxString value = rcpt->GetValue();
      if ( !value.empty() )
      {
         list.Add(value);
      }
   }
}

void wxComposeView::GetRecipients(RecipientType type, wxArrayString& list) const
{
   list.Empty();

   GetRecipientFromControl(type, m_rcptMain, list);

   size_t count = m_rcptExtra.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      GetRecipientFromControl(type, m_rcptExtra[n], list);
   }
}

String wxComposeView::GetRecipients(RecipientType type) const
{
   wxArrayString list;
   GetRecipients(type, list);

   String address;

   size_t count = list.GetCount();
   if( count > 0 )
   {
      address += list[0];
      for ( size_t n = 1; n < count; n++ )
      {
         address += CANONIC_ADDRESS_SEPARATOR;
         address += list[n];
      }
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
   if ( m_kind == Message_New )
   {
      // writing a new message/article: wait until the headers are filled
      // before evaluating the template
      if ( m_template.empty() )
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

      // we need to show text in the message view to be able to quote it
      if ( msg && msgview )
      {
         const_cast<MessageView *>(msgview)->ShowMessage(msg->GetUId());

         // needed for the message to appear in the message view (FIXME)
         MEventManager::DispatchPending();

         // remember the text to quote once and for all, so that we still quote
         // the same text even if the message view selection changes later (and
         // we're reinitialized because, e.g., of an identity change)
         m_textToQuote = msgview->GetText();
      }

      DoInitText(msg);
   }
}

void wxComposeView::Launch()
{
   // we also use this method to initialize the focus as we can't do it before
   // the composer text is initialized

   // the natural order is to enter recipients first and then the subject but
   // if we already have some recipients (e.g. because we're replying to an
   // existing message or because there are some default recipients at the
   // folder level even for the new messages) we give focus to the subject
   // first and, finally, if the subject is also specified (as happens when
   // replying or forwarding) we go directly to the composer

   // notice that CC, BCC and FCC shouldn't count: even if we have them, we
   // still need at least one recipient (usually)
   const size_t numRcpts = m_rcptExtra.size();
   for ( size_t n = 0; n < numRcpts; n++ )
   {
      switch ( m_rcptExtra[n]->GetType() )
      {
         case Recipient_To:
         case Recipient_Newsgroup:
            // we already have a recipient, go to the subject field
            if ( m_txtSubject->GetValue().empty() )
               m_txtSubject->SetFocus();
            else // or directly to the composer if we already have subject too
               SetFocusToComposer();

            return; // skip SetFocus() call below

         default:
            ; // suppress gcc warning
      }
   }

   // no recipients yet, set the focus there
   m_rcptMain->GetText()->SetFocus();
}

void
wxComposeView::DoInitText(Message *msgOrig)
{
   // position the cursor correctly and separate us from the previous message
   // if we're replying to several messages at once
   if ( !IsEmpty() )
   {
      InsertText(_T("\n"));
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
      String templateValue = m_template.empty()
                                 ? GetMessageTemplate(kind, m_Profile)
                                 : m_template;

      if ( templateValue.empty() )
      {
         // if there is no template just don't do anything
         break;
      }

      if ( m_textToQuote.empty() )
      {
         // we don't want to quote anything at all, so remove all occurrences of
         // $QUOTE and/or $TEXT templates in the string -- and also remove
         // anything preceding them assuming that it can only be the
         // attribution line which shouldn't be left if we don't quote anything
         //
         // this is surely not ideal, but I don't see how to do what we want
         // otherwise with the existing code

         const wxChar *pcEnd = NULL;
         const wxChar *pcStart = templateValue.c_str();
         for ( const wxChar *pc = pcStart; ; )
         {
            // find and skip over the next macro occurrence
            pc = wxStrchr(pc, _T('$'));
            if ( !pc++ )
               break;

            static const wxChar *QUOTE = _T("quote");
            static const size_t LEN_QUOTE = wxStrlen(QUOTE);

            static const wxChar *QUOTE822 = _T("quote822");
            static const size_t LEN_QUOTE822 = wxStrlen(QUOTE822);

            static const wxChar *TEXT = _T("text");
            static const size_t LEN_TEXT = wxStrlen(TEXT);

            if ( wxStrnicmp(pc, QUOTE, LEN_QUOTE) == 0 )
            {
               // don't erroneously remove $quote822
               if ( wxStrnicmp(pc, QUOTE822, LEN_QUOTE822) != 0 )
               {
                  pc += LEN_QUOTE;
                  pcEnd = pc;
               }
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
               m_textToQuote
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

            if ( m_template.empty() )
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
                          filename);
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
               pattern << _("vCard files (*.vcf)|*.vcf") << _T('|') << wxGetTranslation(wxALL_FILES);
               filename = wxPLoadExistingFileSelector
                          (
                              this,
                              "vcard",
                              _("Choose vCard file"),
                              NULL, _T("vcard.vcf"), NULL,
                              pattern
                          );
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
         InsertData(wxStrdup(vcard), vcard.length(), _T("text/x-vcard"), filename);
      }
   }

   // the text hasn't been modified by the user
   ResetDirty();
}

void wxComposeView::OnIdle(wxIdleEvent& event)
{
   if ( m_numNewRcpts )
   {
      for ( size_t n = 0; n < m_numNewRcpts; n++ )
         m_sizerRcpts->Show(n);

      m_sizerRcpts->Layout();
      m_panelRecipients->RefreshScrollbar(m_panelRecipients->GetClientSize());

      m_numNewRcpts = 0;

      // now show all the recepient controls
      Thaw();
   }

   event.Skip();
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

void
wxComposeView::SetEncoding(wxFontEncoding encoding, wxFontEncoding encConv)
{
   // use the default encoding if no explicit one specified
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


   // find the encoding for the editor: it must be one supported by the system
   if ( encConv == wxFONTENCODING_SYSTEM )
   {
      encConv = encoding;
#if !wxUSE_UNICODE
      if ( !EnsureAvailableTextEncoding(&encConv) )
         encConv = wxFONTENCODING_SYSTEM;
#endif // !wxUSE_UNICODE
   }

   m_editor->SetEncoding(encConv == wxFONTENCODING_SYSTEM ? encoding : encConv);


   // check "Default" menu item if we use the system default encoding in absence
   // of any user-configured default
   CheckLanguageInMenu(this, m_encoding == wxFONTENCODING_SYSTEM
                                             ? wxFONTENCODING_DEFAULT
                                             : m_encoding);
}

bool wxComposeView::SetEncodingToSameAs(const MimePart *part)
{
   while ( part )
   {
      if ( part->GetType().GetPrimary() == MimeType::TEXT )
      {
         wxFontEncoding enc = part->GetTextEncoding();
         if ( enc != wxFONTENCODING_SYSTEM )
         {
            // Unicode parts are converted by the message viewer (from which we
            // get our text) to another encoding, get it
            wxFontEncoding encConv;
#if !wxUSE_UNICODE
            if ( enc == wxFONTENCODING_UTF7 || enc == wxFONTENCODING_UTF8 )
            {
               String textPart(part->GetTextContent());
               encConv = ConvertUTFToMB(&textPart, enc);
            }
            else // not Unicode
#endif // !wxUSE_UNICODE
            {
               encConv = wxFONTENCODING_SYSTEM;
            }

            SetEncoding(enc, encConv);
            return true;
         }
      }

      if ( SetEncodingToSameAs(part->GetNested()) )
         return true;

      part = part->GetNext();
   }

   return false;
}

void wxComposeView::SetEncodingToSameAs(const Message *msg)
{
   CHECK_RET( msg, _T("no message in SetEncodingToSameAs") );

   // find the first text part with non default encoding
   SetEncodingToSameAs(msg->GetTopMimePart());
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
         // preserve the original encoding as DoClear() resets it
         wxFontEncoding encodingOrig = m_encoding;

         DoClear();

         // forget previously read template if we read it from profile, it
         // could have changed
         if ( !m_customTemplate )
            m_template.clear();

         if ( encodingOrig != wxFONTENCODING_DEFAULT &&
                  encodingOrig != wxFONTENCODING_SYSTEM )
         {
            SetEncoding(encodingOrig);
         }

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

   if ( m_msgBeingSent )
   {
      // we can't close the window while we're still in process of sending a
      // message
      //
      // TODO: allow cancelling the sending thread
      wxLogError(_("Please wait until the message is sent."));

      canClose = false;
   }
   else if ( m_procExtEdit )
   {
      // we can't close while the external editor is running
      //
      // TODO: allow doing it and just delete the editor temporary file later?
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
         LetUserAddAttachment();
         break;


      case WXMENU_COMPOSE_SEND:
      case WXMENU_COMPOSE_SEND_NOW:
      case WXMENU_COMPOSE_SEND_KEEP_OPEN:
      case WXMENU_COMPOSE_SEND_LATER:
         if ( IsReadyToSend() )
         {
            // By default we close the window after sending the message that
            // was edited in it but the user can explicitly choose to keep the
            // window opened
            m_closeAfterSending = id != WXMENU_COMPOSE_SEND_KEEP_OPEN;

            SendMode mode;
            if ( id == WXMENU_COMPOSE_SEND_LATER )
               mode = SendMode_Later;
            else if ( id == WXMENU_COMPOSE_SEND_NOW )
               mode = SendMode_Now;
            else
               mode = SendMode_Default;

            // Save the composer contents before sending it: this is really
            // just a workaround for the design deficiency of c-client which
            // prevents us from both sending the message and saving it in the
            // same time from multiple threads (because both of these actions
            // need to instantiate Rfc822OutputRedirector in SendMessageCC.cpp)
            // but it could be argued that it makes sense as like this we'll
            // never lose the final version of the message.
            AutoSave();

            Send(mode);
         }
         break;

      case WXMENU_COMPOSE_SAVE_AS_DRAFT:
         if ( SaveAsDraft() )
         {
            Close();
         }
         break;

      case WXMENU_COMPOSE_PREVIEW:
         if(m_editor->FinishWork())
         {
            SendMessage_obj msg(BuildMessage());
            if ( !msg )
            {
               wxLogError(_("Failed to create the message to preview."));
            }
            else
            {
               msg->Preview();
            }
         }
         break;

      case WXMENU_COMPOSE_PRINT:
         Print();
         break;

      case WXMENU_COMPOSE_CLEAR:
         if ( !IsModified() ||
                 MDialog_YesNoCancel
                 (
                     _("Are you sure you want to clear this window?\n"
                       "\n"
                       "Please notice that it is currently impossible to\n"
                       "undo this operation."),
                     this,
                     MDIALOG_YESNOTITLE,
                     M_DLG_NO_DEFAULT
                 ) == MDlg_Yes )
         {
            DoClear();
         }
         //else: cancelled
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
            String filename = wxPLoadExistingFileSelector
                              (
                               this,
                               _T("MsgInsertText"),
                               _("Please choose a file to insert."),
                               NULL, _T("dead.letter"), NULL
                              );

            if ( filename.empty() )
            {
               wxLogStatus(this, _("Cancelled"));
            }
            else if ( InsertFileAsText(filename, MessageEditor::Insert_Append) )
            {
               wxLogStatus(this, _("Inserted file '%s'."), filename);
            }
            else
            {
               wxLogError(_("Failed to insert the text file '%s'."), filename);
            }
         }
         break;

      case WXMENU_COMPOSE_SAVETEXT:
         if(m_editor->FinishWork())
         {
            String filename = wxPSaveFileSelector
                              (
                               this,
                               _T("MsgSaveText"),
                               _("Choose file to append message to"),
                               NULL, _T("dead.letter"), NULL
                              );

            if ( filename.empty() )
            {
               wxLogStatus(this, _("Cancelled"));
            }
            else if ( SaveMsgTextToFile(filename) )
            {
               // we have been saved
               ResetDirty();

               wxLogStatus(this, _("Message text saved to file '%s'."), filename);
            }
            else
            {
               wxLogError(_("Failed to save the message."));
            }
         }
         break;

      case WXMENU_COMPOSE_EXTEDIT:
         if(m_editor->FinishWork())
            StartExternalEditor();
         break;

      case WXMENU_COMPOSE_IN_REPLY_TO:
         if ( ConfigureInReplyTo() )
         {
            CHECK_RET( m_btnIsReply, "should have the button" );
            m_btnIsReply->Update();
         }
         break;

      case WXMENU_COMPOSE_PGP_SIGN:
         TogglePGPSigning();

         CHECK_RET( m_btnPGPSign, "should have the button" );
         m_btnPGPSign->Update();
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
                           headerName);
            }
            //else: cancelled
         }
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(m_mode == Mode_Mail ? MH_COMPOSE_MAIL
                                                : MH_COMPOSE_NEWS, this);
         break;

      case WXMENU_EDIT_PASTE:
         m_editor->Paste();
         break;

      case WXMENU_EDIT_PASTE_QUOTED:
         {
            wxClipboardLocker clipboardLock;
            if ( !clipboardLock )
               break;

            wxTextDataObject data;
            if ( wxTheClipboard->GetData(data) )
            {
               m_editor->InsertText(QuoteText(data.GetText(),
                                              m_Profile,
                                              m_OriginalMessage
                                             ),
                                    MessageEditor::Insert_Insert);
            }
         }
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
   if ( m_msgBeingSent )
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
#if wxCHECK_VERSION(3, 1, 0)
      // Try to switch to the already running editor automatically.
      if ( m_procExtEdit->Activate() )
      {
         wxLogStatus(_("Switch to the already running editor (PID %d)"),
                     m_pidEditor);
      }
      else
#endif // wx 3.1+
      {
         // And fall back to telling the user to do it manually if this failed.
         wxLogError(_("External editor is already running (PID %d)"),
                    m_pidEditor);
      }

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
         String command = ExpandExternalCommand(extEdit, tmpFileName.GetName());

         // do start the external process
         m_procExtEdit = new wxProcess(this, HelperProcess_Editor);
         m_pidEditor = wxExecute(command, FALSE, m_procExtEdit);

         if ( !m_pidEditor  )
         {
            wxLogError(_("Execution of '%s' failed."), command);
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
            UpdateTitle();

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
         if ( wxRemove(m_tmpFileName) != 0 )
         {
            wxLogDebug(_T("Stale temp file '%s' left."), m_tmpFileName);
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
      wxLogError(_("The text was left in the file '%s'."), m_tmpFileName);
   }

   m_pidEditor = 0;
   m_tmpFileName.Empty();

   delete m_procExtEdit;
   m_procExtEdit = NULL;


   UpdateTitle();

   // show the frame which might had been obscured by the other windows
   Raise();
}

// ----------------------------------------------------------------------------
// inserting stuff into wxComposeView
// ----------------------------------------------------------------------------

// common part of InsertData() and InsertFile()
void
wxComposeView::DoInsertAttachment(EditorContentPart *mc, const wxChar *mimetype)
{
   mc->SetMimeType(mimetype);

   String ext = wxFileName(mc->GetFileName()).GetExt();

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxIcon icon = iconManager->GetIconFromMimeType(mimetype, ext);

   m_editor->InsertAttachment(icon, mc);
}

// insert any data
void
wxComposeView::InsertData(void *data,
                          size_t length,
                          const wxChar *mimetype,
                          const wxChar *name,
                          const wxChar *filename)
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
         mt = _T("APPLICATION/OCTET-STREAM");
      }
   }

   EditorContentPart *mc = new EditorContentPart();
   if ( MimeType(mt).ShouldShowInline() )
      mc->SetDisposition(_T("INLINE"));
   mc->SetData(data, length, name, filename);

   DoInsertAttachment(mc, mimetype);
}

// insert file data
void
wxComposeView::InsertFile(const wxChar *fileName,
                          const wxChar *mimetype,
                          const wxChar *name)
{
   CHECK_RET( !strutil_isempty(fileName), _T("filename can't be empty") );

   // if there is a slash after the dot, it is not extension (otherwise it
   // might be not an extension too, but consider that it is - how can we
   // decide otherwise?)
   String filename(fileName);
   String strExt = filename.AfterLast('.');
   if ( strExt == filename || wxStrchr(strExt, '/') )
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
   if ( !strutil_isempty(name) )
      props.name = name;
   else
      props.name = wxFileNameFromPath(filename);
   props.mimetype = strMimeType;
   mc->SetFile(props.filename);
   mc->SetName(props.name);

   // we use almost, but not quite, the same logic here as for viewing MIME
   // contents: the exception is that while "text" parts are normally shown
   // inline, we attach them as attachments by default because typically this
   // is the user intention, otherwise the text would have just been pasted
   // inline directly
   const MimeType attachmentMimeType(strMimeType);
   props.disposition =
      attachmentMimeType.ShouldShowInline() && !attachmentMimeType.IsText()
         ? AttachmentProperties::Disposition_Inline
         : AttachmentProperties::Disposition_Attachment;
   mc->SetDisposition(props.GetDisposition());

   // show the attachment properties dialog automatically?
   String configPath = GetPersMsgBoxName(M_MSGBOX_MIME_TYPE_CORRECT);
   if ( !wxPMessageBoxIsDisabled(configPath) )
   {
      bool dontShowAgain = false;

      if ( !ShowAttachmentDialog(m_editor->GetWindow(),
                                 &props,
                                 dontShowAgain) )
      {
         // adding the attachment was cancelled
         wxLogStatus(this, _("Attachment not added."));

         return;
      }

      mc->SetName(props.name);
      mc->SetDisposition(props.GetDisposition());

      if ( dontShowAgain )
         wxPMessageBoxDisable(configPath, wxNO);
   }


   strMimeType = props.mimetype.GetFull();
   DoInsertAttachment(mc, strMimeType);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"), filename, strMimeType);

}

/// insert a text file at the current cursor position
bool
wxComposeView::InsertFileAsText(const String& filename,
                                MessageEditor::InsertMode insMode)
{
   // read the text from the file
   wxString text;
   wxFile file(filename);

   bool ok = file.IsOpened();
   if ( ok )
   {
      const wxFileOffset lenFile = file.Length();
      if ( lenFile == 0 )
      {
         if ( insMode != MessageEditor::Insert_Replace )
         {
            wxLogVerbose(_("File '%s' is empty, no text to insert."), filename);
            return true;
         }
         //else: replace old text with new (empty) one
      }
      else if ( lenFile != wxInvalidOffset ) // non empty file
      {
         wxCharBuffer buf(lenFile);
         ok = file.Read(buf.data(), lenFile) != wxInvalidOffset;

         if ( ok )
         {
            buf.data()[lenFile] = '\0';
#if wxUSE_UNICODE
            // try to interpret the file contents as Unicode
            text = wxString(buf, wxConvAuto());
            if ( text.empty() )
#endif // wxUSE_UNICODE
            {
               // this works whether m_encoding == wxFONTENCODING_SYSTEM (we
               // use the current locale encoding then) or not (in which case
               // we use the specified encoding)
               text = wxString(buf, wxCSConv(m_encoding));
            }
         }
      }
      else // lenFile == wxInvalidOffset
      {
         ok = false;
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

void
wxComposeView::InsertText(const String &text)
{
   // the text here may come from a file and so can be in an encoding different
   // from the one we currently use, but we -- unfortunately -- have no way of
   // knowing about it, except in the special case when we use UTF-8 and then
   // we must check if text is a valid UTF-8 string as otherwise inserting it
   // is going to fail
   String textCopy;
   if ( wxLocale::GetSystemEncoding() == wxFONTENCODING_UTF8 )
   {
      if ( wxConvUTF8.MB2WC(NULL, text, 0) == (size_t)-1 )
      {
         // not a valid UTF-8 string, must suppose it's in some other encoding
         // and as we have no idea about what it is, choose latin1 as the most
         // common (among Mahogany users, anyhow)
         if ( m_encoding == wxFONTENCODING_SYSTEM )
         {
            // change the encoding to latin1 if none explicitly specified
            SetEncoding(wxFONTENCODING_ISO8859_1);
         }
         else // we already have an existing encoding
         {
            // transform the text from latin1 to the current encoding
            textCopy = wxCSConv(m_encoding).cWC2MB(wxConvISO8859_1.cMB2WC(text));
         }
      }
   }

   m_editor->InsertText(textCopy.empty() ? text : textCopy,
                        MessageEditor::Insert_Append);
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
            if ( subtype == _T("ALTERNATIVE") )
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

namespace
{

// Helper class taking care of creating an additional text control in
// wxFileDialog and returning its contents.
class AttachNamesInFileDialog
{
public:
   explicit AttachNamesInFileDialog(wxFileDialog& dialog)
   {
      ms_attachName = this;

      m_text = NULL;

      dialog.SetExtraControlCreator(Create);
   }

   static wxWindow* Create(wxWindow* parent)
   {
      wxCHECK( ms_attachName, NULL );

      return ms_attachName->DoCreate(parent);
   }

   virtual ~AttachNamesInFileDialog()
   {
      ms_attachName = NULL;
   }

   wxArrayString GetNames() const
   {
      return wxSplit(m_names, ' ');
   }

private:
   wxWindow *DoCreate(wxWindow* parent)
   {
      wxPanel* const panel = new wxPanel(parent);

      // Currently we just use a single text control. This is fine for a single
      // attachment but not so great for multiple ones (when the user is
      // supposed to enter space-separated names manually and use backslash to
      // escape any spaces inside the names), however it's not obvious how
      // could this be improved as we don't get any notifications about the
      // change in the number of the selected files from wx.

      wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

#ifdef OS_WIN
      // At least under MSW things look better indented a bit.
      sizer->AddSpacer(10);
#endif // OS_WIN

      sizer->Add(new wxStaticText(panel, wxID_ANY,
                                  "Optional attachment name overriding file name:"),
                 wxSizerFlags().Centre().Border());

      m_text = new wxTextCtrl(panel, wxID_ANY, wxString(),
                              wxDefaultPosition,
                              wxSize(40*panel->GetCharWidth(), -1));
      m_text->SetHint(_("attachment name in the message"));

      sizer->Add(m_text, wxSizerFlags(1).Centre().Border());

      panel->SetSizerAndFit(sizer);

      // We need to bind to this event to update m_names immediately as m_text
      // can't be used any more once the dialog is closed.
      m_text->Bind(wxEVT_TEXT,
                   [=](wxCommandEvent&) { m_names = m_text->GetValue(); });

      return panel;
   }

   wxTextCtrl* m_text;
   wxString m_names;

   // The unique (because we can only be used from the main thread and the file
   // dialog is not reentrant) object being currently used.
   static AttachNamesInFileDialog* ms_attachName;
};

AttachNamesInFileDialog* AttachNamesInFileDialog::ms_attachName = NULL;

} // anonymous namespace

void wxComposeView::LetUserAddAttachment()
{
   // Read the name of the directory used the last time. The name of the last
   // attached file doesn't seem to be important as it would be rarely useful
   // to attach the same file twice -- but it is common to attach files from
   // the same directory as the last time.
   static const char* const SETTINGS_PATH = "/Settings/FilePrompts";
   static const char* const ATTACHMENT_DIR_KEY = "MsgInsertPath";

   wxConfigBase* const config = AllConfigSources::Get().GetLocalConfig();
   wxCHECK_RET( config, wxS("no local config?") );

   // Try to use the value specific to the associated folder, falling back to
   // its parent recursively as usual.
   const wxString folderPath = m_Profile->GetFolderName();
   config->SetPath(wxString::Format("%s/%s", SETTINGS_PATH, folderPath));

   wxString dir;
   for ( ;; )
   {
      if ( config->Read(ATTACHMENT_DIR_KEY, &dir) )
         break;

      if ( config->GetPath() == SETTINGS_PATH )
         break;

      config->SetPath("..");
   }


   // Now show the dialog to the user.
   wxFileDialog dialog
                (
                  this,
                  _("Please choose files to insert."),
                  dir, wxString(),
                  wxGetTranslation(wxALL_FILES),
                  wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST
                );

   AttachNamesInFileDialog attachNames(dialog);

   if ( dialog.ShowModal() != wxID_OK )
   {
      // Dialog cancelled by user.
      return;
   }

   config->Write(wxString::Format
                           (
                              "%s/%s/%s",
                              SETTINGS_PATH, folderPath, ATTACHMENT_DIR_KEY
                           ),
                 dialog.GetDirectory());


   // Finally do attach the files.
   wxArrayString filenames;
   dialog.GetPaths(filenames);

   const wxArrayString names = attachNames.GetNames();

   const size_t nFiles = filenames.size();
   for ( size_t n = 0; n < nFiles; n++ )
   {
      InsertFile(filenames[n], NULL, n < names.size() ? names[n] : wxString());
   }
}

// ----------------------------------------------------------------------------
// wxComposeView sending the message
// ----------------------------------------------------------------------------

bool wxComposeView::CheckForForgottenAttachments() const
{
   if ( !READ_CONFIG(m_Profile, MP_CHECK_FORGOTTEN_ATTACHMENTS) )
      return true;

   // check if we have any attachments already as, if we do, we don't have to
   // check if we forgot them (and also retrieve the message text while doing
   // this)
   wxString msgText;
   bool hasAttachments = false;
   for ( EditorContentPart *part = m_editor->GetFirstPart();
         part && !hasAttachments;
         part = m_editor->GetNextPart() )
   {
      switch ( part->GetType() )
      {
         case EditorContentPart::Type_Text:
            msgText += GetUnquotedText(part->GetText(), m_Profile);
            break;

         default:
            hasAttachments = true;
      }

      part->DecRef();
   }

   if ( hasAttachments )
   {
      // no need to check for anything: there is already at least one
      // attachment so apparently the user didn't forget to add it
      return true;
   }

   // wx built-in regex library only understands \< in BREs, not EREs, and
   // insists on using [[:<:]] which is not supported by glibc so we try to
   // make everybody happy at once by convertin one to the other as needed
   String reText = READ_CONFIG_TEXT(m_Profile, MP_CHECK_ATTACHMENTS_REGEX);
#ifdef wxHAS_REGEX_ADVANCED
   // only built-in lib supports AREs
   reText.Replace(_T("\\<"), _T("[[:<:]]"));
   reText.Replace(_T("\\>"), _T("[[:>:]]"));
#endif

   wxRegEx re(reText, wxRE_EXTENDED | wxRE_ICASE);
   if ( !re.IsValid() )
   {
      MDialog_ErrorMessage
      (
         wxString::Format
         (
            _("The regular expression \"%s\" for detecting missing "
              "attachments is invalid and can't be used.\n"
              "Please change it in the program options and reenable "
              "the check for forgotten attachments there\n"
              "as it will be temporarily disabled now."),
            reText
         ),
         this
      );

      // TODO: should write it to the same path from which
      //       MP_CHECK_ATTACHMENTS_REGEX was really read
      m_Profile->writeEntry(MP_CHECK_FORGOTTEN_ATTACHMENTS, false);

      return true;
   }

   if ( !re.Matches(msgText) )
      return true;

   if ( !MDialog_YesNoDialog
         (
            _("You might have intended to attach a file to this "
              "message but you are about to send it without any attachments\n"
              "\n"
              "Would you like to attach anything before sending?"),
            this,
            MDIALOG_YESNOTITLE,
            M_DLG_YES_DEFAULT,
            M_MSGBOX_CHECK_FORGOTTEN_ATTACHMENTS
         ) )
   {
      if ( wxPMessageBoxIsDisabled(
               GetPersMsgBoxName(M_MSGBOX_CHECK_FORGOTTEN_ATTACHMENTS)) )
      {
         // the user doesn't want us to bother him any more, so don't
         mApplication->GetProfile()->
            writeEntry(MP_CHECK_FORGOTTEN_ATTACHMENTS, false);
      }

      return true;
   }

   wx_const_cast(wxComposeView *, this)->LetUserAddAttachment();

   return false;
}

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

   // check if the editor has anything to say
   if ( !m_editor->FinishWork() )
      return false;

   // check if the user didn't forget to attach the files he wanted to attach
   if ( !CheckForForgottenAttachments() )
      return false;

   // everything is ok
   return true;
}

wxCharBuffer
wxComposeView::EncodeText(const wxString& text,
                          wxFontEncoding *encodingPartPtr,
                          int flags) const
{
   wxCHECK_MSG( encodingPartPtr, wxCharBuffer(), "NULL encoding argument" );
   wxFontEncoding& encodingPart = *encodingPartPtr;

   if ( encodingPart == wxFONTENCODING_SYSTEM )
      encodingPart = wxLocale::GetSystemEncoding();

   // empty text doesn't need any conversion whatsoever and we consider that
   // conversion fails below if it returns empty string as result which is only
   // true if the original text is non-empty in the first place so get rid of
   // this case from the very beginning
   if ( text.empty() )
      return wxCharBuffer("");

   wxCharBuffer textBuf;

   // the part may be written either in the same encoding as chosen in the
   // "Language" menu or in a different -- but hopefully compatible -- one, in
   // which case we need to translate it
   if ( m_encoding != wxFONTENCODING_SYSTEM &&
          encodingPart != m_encoding )
   {
      // try converting this part to the message encoding
      textBuf = text.mb_str(wxCSConv(m_encoding));
      if ( !textBuf.length() )
      {
         // Before asking whether to convert to the part encoding, check if it
         // can be converted.
         textBuf = text.mb_str(wxCSConv(encodingPart));

         if ( textBuf.length() && (flags & Interactive) && !m_okToConvertOnSend )
         {
            if ( !MDialog_YesNoDialog
                  (
                     wxString::Format
                     (
                        _("Text of this message can't be converted "
                          "to the encoding \"%s\", would you like "
                          " to send it in encoding \"%s\" instead?"),
                        wxFontMapper::GetEncodingName(m_encoding),
                        wxFontMapper::GetEncodingName(encodingPart)
                     ),
                     this,
                     MDIALOG_YESNOTITLE,
                     M_DLG_YES_DEFAULT,
                     M_MSGBOX_SEND_DIFF_ENCODING
                  ) )
            {
               return wxCharBuffer();
            }

            // don't ask again for this message
            m_okToConvertOnSend = true;
         }
         //else: don't ask, convert by default
      }
      else // successfully converted to our encoding, just use it
      {
         encodingPart = m_encoding;
      }
   }

   if ( !textBuf )
   {
      textBuf = text.mb_str(wxCSConv(encodingPart));
      if ( !textBuf.length() )
      {
         if ( flags & Interactive )
         {
            if ( !MDialog_YesNoDialog
                  (
                     wxString::Format
                     (
                        _("Text of this message can't be converted "
                          "to the encoding \"%s\", would you like "
                          "to send it in UTF-8 instead?"),
                        wxFontMapper::GetEncodingName(encodingPart)
                     ),
                     this,
                     MDIALOG_YESNOTITLE,
                     M_DLG_YES_DEFAULT,
                     M_MSGBOX_SEND_DIFF_ENCODING
                  ) )
            {
               return wxCharBuffer();
            }
         }

         encodingPart = wxFONTENCODING_UTF8;
         textBuf = text.utf8_str();

         // everything is convertible to UTF-8
         ASSERT_MSG( textBuf.length(), "conversion to UTF-8 shouldn't fail" );
      }
   }

   return textBuf;
}

SendMessage *
wxComposeView::BuildMessage(int flags) const
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
   SendMessage_obj msg(SendMessage::Create(m_Profile, proto, ::GetFrame(this)));
   if ( !msg )
   {
      // can't do anything more
      return NULL;
   }

   // compose the body
   // ----------------

   // if we find a part which we can't process (probably because it's an
   // attachment and the corresponding file doesn't exist or can't be read) we
   // still continue with building the message as we want to give the user the
   // errors about all such problematic parts instead of giving the error about
   // the first one, then, after he retries after correcting the problem, about
   // the second one and so forth
   bool allPartsOk = true;
   for ( EditorContentPart *part = m_editor->GetFirstPart();
         part;
         part = m_editor->GetNextPart() )
   {
      switch ( part->GetType() )
      {
         case EditorContentPart::Type_Text:
            {
               wxFontEncoding encodingPart = part->GetEncoding();
               wxCharBuffer
                  textBuf = EncodeText(part->GetText(), &encodingPart, flags);
               if ( !textBuf )
               {
                  // send aborted
                  part->DecRef();
                  return NULL;
               }

               msg->AddPart(
                              MimeType::TEXT,
                              textBuf,
                              textBuf.length(),
                              _T("PLAIN"),
                              _T("INLINE"),  // disposition
                              NULL,          // disposition parameters
                              NULL,          // other parameters
                              encodingPart
                           );
            }
            break;

         case EditorContentPart::Type_File:
            {
               bool partOk = false;

               String filename = part->GetFileName();
               wxFile file;
               if ( file.Open(filename) )
               {
                  size_t size = file.Length();
                  wxChar *buffer = new wxChar[size + 1];
                  if ( file.Read(buffer, size) )
                  {
                     // always NUL terminate it
                     buffer[size] = '\0';

                     // use the user provided name instead of local filename if
                     // it was given
                     String name = part->GetName();
                     if ( name.empty() )
                     {
                        // use only file name, i.e. without path, because the
                        // receiving MUA discards the path anyhow (for obvious
                        // security reasons) and the user might not like that
                        // we show his local paths in outgoing mail messages
                        name = wxFileNameFromPath(filename);
                     }

                     MessageParameterList plist, dlist;
                     MessageParameter *p;

                     // newer mailers look for "FILENAME" in disposition
                     // parameters according to RFC 2183
                     p = new MessageParameter(_T("FILENAME"), name);
                     dlist.push_back(p);

                     // but some old mailers still use "NAME" in content-type
                     // parameters (per obsolete RFC 1521), so put it there as
                     // well
                     p = new MessageParameter(_T("NAME"), name);
                     plist.push_back(p);

                     const MimeType& mt = part->GetMimeType();
                     msg->AddPart
                          (
                            mt.GetPrimary(),
                            buffer, size,
                            mt.GetSubType(),
                            part->GetDisposition(),
                            &dlist,
                            &plist
                          );

                     partOk = true;
                  }
                  else if ( flags & Interactive )
                  {
                     wxLogError(_("Cannot read file '%s' included in "
                                  "this message!"), filename);
                  }

                  delete [] buffer;
               }
               else if ( flags & Interactive )
               {
                  wxLogError(_("Cannot open file '%s' included in "
                               "this message!"), filename);
               }

               if ( !partOk )
                  allPartsOk = false;
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

                  p = new MessageParameter(_T("FILENAME"), wxFileNameFromPath(name));
                  dlist.push_back(p);
               }

               if ( !filename.empty() )
               {
                  MessageParameter *p;

                  p = new MessageParameter(_T("NAME"), filename);
                  plist.push_back(p);
               }

               const MimeType& mt = part->GetMimeType();
               msg->AddPart
                    (
                      mt.GetPrimary(),
                      (wxChar *)part->GetData(),
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

   if ( !allPartsOk )
   {
      // we shouldn't send the message without some attachments, abort
      return NULL;
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
   StringList::const_iterator i = m_extraHeadersNames.begin();
   StringList::const_iterator j = m_extraHeadersValues.begin();
   for ( ; i != m_extraHeadersNames.end(); ++i, ++j )
   {
      msg->AddHeaderEntry(*i, *j);
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


   // postprocess the message if we're really going to send it
   // --------------------------------------------------------

   if ( flags & ForSending )
   {
      // cryptographically sign the message if configured to do it
      if ( IsPGPSigningEnabled() )
      {
         msg->EnableSigning(m_options.m_signWithPGPAs);
      }
   }

   // the caller is responsible for deleting the object
   return msg.release();
}

void
wxComposeView::Send(SendMode mode)
{
   CHECK_RET( !m_msgBeingSent, _T("wxComposeView::Send() reentered") );

   m_msgBeingSent = BuildMessage();
   if ( !m_msgBeingSent )
   {
      wxLogError(_("Failed to create the message to send."));

      return;
   }

   // Set a flag indicating that we're sending the message and update the UI
   // accordingly. Notice that this flag is reset, and UI restored, only by
   // OnSendResult() so the code below must either call it directly before
   // returning or arrange for it to be called at some later time.
   wxLogStatus(this, m_mode == Mode_News ? _("Posting message")
                                         : _("Sending message..."));
   Disable();

   // Set the busy cursor only while we prepare the message for sending, we
   // don't want to leave it on while the message is being sent in the
   // background (unlike the window which is left disabled during all this
   // time).
   MBusyCursor bc;

   // and now do send the message
   if ( mode == SendMode_Later )
   {
      wxString err;

      MModule_Calendar *calmod =
         (MModule_Calendar *) MModule::GetProvider(MMODULE_INTERFACE_CALENDAR);
      if(calmod)
      {
         if ( !calmod->ScheduleMessage(m_msgBeingSent) )
            err = _("Failed to schedule message for later delivery.");

         calmod->DecRef();
      }
      else
      {
         err = _("Cannot schedule message for sending later because "
                 "the calendar module is not available.");
      }

      OnSendResult(err);
   }
   else
   {
      int flags = 0;
      if ( mode == SendMode_Now )
         flags |= SendMessage::NeverQueue;

      SendThreadResult res;
      switch ( m_msgBeingSent->PrepareForSending(flags, &res.outbox) )
      {
         case SendMessage::Result_Prepared:
            {
               wxScopedPtr<SendThread>
                  thr(new SendThread(*this, *m_msgBeingSent));
               if ( thr->Create() == wxTHREAD_NO_ERROR &&
                     thr->Run() == wxTHREAD_NO_ERROR )
               {
                  // thread was successfully started and will delete itself
                  // later, so don't delete it here ourselves
                  thr.release();

                  // it will also call OnSendResult() later, when it's done, so
                  // return and skip calling it below
                  return;
               }

               res = _("Failed to launch the message sending worker thread.");
            }
            break;

         case SendMessage::Result_Queued:
            // message was just queued for sending later, we don't need to do
            // anything else
            break;

         case SendMessage::Result_Cancelled:
            // sending was cancelled, indicate failure but without giving any
            // error message which is useless in this case
            res.success = false;
            break;

         case SendMessage::Result_Error:
            res = _("Failed to prepare the message for sending.");
            break;
      }

      OnSendResult(res);
   }
}

void wxComposeView::OnSendResult(const SendThreadResult& res)
{
   ASSERT_MSG( m_msgBeingSent, "should be in process of sending a message" );

   Enable();

   if ( res.success )
      m_msgBeingSent->AfterSending();
   delete m_msgBeingSent;
   m_msgBeingSent = NULL;

   if ( res.success )
   {
      // FIXME: remove this and possibly replace with an info bar.
      MDialog_Message(_("Message sent."),
                      this, // parent window
                      MDIALOG_MSGTITLE,
                      "MailSentMessage");

      if ( READ_CONFIG(m_Profile, MP_AUTOCOLLECT_OUTGOING) )
      {
         // remember the addresses we sent mail to
         SaveRecipientsAddresses
         (
            M_ACTION_ALWAYS,
            READ_CONFIG_TEXT(m_Profile, MP_AUTOCOLLECT_ADB),
            _("Previous Recipients")
         );
      }

      if ( m_OriginalMessage != NULL )
      {
         // we mark the original message as "answered"
         MailFolder *mf = m_OriginalMessage->GetFolder();
         if(mf)
         {
            if ( !mf->IsOpened() )
            {
               // we could have lost connection while composing the reply
               //
               // TODO: try to reopen the folder
               wxLogWarning(_("Failed to mark the original message as "
                              "answered because the folder \"%s\" containing "
                              "it was closed in the meanwhile."),
                            mf->GetName());
            }
            else
            {
               mf->SetMessageFlag(m_OriginalMessage->GetUId(),
                                  MailFolder::MSG_STAT_ANSWERED, true);
            }
         }
      }

      ResetDirty();

      // show what we have done with the message

      String s;
      if ( m_mode == Mode_News )
      {
         if ( res.outbox.empty() )
         {
            s.Printf(_("Message has been posted to %s"),
                     GetRecipients(Recipient_Newsgroup));
         }
         else
         {
            s.Printf(_("Article queued in '%s'."), res.outbox);
         }
      }
      else // email message
      {
         if ( res.outbox.empty() )
         {
            // NB: don't show BCC as the message might be saved in the log file
            s.Printf(_("Message has been sent to %s"),
                     GetRecipients(Recipient_To));

            String rcptCC = GetRecipients(Recipient_Cc);
            if ( !rcptCC.empty() )
            {
               s += String::Format(_(" (with courtesy copy sent to %s)"),
                                   rcptCC);
            }
            else // no CC
            {
               s += '.';
            }
         }
         else
         {
            s.Printf(_("Message queued in '%s'."), res.outbox);
         }
      }

      // avoid crashes if the message has any stray '%'s
      wxLogStatus(this, _T("%s"), s);

      // we can now safely remove the draft message, if any
      DeleteDraft();

      // and close the window with this message unless we were explicitly asked
      // to keep it opened
      if ( m_closeAfterSending )
         Close();
   }
   else // message not sent
   {
      if ( !res.errGeneral.empty() )
      {
         if ( !res.errDetailed.empty() )
         {
            wxLogError("%s", res.errDetailed);
         }
         //else: no detailed error message, this doesn't carry any particular
         //      meaning unlike the absence of the general error message

         wxLogError("%s", res.errGeneral);
      }
      //else: message sending must have been cancelled, no need to tell the
      //      user about it

      wxLogStatus(this, _("Message was not sent."));
   }
}

void wxComposeView::OnSendThreadDone(wxThreadEvent& evt)
{
   OnSendResult(evt.GetPayload<SendThreadResult>());
}

void
wxComposeView::AddHeaderEntry(const String& name, const String& value)
{
   // first check if we don't already have a header with this name
   const StringList::iterator end = m_extraHeadersNames.end();
   for ( StringList::iterator i = m_extraHeadersNames.begin(),
                                j = m_extraHeadersValues.begin();
         i != end;
         ++i, ++j )
   {
      if ( *i == name )
      {
         if ( value.empty() )
         {
            // remove the existing header
            m_extraHeadersNames.erase(i);
            m_extraHeadersValues.erase(j);
         }
         else // modify the existing header
         {
            *j = value;
         }
         return;
      }
   }

   // if we didn't find it, add a new one
   m_extraHeadersNames.push_back(MIME::EncodeHeader(name).data());
   m_extraHeadersValues.push_back(MIME::EncodeHeader(value).data());
}

bool wxComposeView::IsPGPSigningEnabled() const
{
   return m_options.m_signWithPGP;
}

void wxComposeView::TogglePGPSigning()
{
   m_options.m_signWithPGP = !m_options.m_signWithPGP;
}

bool wxComposeView::IsInReplyTo() const
{
   StringList::const_iterator i,
                              end = m_extraHeadersNames.end();
   for ( i = m_extraHeadersNames.begin(); i != end; ++i )
   {
      if ( *i == "In-Reply-To" )
         return true;
   }

   return false;
}

bool wxComposeView::ConfigureInReplyTo()
{
   StringList::iterator end = m_extraHeadersNames.end();
   StringList::iterator i, j;
   for ( i = m_extraHeadersNames.begin(),
         j = m_extraHeadersValues.begin(); i != end; ++i, ++j )
   {
      if ( *i == "In-Reply-To" )
         break;
   }

   String messageId;
   if ( i != end )
      messageId = *j;

   String messageIdNew = messageId;
   if ( !ConfigureInReplyToHeader(&messageIdNew, this) ||
            messageIdNew == messageId )
      return false;

   if ( messageIdNew.empty() )
   {
      m_extraHeadersNames.erase(i);
      m_extraHeadersValues.erase(j);

      // also update references header
      for ( i = m_extraHeadersNames.begin(),
            end = m_extraHeadersNames.end(),
            j = m_extraHeadersValues.begin() ; i != end; ++i, ++j )
      {
         if ( *i == "References" )
         {
            String ref = *j;
            ref.Trim(true).Trim(false);

            if ( ref == messageId )
            {
               // just remove the header completely
               m_extraHeadersNames.erase(i);
               m_extraHeadersValues.erase(j);
            }
            else // need to edit it
            {
               size_t pos = ref.find(messageId);
               if ( pos != String::npos )
               {
                  // no need to check for "pos > 0" as the string can't
                  // start with spaces: we've trimmed it above
                  size_t n;
                  for ( n = 1; isspace(ref[pos - n]); n++ )
                     ;

                  n--; // took one too many: this one is not a space

                  // remove this message id with all preceding space
                  ref.erase(pos - n, n + messageId.length());

                  *j = ref;
               }
            }
            break;
         }
      }
   }
   else // should be a reply
   {
      if ( i == end )
      {
         AddHeaderEntry(_T("In-Reply-To"), messageIdNew);
      }
      else // just modify existing value
      {
         *j = messageIdNew;
      }

      // and add to references
      for ( i = m_extraHeadersNames.begin(),
            end = m_extraHeadersNames.begin(),
            j = m_extraHeadersValues.begin(); i != end; ++i, ++j )
      {
         if ( *i == "References" )
         {
            String ref = *j;

            // if we had a message id already, replace the old one with the
            // new one
            if ( messageId.empty() || !ref.Replace(messageId, messageIdNew) )
            {
               // if replacement failed (or if we had nothing to replace),
               // just add new message id
               if ( !ref.empty() )
               {
                  // continue "References" header on the next line
                  ref += _T("\015\012 ");
               }

               ref += messageIdNew;
            }

            *j = ref;

            break;
         }
      }

      if ( i == end )
      {
         AddHeaderEntry(_T("References"), messageIdNew);
      }
   }

   return true;
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

/// sets From field using the default value for it from the current profile
void
wxComposeView::SetDefaultFrom()
{
   if ( m_txtFrom )
   {
      SetFrom(Address::GetSenderAddress(m_Profile));
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
                   "the folder '%s'."), mf->GetName());
      return false;
   }

   m_DraftMessage->DecRef();
   m_DraftMessage = NULL;

   return true;
}

SendMessage *wxComposeView::BuildDraftMessage(int flags) const
{
   ASSERT_MSG( !(flags & ForSending), "draft is not for sending" );

   SendMessage_obj msg(BuildMessage(flags));
   if ( !msg )
   {
      if ( flags & Interactive )
      {
         wxLogError(_("Failed to create the message to save."));
      }

      return NULL;
   }

   // mark this message as our draft (the value doesn't matter)
   msg->AddHeaderEntry(HEADER_IS_DRAFT, _T("Yes"));

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

   // also save the Fcc header contents because it's not a "real" header
   msg->AddHeaderEntry(_T("FCC"), GetRecipients(Recipient_Fcc));

   return msg.release();
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
   if(!m_editor->FinishWork())
      return false;

   SendMessage_obj msg(BuildDraftMessage());
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
         nameDrafts
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
   name << DIR_SEPARATOR << _T("composer") << DIR_SEPARATOR;

   return name;
}

bool
wxComposeView::AutoSave()
{
   // we disable autosave if we don't have a directory to save files to
   static bool s_autosaveEnabled = true;

   if ( !s_autosaveEnabled )
      return true;

   if ( !m_editor )
      return true;

   if ( !m_editor->IsModified() )
   {
      // nothing to do
      return true;
   }

   SendMessage_obj msg(BuildDraftMessage(NonInteractive));
   if ( !msg )
      return false;

   String fname = m_filenameAutoSave;  // reuse the last file name
   if ( fname.empty() )                // but initialize if it's the first time
   {
      // make sure the directory we use for these scratch files exists
      String name = GetComposerAutosaveDir();
      if ( !wxDir::Exists(name) )
      {
         if ( !wxMkdir(name, 0700) )
         {
            wxLogSysError(_("Failed to create the directory '%s' for the "
                            "temporary composer files"), name);

            wxLogError(_("Composer messages won't be saved automatically "
                         "for the duration of this session.\n"
                         "Please make sure that Mahogany can create the "
                         "directory \"%s\" and restart the program."),
                       name);
            s_autosaveEnabled = false;

            return false;
         }
      }

      // we need a unique file name during the life time of this object as this
      // file is always going to be deleted if we're destroyed correctly, it
      // can only be left if the program crashes
      fname = name + String::Format(_T("%05d%p"), (int)getpid(), this);
   }

   String contents;
   if ( !msg->WriteToString(contents) )
   {
      // this is completely unexpected
      FAIL_MSG( _T("Failed to get the message text?") );

      return false;
   }

   if ( !MailFolder::SaveMessageAsMBOX(fname, contents) )
   {
      // don't make this a wxLogError() as it would result in a message box
      // which is a wrong thing to do for a background operation
      wxLogStatus(_("Failed to automatically save the message."));

      // if we did manage to create the file, leave it even if saving failed,
      // it might still contain something useful for recovery
      if ( wxFileName::FileExists(fname) )
         m_filenameAutoSave = fname;

      return false;
   }

   if ( m_filenameAutoSave.empty() )
   {
      // remember the file name for the subsequent calls of this function
      m_filenameAutoSave = fname;
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

   int nResumed = 0;

   wxString filename;
   bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
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
         MailFolder_obj mf(MailFolder::OpenFolder(folder));
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
                 filename);
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
   AttachmentMenu::ShowAttachmentProperties(GetWindow(), part);
}

void
MessageEditor::ShowAttachmentMenu(EditorContentPart *part, const wxPoint& pt)
{
   CHECK_RET( part, _T("no attachment to show menu for in ShowAttachmentMenu") );

   if ( part->GetFileName().empty() )
   {
      // this is not an attachment at all?
      return;
   }

   wxWindow *win = GetWindow();
   CHECK_RET( win, _T("can't show attachment menu without parent window") );

   AttachmentMenu menu(win, part);
   win->PopupMenu(&menu, pt);
}

// ----------------------------------------------------------------------------
// trivial MessageEditor accessors
// ----------------------------------------------------------------------------

Profile *MessageEditor::GetProfile() const
{
   return m_composer->GetProfile();
}

const ComposerOptions& MessageEditor::GetOptions() const
{
   return m_composer->GetOptions();
}

bool MessageEditor::OnFirstTimeFocus()
{
   return m_composer->OnFirstTimeFocus();
}

void MessageEditor::OnFirstTimeModify()
{
   m_composer->OnFirstTimeModify();
}
