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

#  include <ctype.h>          // for isspace()
#endif

#include <wx/textctrl.h>
#include <wx/ffile.h>
#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/mimetype.h>
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

#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxIdentityCombo.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxComposeView.h"

#include "adb/AdbEntry.h"
#include "adb/AdbManager.h"

#include "MessageTemplate.h"
#include "TemplateDialog.h"

#include "MModule.h"
#include "modules/Calendar.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADB_SUBSTRINGEXPANSION;
extern const MOption MP_AFMPATH;
extern const MOption MP_ALWAYS_USE_EXTERNALEDITOR;
extern const MOption MP_AUTOMATIC_WORDWRAP;
extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_CC;
extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_COMPOSE_TO;
extern const MOption MP_COMPOSE_USE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE_SEPARATOR;
extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_CVIEW_BGCOLOUR;
extern const MOption MP_CVIEW_FGCOLOUR;
extern const MOption MP_CVIEW_FONT;
extern const MOption MP_CVIEW_FONT_SIZE;
extern const MOption MP_EXTERNALEDITOR;
extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_HOSTNAME;
extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_SMTPHOST;
extern const MOption MP_USERLEVEL;
extern const MOption MP_USEVCARD;
extern const MOption MP_VCARD;
extern const MOption MP_WRAPMARGIN;

#ifdef OS_UNIX
extern const MOption MP_USE_SENDMAIL;
#endif // OS_UNIX

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   // the expand button
   IDB_EXPAND = 100
};

// the default message title
#define COMPOSER_TITLE (_("Message Composition"))

// separate multiple addresses with commas
#define CANONIC_ADDRESS_SEPARATOR   ", "

// code here was written with assumption that x and y margins are the same
#define LAYOUT_MARGIN LAYOUT_X_MARGIN

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

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxRcptRemoveButton;
class wxRcptTextCtrl;
class wxRcptTypeChoice;

// ----------------------------------------------------------------------------
// MimeContent represents an attachement in the composer
// ----------------------------------------------------------------------------

class MimeContent : public wxLayoutObject::UserData
{
public:
   // constants
   enum MimeContentType
   {
      MIMECONTENT_NONE,
      MIMECONTENT_FILE,
      MIMECONTENT_DATA
   };

   // ctor & dtor
   MimeContent()
      {
         m_Type = MIMECONTENT_NONE;
      }
   // initialize
   void SetMimeType(const String& mimeType);
   void SetData(void *data,
                size_t length,
                const char *filename = NULL); // we'll delete data!
   void SetFile(const String& filename);

   // accessors
   MimeContentType GetType() const { return m_Type; }

   String GetMimeType() const { return m_MimeType.GetFull(); }
   MimeType::Primary GetMimeCategory() const { return m_MimeType.GetPrimary(); }

   const String& GetFileName() const
      { return m_FileName; }

   const void *GetData() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Data; }
   size_t GetSize() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Length; }

protected:
   ~MimeContent()
      {
         if ( m_Type == MIMECONTENT_DATA )
         {
            // FIXME: should be "char *" everywhere!
            delete [] (char *)m_Data;
         }
      }


private:
   MimeContentType m_Type;

   void     *m_Data;
   size_t    m_Length;
   String    m_FileName;

   MimeType m_MimeType;
};

// ----------------------------------------------------------------------------
// wxRcptControls: all controls in the row corresponding to one recipient
// ----------------------------------------------------------------------------

class wxRcptControls
{
public:
   wxRcptControls(wxComposeView *cv, size_t index)
   {
      m_composeView = cv;
      m_index = index;

      m_choice = NULL;
      m_text = NULL;
      m_btn = NULL;
   }

   ~wxRcptControls();

   // create the controls and return a sizer containing them
   wxSizer *CreateControls(wxWindow *parent);

   // initialize the controls
   void InitControls(const String& value, wxComposeView::RecipientType rt);

   // decrement our index (presumably because another control was deleted
   // before us)
   void DecIndex()
   {
      ASSERT_MSG( m_index > 0, "shouldn't be called" );

      m_index--;
   }

   // get our text control
   wxRcptTextCtrl *GetText() const { return m_text; }

   // get the currently selected address type
   wxComposeView::RecipientType GetType() const;

   // get the current value of the text field
   wxString GetValue() const;

   // starting from now, all methods are for the wxRcptXXX controls only

   // set this one as last active - called by text
   void SetLast() { m_composeView->SetLastAddressEntry(m_index); }

   // change type of this one - called by choice
   void OnTypeChange(wxComposeView::RecipientType rcptType);

   // remove this one - called by button
   void OnRemove() { m_composeView->OnRemoveRcpt(m_index); }

   // is it enabled (disabled if type == none)?
   bool IsEnabled() const;

   // get the composer
   wxComposeView *GetComposer() const { return m_composeView; }

private:
   // the back pointer to the composer
   wxComposeView *m_composeView;

   // our index in m_composeView->m_rcptControls
   size_t m_index;

   // our controls
   wxRcptTypeChoice *m_choice;
   wxRcptTextCtrl *m_text;
   wxRcptRemoveButton *m_btn;
};

// ----------------------------------------------------------------------------
// wxRcptTypeChoice is first part of wxRcptControls
// ----------------------------------------------------------------------------

class wxRcptTypeChoice : public wxChoice
{
public:
   wxRcptTypeChoice(wxRcptControls *rcptControls, wxWindow *parent)
      : wxChoice(parent, -1,
                 wxDefaultPosition, wxDefaultSize,
                 WXSIZEOF(ms_addrTypes), ms_addrTypes)
   {
      m_rcptControls = rcptControls;
   }

   // callbacks
   void OnChoice(wxCommandEvent& event);

private:
   // the back pointer to the entire group of controls
   wxRcptControls *m_rcptControls;

   static const wxString ms_addrTypes[wxComposeView::Recipient_Max];

   DECLARE_EVENT_TABLE()
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

class wxAddressTextCtrl : public wxTextCtrl
{
public:
   // ctor
   wxAddressTextCtrl(wxWindow *parent, wxComposeView *composer)
      : wxTextCtrl(parent, -1, "",
                   wxDefaultPosition, wxDefaultSize,
                   wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB)
   {
      m_composeView = composer;

      m_lookupMode = READ_CONFIG(composer->GetProfile(),
                                 MP_ADB_SUBSTRINGEXPANSION)
                        ? AdbLookup_Substring : AdbLookup_StartsWith;
   }

   // expand the text in the control using the address book(s): returns FALSE
   // if no expansion took place (and also show a message in the status bar
   // about this unless quiet is true)
   bool DoExpand(bool quiet = false);

   // callbacks
   void OnFocusSet(wxFocusEvent& event);
   void OnChar(wxKeyEvent& event);
   void OnEnter(wxCommandEvent& event);

protected:
   // to implement in the derived classes
   virtual void SetAsLastActive() const = 0;

   wxComposeView *GetComposer() const { return m_composeView; }

private:
   // the composer
   wxComposeView *m_composeView;

   // ADB lookup mode (substring or any)
   int m_lookupMode;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxNewAddressTextCtrl: wxAddressTextCtrl which allows the user to add new
// recipients
// ----------------------------------------------------------------------------

class wxNewAddressTextCtrl : public wxAddressTextCtrl
{
public:
   wxNewAddressTextCtrl(wxComposeView *composeView, wxWindow *parent)
      : wxAddressTextCtrl(parent, composeView)
      {
         m_composeView = composeView;
      }

   // add the new recipient fields for the addresses currently entered
   void AddNewRecipients(bool quiet = false);

   // callbacks
   void OnEnter(wxCommandEvent& event);

protected:
   virtual void SetAsLastActive() const
      { m_composeView->SetLastAddressEntry(-1); }

private:
   wxComposeView *m_composeView;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxRcptTextCtrl: wxAddressTextCtrl which is part of wxRcptControls
// ----------------------------------------------------------------------------

class wxRcptTextCtrl : public wxAddressTextCtrl
{
public:
   wxRcptTextCtrl(wxRcptControls *rcptControls, wxWindow *parent)
      : wxAddressTextCtrl(parent, rcptControls->GetComposer())
      {
         m_rcptControls = rcptControls;
      }

   // callbacks
   void OnUpdateUI(wxUpdateUIEvent& event);

protected:
   virtual void SetAsLastActive() const { m_rcptControls->SetLast(); }

private:
   // the back pointer to the entire group of controls
   wxRcptControls *m_rcptControls;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxRcptRemoveButton: small button which is used to remove previously added
// recipient controls
// ----------------------------------------------------------------------------

class wxRcptRemoveButton : public wxButton
{
public:
   wxRcptRemoveButton(wxRcptControls *rcptControls, wxWindow *parent)
      : wxButton(parent, -1, _("Delete"))
      {
         m_rcptControls = rcptControls;
      }

   // callback
   void OnButton(wxCommandEvent&) { m_rcptControls->OnRemove(); }

private:
   // the back pointer to the entire group of controls
   wxRcptControls *m_rcptControls;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// a slightly different wxLayoutWindow: it calls back the composer the first
// time it gets focus which allows us to launch an external editor then (if
// configured to do so, of course)
// ----------------------------------------------------------------------------

class wxComposerLayoutWindow : public wxLayoutWindow
{
public:
   wxComposerLayoutWindow(wxComposeView *composer, wxWindow *parent)
      : wxLayoutWindow(parent)
   {
      m_composer = composer;

      m_firstTimeModify =
      m_firstTimeFocus = TRUE;
   }

protected:
   void OnKeyDown(wxKeyEvent& event)
   {
      if ( m_firstTimeModify )
      {
         m_firstTimeModify = FALSE;

         m_composer->OnFirstTimeModify();
      }

      event.Skip();
   }

   void OnFocus(wxFocusEvent& event)
   {
      if ( m_firstTimeFocus )
      {
         m_firstTimeFocus = FALSE;

         if ( m_composer->OnFirstTimeFocus() )
         {
            // composer doesn't need first modification notification any more
            // because it modified the text itself
            m_firstTimeModify = FALSE;
         }
      }

      event.Skip();
   }

private:
   wxComposeView *m_composer;

   bool m_firstTimeModify,
        m_firstTimeFocus;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables &c
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxComposeView, wxMFrame)

BEGIN_EVENT_TABLE(wxComposeView, wxMFrame)
   // process termination notification
   EVT_END_PROCESS(HelperProcess_Editor, wxComposeView::OnExtEditorTerm)

   // button notifications
   EVT_BUTTON(IDB_EXPAND, wxComposeView::OnExpand)

   // identity combo notification
   EVT_CHOICE(IDC_IDENT_COMBO, wxComposeView::OnIdentChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptTypeChoice, wxChoice)
   EVT_CHOICE(-1, wxRcptTypeChoice::OnChoice)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAddressTextCtrl, wxTextCtrl)
   EVT_SET_FOCUS(wxAddressTextCtrl::OnFocusSet)

   EVT_CHAR(wxAddressTextCtrl::OnChar)
   EVT_TEXT_ENTER(-1, wxAddressTextCtrl::OnEnter)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxNewAddressTextCtrl, wxAddressTextCtrl)
   EVT_TEXT_ENTER(-1, wxNewAddressTextCtrl::OnEnter)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptTextCtrl, wxAddressTextCtrl)
   EVT_UPDATE_UI(-1, wxRcptTextCtrl::OnUpdateUI)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxRcptRemoveButton, wxButton)
   EVT_BUTTON(-1, wxRcptRemoveButton::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxComposerLayoutWindow, wxLayoutWindow)
   EVT_KEY_DOWN(wxComposerLayoutWindow::OnKeyDown)
   EVT_SET_FOCUS(wxComposerLayoutWindow::OnFocus)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MimeContent
// ----------------------------------------------------------------------------

void MimeContent::SetMimeType(const String& mimeType)
{
   m_MimeType = mimeType;
}

void MimeContent::SetData(void *data,
                          size_t length,
                          const char *filename)
{
   ASSERT( data != NULL );

   m_Data = data;
   m_Length = length;
   m_Type = MIMECONTENT_DATA;
   if(filename)
      m_FileName = filename;
}

void MimeContent::SetFile(const String& filename)
{
   ASSERT( !filename.empty() );

   m_FileName = filename;
   m_Type = MIMECONTENT_FILE;
}

// ----------------------------------------------------------------------------
// wxRcptControls
// ----------------------------------------------------------------------------

wxSizer *wxRcptControls::CreateControls(wxWindow *parent)
{
   // create controls
   m_choice = new wxRcptTypeChoice(this, parent);
   m_text = new wxRcptTextCtrl(this, parent);
   m_btn = new wxRcptRemoveButton(this, parent);

   // add them to sizer
   wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
   sizer->Add(m_choice, 0, wxRIGHT | wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
   sizer->Add(m_text, 1, wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
   sizer->Add(m_btn, 0, wxLEFT, LAYOUT_MARGIN);

   return sizer;
}

void wxRcptControls::InitControls(const String& value,
                                  wxComposeView::RecipientType rt)
{
   CHECK_RET( m_choice, "must call CreateControls first" );

   m_choice->SetSelection(rt);
   m_text->SetValue(value);
}

wxRcptControls::~wxRcptControls()
{
   delete m_choice;
   delete m_text;
   delete m_btn;
}

void wxRcptControls::OnTypeChange(wxComposeView::RecipientType rcptType)
{
   m_composeView->OnRcptTypeChange(rcptType);
}

bool wxRcptControls::IsEnabled() const
{
   return m_choice->GetSelection() != wxComposeView::Recipient_None;
}

wxComposeView::RecipientType wxRcptControls::GetType() const
{
   return (wxComposeView::RecipientType)m_choice->GetSelection();
}

wxString wxRcptControls::GetValue() const
{
   return m_text->GetValue();
}

// ----------------------------------------------------------------------------
// wxRcptTypeChoice
// ----------------------------------------------------------------------------

const wxString wxRcptTypeChoice::ms_addrTypes[] =
{
   _("To"),
   _("Cc"),
   _("Bcc"),
   _("Newsgroup"),
   _("None"),
};

void wxRcptTypeChoice::OnChoice(wxCommandEvent& event)
{
   // notify the others (including the composer indirectly)
   m_rcptControls->
      OnTypeChange((wxComposeView::RecipientType)event.GetSelection());

   event.Skip();
}

// ----------------------------------------------------------------------------
// wxAddressTextCtrl
// ----------------------------------------------------------------------------

void wxAddressTextCtrl::OnEnter(wxCommandEvent& /* event */)
{
   // pass to the next control when <Enter> is pressed
   wxNavigationKeyEvent event;
   event.SetDirection(TRUE);       // forward
   event.SetWindowChange(FALSE);   // control change
   event.SetEventObject(this);

   GetParent()->GetEventHandler()->ProcessEvent(event);
}

// mark this ctrl as being the last active - so the [Expand] btn will expand us
void wxAddressTextCtrl::OnFocusSet(wxFocusEvent& event)
{
   SetAsLastActive();

   event.Skip();
}

// expand the address when <TAB> is pressed
void wxAddressTextCtrl::OnChar(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   SetAsLastActive();

   // we're only interested in TABs and only it's not a second TAB in a row
   if ( event.KeyCode() == WXK_TAB )
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
            DoExpand();

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

bool wxAddressTextCtrl::DoExpand(bool quiet)
{
   // try to expand the last component
   String text = GetValue();
   text.Trim(FALSE); // trim spaces from both sides
   text.Trim(TRUE);

   // check for the lone '"' simplifies the code for finding the starting
   // position below: it should be done here, otherwise the following loop
   // will crash!
   if ( text.empty() || text == '"' )
   {
      // don't do anything
      if ( !quiet )
      {
         wxLogStatus(GetComposer(),
                     _("Nothing to expand - please enter something."));
      }

      return FALSE;
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

   // remove "mailto:" prefix if it's there - this is convenient when you paste
   // in an URL from the web browser
   String newText;
   if ( !textOrig.StartsWith("mailto:", &newText) )
   {
      // if the text already has a '@' inside it, assume it's a full email
      // address and doesn't need to be expanded (this saves a lot of time as
      // expanding a non existing address looks through all address books...)
      if ( textOrig.find('@') != String::npos )
      {
         // nothing to do
         return TRUE;
      }

      wxArrayString expansions;
      if ( !AdbExpand(expansions, textOrig, m_lookupMode,
                     quiet ? NULL : GetComposer()) )
      {
         // cancelled, don't do anything
         return TRUE;
      }

      // construct the replacement string(s)
      size_t nExpCount = expansions.GetCount();
      for ( size_t nExp = 0; nExp < nExpCount; nExp++ )
      {
         if ( nExp > 0 )
            newText += CANONIC_ADDRESS_SEPARATOR;

         wxString address(expansions[nExp]);

         // sometimes we must quote the address
         bool doQuote = strpbrk(address, ",;\"") != (const char *)NULL;
         if ( doQuote )
         {
            newText += '"';

            // escape all quotes
            address.Replace("\"", "\\\"");
         }

         newText += address;

         if ( doQuote )
         {
            newText += '"';
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

   SetValue(oldText + newText);
   SetInsertionPointEnd();

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxNewAddressTextCtrl
// ----------------------------------------------------------------------------

void wxNewAddressTextCtrl::AddNewRecipients(bool quiet)
{
   // expand before adding (make this optional?)
   DoExpand(quiet);

   // add new recipient(s)
   m_composeView->AddRecipients(GetValue());

   // clear the entry zone as recipient(s) were moved elsewhere
   SetValue("");
}

void wxNewAddressTextCtrl::OnEnter(wxCommandEvent& /* event */)
{
   AddNewRecipients();
}

// ----------------------------------------------------------------------------
// wxRcptTextCtrl
// ----------------------------------------------------------------------------

void wxRcptTextCtrl::OnUpdateUI(wxUpdateUIEvent& event)
{
   // enable the text only if it has a valid type
   event.Enable(m_rcptControls->IsEnabled());
}

// ----------------------------------------------------------------------------
// wxComposeView creation: static functions
// ----------------------------------------------------------------------------

/** Constructor for posting news.
    @param parentProfile parent profile
    @param hide if true, do not show frame
    @return pointer to the new compose view
*/
Composer *
Composer::CreateNewArticle(const MailFolder::Params& params,
                           Profile *parentProfile,
                           bool hide)
{
   wxWindow *parent = mApplication->TopLevelFrame();
   wxComposeView *cv = new wxComposeView("ComposeViewNews", parent);
   cv->m_mode = wxComposeView::Mode_News;
   cv->m_kind = wxComposeView::Message_New;
   cv->m_template = params.templ;
   cv->SetTitle(COMPOSER_TITLE);
   cv->Create(parent, parentProfile);

   return cv;
}

/** Constructor for sending mail.
    @param parentProfile parent profile
    @param hide if true, do not show frame
    @return pointer to the new compose view
*/
Composer *
Composer::CreateNewMessage(const MailFolder::Params& params,
                           Profile *parentProfile,
                           bool hide)
{
   wxWindow *parent = mApplication->TopLevelFrame();
   wxComposeView *cv = new wxComposeView("ComposeViewMail", parent);
   cv->m_mode = wxComposeView::Mode_Mail;
   cv->m_kind = wxComposeView::Message_New;
   cv->m_template = params.templ;
   cv->SetTitle(COMPOSER_TITLE);
   cv->Create(parent,parentProfile);

   return cv;
}

Composer *
Composer::CreateReplyMessage(const MailFolder::Params& params,
                             Profile *parentProfile,
                             Message *original,
                             bool hide)
{
   wxComposeView *cv = CreateNewMessage(parentProfile, hide)->GetComposeView();

   cv->m_kind = wxComposeView::Message_Reply;
   cv->m_template = params.templ;

   cv->m_OriginalMessage = original;
   SafeIncRef(cv->m_OriginalMessage);

   // write reply by default in the same encoding as the original message
   cv->SetEncodingToSameAs(original);

   return cv;
}

Composer *
Composer::CreateFwdMessage(const MailFolder::Params& params,
                           Profile *parentProfile,
                           Message *original,
                           bool hide)
{
   wxComposeView *cv = CreateNewMessage(parentProfile, hide)->GetComposeView();

   cv->m_kind = wxComposeView::Message_Forward;
   cv->m_template = params.templ;

   // preserve message encoding when forwarding it
   cv->SetEncodingToSameAs(original);

   return cv;
}

// ----------------------------------------------------------------------------
// wxComposeView ctor/dtor
// ----------------------------------------------------------------------------

wxComposeView::wxComposeView(const String &name,
                             wxWindow *parent)
             : wxMFrame(name,parent)
{
   m_name = name;
   m_pidEditor = 0;
   m_procExtEdit = NULL;
   m_sending = false;
   m_OriginalMessage = NULL;

   m_indexLast = -1;

   // by default new recipients are "to"
   m_rcptTypeLast = Recipient_To;

   m_LayoutWindow = NULL;
   m_encoding = wxFONTENCODING_SYSTEM;
}

wxComposeView::~wxComposeView()
{
   SafeDecRef(m_Profile);

   delete m_LayoutWindow;

   SafeDecRef(m_OriginalMessage);
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

   // FIXME: provide some visual feedback for them, like
   //        enabling/disabling them. Not used yet.
   m_MItemCut = GetMenuBar()->FindItem(WXMENU_EDIT_CUT);
   m_MItemCopy = GetMenuBar()->FindItem(WXMENU_EDIT_COPY);
   m_MItemPaste = GetMenuBar()->FindItem(WXMENU_EDIT_CUT);

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
         FAIL_MSG( "didn't find \"Send later\" in compose menu, why?" );
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
   AddToolbarButtons(CreateToolBar(), WXFRAME_COMPOSE);

   CreateStatusBar(2);
   static const int s_widths[] = { -1, 90 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);
}

// ----------------------------------------------------------------------------
// wxComposeView "real" creation: here we create the controls and lay them out
// ----------------------------------------------------------------------------

wxSizer *
wxComposeView::CreateSizerWithText(wxControl *control,
                                   wxTextCtrl **ppText,
                                   TextField tf,
                                   wxWindow *parent)
{
   if ( !parent )
      parent = m_panel;

   wxSizer *sizerRow = new wxBoxSizer(wxHORIZONTAL);
   wxTextCtrl *text;
   switch ( tf )
   {
      default:
         FAIL_MSG( "unexpected text field kind" );
         // fall through

      case TextField_Normal:
         text = new wxTextCtrl(parent, -1, _T(""));
         break;

      case TextField_Address:
         text = new wxNewAddressTextCtrl(this, parent);
   }

   sizerRow->Add(control, 0, wxRIGHT | wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);
   sizerRow->Add(text, 1, wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);

   if ( ppText )
      *ppText = text;

   return sizerRow;
}

// create a sizer containing a label and a text ctrl
wxSizer *
wxComposeView::CreateSizerWithTextAndLabel(const wxString& label,
                                           wxTextCtrl **ppText,
                                           TextField tf)
{
    return CreateSizerWithText(new wxStaticText(m_panel, -1, label), ppText, tf);
}

void wxComposeView::CreatePlaceHolder()
{
   CHECK_RET( m_sizerRcpts && m_rcptControls.IsEmpty(),
              "can't or shouldn't create the place holder now!" );

   m_sizerRcpts->Add(0, 0, 1);
   m_sizerRcpts->Add(new wxStaticText
                         (
                           m_panelRecipients->GetCanvas(),
                           -1,
                           _("No recipients")
                         ),
                     0, wxALIGN_CENTRE | wxALL, LAYOUT_MARGIN);
   m_sizerRcpts->Add(0, 0, 1);
}

void wxComposeView::DeletePlaceHolder()
{
   CHECK_RET( m_sizerRcpts && m_rcptControls.IsEmpty(),
              "can't or shouldn't delete the place holder now!" );

   // remove the spacers and the static text we had added to it
   m_sizerRcpts->Remove((int)0); // remove first spacer by position

   wxSizerItem *item =
      (wxSizerItem *)m_sizerRcpts->GetChildren().First()->Data();

   ASSERT_MSG( item->IsWindow(), "should be the static text" );

   wxWindow *win = item->GetWindow();
   m_sizerRcpts->Remove((int)0); // remove static text by position

   m_sizerRcpts->Remove((int)0); // remove second spacer by position
   delete win;
}

wxSizer *wxComposeView::CreateHeaderFields()
{
   // top level vertical (box) sizer
   wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);

   // leave number of rows unspecified, it can eb calculated from number of
   // columns (2)
   wxFlexGridSizer *sizerHeaders =
      new wxFlexGridSizer(0, 2, LAYOUT_MARGIN, LAYOUT_MARGIN);

   sizerHeaders->AddGrowableCol(1);

   // add from line but only if user level is advanced
   if ( READ_CONFIG(m_Profile, MP_USERLEVEL) >= M_USERLEVEL_ADVANCED )
   {
      sizerHeaders->Add(new wxStaticText(m_panel, -1, _("&From:")),
                        0, wxALIGN_CENTRE_VERTICAL);

      wxSizer *sizerFrom = new wxBoxSizer(wxHORIZONTAL);

      m_txtFrom = new wxTextCtrl(m_panel, -1, _T(""));
      sizerFrom->Add(m_txtFrom, 1, wxALIGN_CENTRE_VERTICAL);

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

   m_txtSubject = new wxTextCtrl(m_panel, -1, _T(""));
   sizerHeaders->Add(m_txtSubject, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL);

   // recipient line
   sizerHeaders->Add(new wxStaticText(m_panel, -1, _("&Address:")),
                     0, wxALIGN_CENTRE_VERTICAL);
   m_txtRecipient = new wxNewAddressTextCtrl(this, m_panel);

   wxSizer *sizerRcpt = new wxBoxSizer(wxHORIZONTAL);
   sizerRcpt->Add(m_txtRecipient, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL);

   wxButton *btn = new wxButton(m_panel, IDB_EXPAND, "&Expand");
   btn->SetToolTip(_("Expand the address using address books"));
   sizerRcpt->Add(btn, 0, wxLEFT | wxALIGN_CENTRE_VERTICAL, LAYOUT_MARGIN);

   sizerHeaders->Add(sizerRcpt, 1, wxEXPAND | wxALIGN_CENTRE_VERTICAL);

   sizerTop->Add(sizerHeaders, 0, wxALL | wxEXPAND, LAYOUT_MARGIN);

   // the spare space for already entered recipients below: we use an extra
   // sizer because we keep it to add more stuff to it later
   m_panelRecipients = new wxEnhancedPanel(m_panel);

   m_sizerRcpts = new wxBoxSizer(wxVERTICAL);
   CreatePlaceHolder();

   m_panelRecipients->GetCanvas()->SetSizer(m_sizerRcpts);
   m_panelRecipients->GetCanvas()->SetAutoLayout(TRUE);

   sizerTop->Add(m_panelRecipients, 1, wxEXPAND);

   // this number is completely arbitrary (FIXME)
   sizerTop->SetItemMinSize(m_panelRecipients, 0, 80);

   return sizerTop;
}

void
wxComposeView::Create(wxWindow * WXUNUSED(parent),
                      Profile *parentProfile,
                      bool hide)
{
   // first create the profile: we must have one, so find a non NULL one
   m_Profile = parentProfile ? parentProfile : mApplication->GetProfile();
   m_Profile->IncRef();

   // sometimes this profile had been created before the identity changed:
   // make sure we use the current identity for the new message composition
   m_Profile->SetIdentity(READ_APPCONFIG(MP_CURRENT_IDENTITY));

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
   CreateFTCanvas();

   // configure splitter
   wxCoord heightHeaders = m_panel->GetSize().y;
   m_splitter->SplitHorizontally(m_panel, m_LayoutWindow, heightHeaders);
   m_splitter->SetMinimumPaneSize(heightHeaders);

   // initialize the controls
   // -----------------------

   // set def values for all headers
   SetDefaultFrom();

   AddTo(READ_CONFIG(m_Profile, MP_COMPOSE_TO));
   AddCc(READ_CONFIG(m_Profile, MP_COMPOSE_CC));
   AddBcc(READ_CONFIG(m_Profile, MP_COMPOSE_BCC));

   // show the frame
   // --------------

   if ( !hide )
   {
      Show(TRUE);
   }
}

/// create the compose window itself
void
wxComposeView::CreateFTCanvas(void)
{
   wxASSERT_MSG( m_LayoutWindow == NULL, "creating layout window twice?" );

   // create the window
   m_LayoutWindow = new wxComposerLayoutWindow(this, m_splitter);

   // and set its params from config:

   // colours
   GetColourByName(&m_fg, READ_CONFIG(m_Profile, MP_CVIEW_FGCOLOUR), "black");
   GetColourByName(&m_bg, READ_CONFIG(m_Profile, MP_CVIEW_BGCOLOUR), "white");

   // font family
   static const int wxFonts[] =
   {
     wxDEFAULT,
     wxDECORATIVE,
     wxROMAN,
     wxSCRIPT,
     wxSWISS,
     wxMODERN,
     wxTELETYPE
   };

   m_font = READ_CONFIG(m_Profile, MP_CVIEW_FONT);
   if ( m_font < 0 || (size_t)m_font > WXSIZEOF(wxFonts) )
   {
      FAIL_MSG( "invalid font value in config" );
      m_font = 0;
   }

   m_font = wxFonts[m_font];

   // font size
   m_size = READ_CONFIG(m_Profile,MP_CVIEW_FONT_SIZE);

   // others
#ifndef OS_WIN
   m_LayoutWindow->
      SetFocusFollowMode(READ_CONFIG_BOOL(m_Profile, MP_FOCUS_FOLLOWSMOUSE));
#endif

   EnableEditing(true);
   m_LayoutWindow->SetCursorVisibility(1);
   DoClear();

   m_LayoutWindow->SetWrapMargin( READ_CONFIG(m_Profile, MP_WRAPMARGIN));
   m_LayoutWindow->
      SetWordWrap(READ_CONFIG_BOOL(m_Profile, MP_AUTOMATIC_WORDWRAP));

   // tell it which status bar pane to use
   m_LayoutWindow->SetStatusBar(GetStatusBar(), 0, 1);
}

void wxComposeView::DoClear()
{
   m_LayoutWindow->Clear(m_font, m_size, (int) wxNORMAL, (int) wxNORMAL, 0,
                         &m_fg, &m_bg);

   // set the default encoding if any
   SetEncoding(wxFONTENCODING_DEFAULT);

   ResetDirty();
}

// ----------------------------------------------------------------------------
// wxComposeView address headers stuff
// ----------------------------------------------------------------------------

void
wxComposeView::AddRecipientControls(const String& value, RecipientType rt)
{
   // remove the place holder we had there before
   if ( m_rcptControls.IsEmpty() )
   {
      DeletePlaceHolder();
   }

   // create the controls and add them to the sizer
   wxRcptControls *rcpt = new wxRcptControls(this, m_rcptControls.GetCount());

   wxSizer *sizerRcpt = rcpt->CreateControls(m_panelRecipients->GetCanvas());

   rcpt->InitControls(value, rt);

   m_rcptControls.Add(rcpt);

   m_sizerRcpts->Add(sizerRcpt, 0, wxALL | wxEXPAND, LAYOUT_MARGIN / 2);
   m_sizerRcpts->Layout();
   m_panelRecipients->RefreshScrollbar(m_panelRecipients->GetClientSize());
}

void
wxComposeView::OnRemoveRcpt(size_t index)
{
   // we can just remove the sizer by position because AddRecipientControls()
   // above adds one sizer for each new recipient - if it changes, this code
   // must change too!
   m_sizerRcpts->Remove(index);

   // and delete the controls
   delete m_rcptControls[index];

   // remove them from the arrays too
   m_rcptControls.RemoveAt(index);

   // and don't forget to adjust the indices of all the others
   size_t count = m_rcptControls.GetCount();
   while ( index < count )
   {
      m_rcptControls[index++]->DecIndex();
   }

   // and adjust the number of controls
   if ( m_indexLast != -1 && (size_t)m_indexLast == count )
   {
      m_indexLast = count - 1;
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

   // split the string in addreses and add all of them
   AddressList_obj addrList(address, READ_CONFIG(m_Profile, MP_HOSTNAME));
   Address *addr = addrList->GetFirst();
   while ( addr )
   {
      AddRecipient(addr->GetAddress(), addrType);

      addr = addrList->GetNext(addr);
   }
}

void
wxComposeView::AddRecipient(const String& addr, RecipientType addrType)
{
   // this is a private function, AddRecipients() above is the public one and it
   // does the parameter validation
   CHECK_RET( !addr.empty() && addrType != Recipient_Max,
              "invalid parameter in AddRecipient()" );

   // look if we don't already have it
   size_t count = m_rcptControls.GetCount();

   bool foundWithAnotherType = false;
   for ( size_t n = 0; n < count; n++ )
   {
      if ( !IsRecipientEnabled(n) )
      {
         // type "none" doesn't count
         continue;
      }

      if ( Message::CompareAddresses(m_rcptControls[n]->GetValue(), addr) )
      {
         // ok, we already have this address - is it of the same type?
         if ( m_rcptControls[n]->GetType() == addrType )
         {
            // yes, don't add it again
            wxLogStatus(this,
                        _("Address '%s' is already in the recipients list, "
                          "not added."),
                        addr.c_str());
            return;
         }

         foundWithAnotherType = true;

         // continue - we may yet find it with the same typ later
      }
   }

   if ( foundWithAnotherType )
   {
      // warn the user that the address is already present but add it
      // nevertheless - I think this is the most reasonable thing to do,
      // anything better?
      wxLogStatus(this,
                  _("Address '%s' is already in the recipients list but "
                    "with a different type - will be duplicated."),
                  addr.c_str());
   }

   // do add it (either not found or found with a different address)
   AddRecipientControls(addr, addrType);
}

bool wxComposeView::IsRecipientEnabled(size_t index) const
{
   return m_rcptControls[index]->IsEnabled();
}

String wxComposeView::GetRecipients(RecipientType type) const
{
   size_t count = m_rcptControls.GetCount();

   String rcpt;
   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_rcptControls[n]->GetType() == type )
      {
         if ( !rcpt.empty() )
            rcpt += CANONIC_ADDRESS_SEPARATOR;

         rcpt += m_rcptControls[n]->GetValue();
      }
   }

   return rcpt;
}

String wxComposeView::GetFrom() const
{
   String from;

   // be careful here: m_txtFrom might be not shown
   if ( m_txtFrom )
   {
      from = m_txtFrom->GetValue();
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
wxComposeView::InitText(Message *msg, MessageView *msgview)
{
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

      if ( m_template.empty() )
      {
         // this will only insert the sig
         DoInitText(NULL);
      }
      else // we have a template
      {
         // but we can't evaluate it yet because the headers values are
         // unknown, delay the evaluation
         InsertText(_("--- Please choose evaluate template from ---\n"
                      "--- the menu after filling the headers or ---\n"
                      "--- just type something in the composer ---"));
         ResetDirty();
      }
   }
   else
   {
      // replying or forwarding - evaluate the template right now
      CHECK_RET( msg, "no message in InitText" );

      DoInitText(msg, msgview);
   }

   // we also use this method to initialize the focus as we can't do it before
   // the composer text is inited

   // if the subject is already not empty (which it is when
   // replying/forwarding), put the cursor directly into the compose window,
   // otherwise let the user enter the subject first
   switch ( m_kind )
   {
      default:
         FAIL_MSG( "unknown message kind" );
         // fall through

      case Message_New:
         m_txtSubject->SetFocus();
         break;

      case Message_Reply:
      case Message_Forward:
         m_LayoutWindow->SetFocus();
   }

   Show();
}

void
wxComposeView::DoInitText(Message *mailmsg, MessageView *msgview)
{
   // first append signature - everything else will be inserted before it
   if ( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      wxTextFile fileSig;

      bool hasSign = false;
      while ( !hasSign )
      {
         String strSignFile = READ_CONFIG(m_Profile, MP_COMPOSE_SIGNATURE);
         if ( !strSignFile.empty() )
            hasSign = fileSig.Open(strSignFile);

         if ( !hasSign )
         {
            // no signature at all or sig file not found, propose to choose or
            // change it now
            wxString msg;
            if ( strSignFile.empty() )
            {
               msg = _("You haven't configured your signature file.");
            }
            else
            {
               // to show message from wxTextFile::Open()
               wxLog *log = wxLog::GetActiveTarget();
               if ( log )
                  log->Flush();

               msg.Printf(_("Signature file '%s' couldn't be opened."),
                          strSignFile.c_str());
            }

            msg += _("\n\nWould you like to choose your signature "
                     "right now (otherwise no signature will be used)?");
            if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                                     true, "AskForSig") )
            {
               strSignFile = wxPFileSelector("sig",
                                             _("Choose signature file"),
                                             NULL, ".signature", NULL,
                                             _(wxALL_FILES),
                                             0, this);
            }
            else
            {
               // user doesn't want to use signature file
               break;
            }

            if ( strSignFile.empty() )
            {
               // user canceled "choose signature" dialog
               break;
            }

            m_Profile->writeEntry(MP_COMPOSE_SIGNATURE, strSignFile);
         }
      }

      if ( hasSign )
      {
         wxLayoutList *layoutList = m_LayoutWindow->GetLayoutList();

         // insert separator optionally
         if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) )
         {
            layoutList->LineBreak();
            layoutList->Insert("--");
         }

         // read the whole file
         size_t nLineCount = fileSig.GetLineCount();
         for ( size_t nLine = 0; nLine < nLineCount; nLine++ )
         {
            layoutList->LineBreak();
            layoutList->Insert(fileSig[nLine]);
         }

         // let's respect the netiquette
         static const size_t nMaxSigLines = 4;
         if ( nLineCount > nMaxSigLines )
         {
            wxString msg;
            msg.Printf(_("Your signature is %stoo long: it should "
                        "not be more than %d lines."),
                       nLineCount > 10 ? _("way ") : "", nMaxSigLines);
            MDialog_Message(msg, m_LayoutWindow,
                            _("Signature is too long"),
                            GetPersMsgBoxName(M_MSGBOX_SIGNATURE_LENGTH));

         }

         layoutList->MoveCursorTo(wxPoint(0,0));
      }
      else
      {
         // don't ask the next time
         m_Profile->writeEntry(MP_COMPOSE_USE_SIGNATURE, false);
      }
   }

   // now deal with templates: first decide what kind of template do we need
   MessageTemplateKind kind;
   switch ( m_kind )
   {
      case Message_Reply:
         kind = m_mode == Mode_Mail ? MessageTemplate_Reply
                                    : MessageTemplate_Followup;
         break;

      case Message_Forward:
         ASSERT_MSG( m_mode == Mode_Mail, "can't forward article in news" );

         kind = MessageTemplate_Forward;
         break;

      default:
         FAIL_MSG("unknown message kind");
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

      // we will only run this loop once unless there are erros in the template
      // and the user changed it
      templateChanged = FALSE;

      // do parse the template
      if ( !ExpandTemplate(*this, m_Profile, templateValue, mailmsg, msgview) )
      {
         // first show any errors which the call to Parse() above could
         // generate
         wxLog::FlushActive();

         if ( MDialog_YesNoDialog(_("There were errors in the template. Would "
                                    "you like to edit it now?"),
                                    this,
                                    MDIALOG_YESNOTITLE,
                                    true,
                                    "FixTemplate") )
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

   // finally, attach a vCard if requested
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
                                     true, "AskForVCard") )
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
         InsertData(strutil_strdup(vcard), vcard.length(),
                    "text/x-vcard", filename);
      }
   }

   m_LayoutWindow->Refresh();
}

void wxComposeView::OnFirstTimeModify()
{
   // evaluate the template right now if we have one and it hadn't been done
   // yet (for messages of kind other than new it is done on creation)
   if ( m_kind == Message_New && !m_template.empty() )
   {
      DoClear();
      DoInitText(NULL);
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
   m_LayoutWindow->GetLayoutList()->SetFontEncoding(m_encoding);

   // check "Default" menu item if we use the system default encoding in absence
   // of any user-configured default
   CheckLanguageInMenu(this, m_encoding == wxFONTENCODING_SYSTEM
                                             ? wxFONTENCODING_DEFAULT
                                             : m_encoding);
}

void wxComposeView::SetEncodingToSameAs(Message *msg)
{
   if ( !msg )
      return;

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
   m_LayoutWindow->SetEditable(enable);
}

// ----------------------------------------------------------------------------
// wxComposeView callbacks
// ----------------------------------------------------------------------------

// use the newly specified identity
void
wxComposeView::OnIdentChange(wxCommandEvent& event)
{
   wxString ident = event.GetString();
   if ( ident != m_Profile->GetIdentity() )
   {
      if ( event.GetInt() == 0 )
      {
         // first one is always the default identity
         m_Profile->ClearIdentity();
      }
      else
      {
         m_Profile->SetIdentity(ident);
      }

      SetDefaultFrom();
   }
}

void
wxComposeView::OnRcptTypeChange(RecipientType type)
{
   // remember it as the last type and reuse it for the next recipient
   m_rcptTypeLast = type;
}

// expand (using the address books) the value of the last active text zone
void
wxComposeView::OnExpand(wxCommandEvent &WXUNUSED(event))
{
   wxAddressTextCtrl *text;

   if ( m_indexLast == -1 )
   {
      // the new recipient field
      text = m_txtRecipient;
   }
   else // m_indexLast is the index into m_rcptControls array of existing rcpts
   {
      // we know that is of the right type
      text = m_rcptControls[(size_t)m_indexLast]->GetText();
   }

   (void)text->DoExpand();
}

// can we close the window now? check the modified flag
bool
wxComposeView::CanClose() const
{
   bool canClose = true;

   // we can't close while the external editor is running (I think it will
   // lead to a nice crash later)
   if ( m_procExtEdit )
   {
      wxLogError(_("Please terminate the external editor (PID %d) before "
                   "closing this window."), m_pidEditor);

      canClose = false;
   }
   else if ( m_LayoutWindow && m_LayoutWindow->IsModified() )
   {
      // ask the user if he wants to save the changes
      canClose = MDialog_YesNoDialog
                 (
                  _("There are unsaved changes, close anyway?"),
                  this, // parent
                  MDIALOG_YESNOTITLE,
                  false, // "yes" not default
                  "UnsavedCloseAnyway"
                 );
   }
   else
   {
      canClose = true;
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
      case WXMENU_COMPOSE_SEND_LATER:
         if ( IsReadyToSend() )
         {
            if ( Send( (id == WXMENU_COMPOSE_SEND_LATER) ) )
            {
               Close();
            }
         }
         break;

      case WXMENU_COMPOSE_SEND_KEEP_OPEN:
         if ( IsReadyToSend() )
         {
            (void)Send();
         }
         break;

      case WXMENU_COMPOSE_PRINT:
         Print();
         break;

      case WXMENU_COMPOSE_CLEAR:
         DoClear();
         break;

      case WXMENU_COMPOSE_EVAL_TEMPLATE:
         if ( !m_LayoutWindow->IsModified() )
         {
            // remove our prompt
            DoClear();
         }

         DoInitText(NULL);
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
            else if ( InsertFileAsText(filename) )
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
         m_LayoutWindow->Paste( WXLO_COPY_FORMAT, FALSE );
         m_LayoutWindow->Refresh();
         break;

      case WXMENU_EDIT_COPY:
         m_LayoutWindow->Copy( WXLO_COPY_FORMAT, FALSE );
         m_LayoutWindow->Refresh();
         break;

      case WXMENU_EDIT_CUT:
         m_LayoutWindow->Cut( WXLO_COPY_FORMAT, FALSE );
         m_LayoutWindow->Refresh();
         break;

      default:
         if ( WXMENU_CONTAINS(LANG, id) )
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
   // it may happen that the message is sent before the composer gets focus,
   // avoid starting the external editor by this time!
   if ( m_sending )
      return true;

   // now we can launch the ext editor if configured to do it
   if ( READ_CONFIG(m_Profile, MP_ALWAYS_USE_EXTERNALEDITOR) )
   {
      // we're going to modify it even if the composer window doesn't know it
      OnFirstTimeModify();

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
   // TODO: our hash is quite lame actually - it is just the text length!
   unsigned long len = 0;

   wxLayoutExportObject *exp;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());

   while( (exp = wxLayoutExport(&status, WXLO_EXPORT_AS_TEXT)) != NULL )
   {
      // non text objects get ignored
      if (exp->type == WXLO_EXPORT_TEXT )
      {
         len += exp->content.text->length();
      }
   }

   return len;
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

         // 'false' means that it's ok to leave the file empty
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
               FAIL_MSG( "composer without frame?" );
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
                                  true, "AskForExtEdit") )
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
   CHECK_RET( event.GetPid() == m_pidEditor , "unknown program terminated" );

   // reenable editing the text in the built-in editor
   EnableEditing(true);

   wxFrame *frame = GetFrame();
   if ( frame )
   {
      frame->SetTitle(COMPOSER_TITLE);
   }
   else
   {
      FAIL_MSG( "composer without frame?" );
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
      // 'true' means "replace the first text part"
      if ( !InsertFileAsText(m_tmpFileName, true) )
      {
         wxLogError(_("Failed to insert back the text from external editor."));
      }
      else // InsertFileAsText() succeeded
      {
         if ( remove(m_tmpFileName) != 0 )
         {
            wxLogDebug("Stale temp file '%s' left.", m_tmpFileName.c_str());
         }

         ok = true;

         // check if the text was really changed
         if ( ComputeTextHash() == m_oldTextHash )
         {
            // assume it wasn't
            ResetDirty();

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

   // the text could have been scrolled down but it might have become shorter
   // after editing, so reset the scrollbars to ensure that it is visible
   m_LayoutWindow->Scroll(0, 0);

   // show the frame which might had been obscured by the other windows
   Raise();
}

// ----------------------------------------------------------------------------
// inserting stuff into wxComposeView
// ----------------------------------------------------------------------------

// insert any data
void
wxComposeView::InsertData(void *data,
                          size_t length,
                          const char *mimetype,
                          const char *filename)
{
   MimeContent *mc = new MimeContent();

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

   mc->SetMimeType(mimetype);
   mc->SetData(data, length, filename);

   wxIcon icon =
      mApplication->GetIconManager()->GetIconFromMimeType(mimetype,
                                                          wxString(filename).AfterLast('.'));

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   mc->DecRef();
   m_LayoutWindow->GetLayoutList()->Insert(obj);

   Refresh();
}

// insert file data
void
wxComposeView::InsertFile(const char *fileName, const char *mimetype)
{
   CHECK_RET( !strutil_isempty(fileName), "filename can't be empty" );

   MimeContent *mc = new MimeContent();

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

   String msg;
   msg.Printf(_("File '%s' seems to contain data of\n"
                "MIME type '%s'.\n"
                "Is this correct?"),
              filename.c_str(), strMimeType.c_str());
   if ( !MDialog_YesNoDialog( msg, this, _("Content MIME type"),
                              true,
                              GetProfile()->GetName()+"/MimeTypeCorrect") )
   {
      wxString newtype = strMimeType;
      if(MInputBox(&newtype, _("MIME type"),
                   _("Please enter new MIME type:"),
                   this))
         strMimeType = newtype;
   }
   mc->SetMimeType(strMimeType);
   mc->SetFile(filename);

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxIcon icon = iconManager->GetIconFromMimeType(strMimeType, strExt);

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   mc->DecRef();
   m_LayoutWindow->GetLayoutList()->Insert(obj);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"),
               filename.c_str(), strMimeType.c_str());

   m_LayoutWindow->Refresh();
}

/// insert a text file at the current cursor position
bool
wxComposeView::InsertFileAsText(const String& filename,
                                bool replaceFirstTextPart)
{
   // read the text from the file
   char *text = NULL;
   off_t lenFile = 0;      // suppress warning
   wxFile file(filename);

   bool ok = file.IsOpened();
   if ( ok )
   {
      lenFile = file.Length();
      if ( lenFile == 0 )
      {
         wxLogVerbose(_("File '%s' is empty, no text to insert."),
                      filename.c_str());
         return true;
      }

      text = new char[lenFile + 1];
      text[lenFile] = '\0';

      ok = file.Read(text, lenFile) != wxInvalidOffset;
   }

   if ( !ok )
   {
      wxLogError(_("Cannot insert text file into the message."));

      if ( text )
         delete [] text;

      return false;
   }

   // insert the text in the beginning of the message replacing the old
   // text if asked for this
   if ( replaceFirstTextPart )
   {
      // VZ: I don't know why exactly does this happen but exporting text and
      //     importing it back adds a '\n' at the end, so this is useful as a
      //     quick workaround for this bug - of course, it's not a real solution
      //     (FIXME)
      size_t index = (size_t)lenFile - 1;
      if ( text[index] == '\n' )
      {
         // check for "\r\n" too
         if ( index > 0 && text[index - 1] == '\r' )
         {
            // truncate one char before
            index--;
         }

         text[index] = '\0';
      }

      // this is not as simple as it sounds, because we normally exported all
      // the text which was in the beginning of the message and it's not one
      // text object, but possibly several text objects and line break
      // objects, so now we must delete them and then recreate the new ones...

      wxLayoutList * layoutList = m_LayoutWindow->GetLayoutList();
      wxLayoutList * other_list = new wxLayoutList;
      wxLayoutObject *obj;
      wxLayoutExportStatus status(layoutList);
      wxLayoutExportObject *exp;
      while( (exp = wxLayoutExport(&status, WXLO_EXPORT_AS_OBJECTS)) != NULL )
      {
         // ignore WXLO_EXPORT_EMPTYLINE:
         if(exp->type == WXLO_EXPORT_OBJECT)
         {
            obj = exp->content.object;
            switch(obj->GetType())
            {
            case WXLO_TYPE_TEXT:
               ; //    do nothing
               break;
            case WXLO_TYPE_ICON:
               other_list->Insert(obj->Copy());
               break;
            default:
               ; // cmd    objects get ignored
            }
         }
         delete exp;
      }
      layoutList->Empty();
      //now we move the non-text objects back:
      wxLayoutExportStatus status2(other_list);
      while((exp = wxLayoutExport( &status2,
                                      WXLO_EXPORT_AS_OBJECTS)) != NULL)
         if(exp->type == WXLO_EXPORT_EMPTYLINE)
            layoutList->LineBreak();
         else
            layoutList->Insert(exp->content.object->Copy());
      delete other_list;
   }

   // now insert the new text
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), text);
   m_LayoutWindow->ResizeScrollbars(true);
   m_LayoutWindow->SetModified();
   m_LayoutWindow->SetDirty();
   m_LayoutWindow->Refresh();
   delete [] text;
   return true;
}

/// inserts a text
void
wxComposeView::InsertText(const String &txt)
{
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), txt);
   m_LayoutWindow->ResizeScrollbars(true);
   m_LayoutWindow->SetDirty();
   m_LayoutWindow->GetLayoutList()->ForceTotalLayout();
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
      String host = READ_CONFIG(mApplication->GetProfile(), MP_SMTPHOST);
      if ( host.empty() )
      {
         if ( MDialog_YesNoDialog(
                  _("The message can not be sent because the network settings "
                    "are either not configured or incorrect. Would you like to "
                    "change them now?"),
                  this,
                  MDIALOG_YESNOTITLE,
                  true /* yes is default */,
                  "ConfigNetFromCompose") )
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

   // take into account any recipients still in the "new address" field
   m_txtRecipient->AddNewRecipients(true /* quiet */);

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
               true /* yes is default */,
               "SendemptySubject") )
      {
         m_txtSubject->SetFocus();

         return false;
      }
   }

   // everything is ok
   return true;
}

bool
wxComposeView::Send(bool schedule)
{
   CHECK( !m_sending, false, "wxComposeView::Send() reentered" );

   m_sending = true;

   Protocol proto;
   switch ( m_mode )
   {
      case Mode_Mail:
#ifdef OS_UNIX
         if ( READ_CONFIG_BOOL(m_Profile, MP_USE_SENDMAIL) )
            proto = Prot_Sendmail;
         else
#endif // OS_UNIX
            proto = Prot_SMTP;
         break;

      case Mode_News:
         proto = Prot_NNTP;
         break;

      default:
         proto = Prot_Illegal;
   }

   if ( proto == Prot_Illegal )
   {
      FAIL_MSG( "unknown protocol" );

      // we do have to use something though
      proto = Prot_Default;
   }

   // Create the message to be composed
   SendMessage *msg = SendMessage::Create(m_Profile, proto);
   if ( !msg )
   {
      // this shouldn't ever happen, but who knows...
      wxLogError(_("Failed to create the message to send."));

      return false;
   }

   wxBusyCursor bc;
   wxLogStatus(this, _("Sending message..."));
   Disable();

   // compose the body
   wxLayoutObject *lo = NULL;
   MimeContent *mc = NULL;
   wxLayoutExportObject *exp;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());
   while((exp = wxLayoutExport( &status,
                                   WXLO_EXPORT_AS_TEXT,
                                   WXLO_EXPORT_WITH_CRLF)) != NULL)
   {
      if(exp->type == WXLO_EXPORT_TEXT)
      {
         String* text = exp->content.text;
         msg->AddPart(
                        MimeType::TEXT,
                        text->c_str(), text->length(),
                        "PLAIN",
                        "INLINE",   // disposition
                        NULL,       // disposition parameters
                        NULL,       // other parameters
                        m_encoding
                     );
      }
      else
      {
         lo = exp->content.object;
         if(lo->GetType() == WXLO_TYPE_ICON)
         {
            mc = (MimeContent *)lo->GetUserData();

            switch( mc->GetType() )
            {
            case MimeContent::MIMECONTENT_FILE:
            {
               String filename = mc->GetFileName();
               wxFile file;
               if ( file.Open(filename) )
               {
                  size_t size = file.Length();
                  char *buffer = new char[size + 1];
                  if ( file.Read(buffer, size) )
                  {
                     // always NUL terminate it
                     buffer[size] = '\0';

                     String basename = wxFileNameFromPath(filename);

                     MessageParameterList plist, dlist;

                     // some mailers want "FILENAME" in disposition parameters
                     MessageParameter *p =
                         new MessageParameter("FILENAME", basename);
                     dlist.push_back(p);

                     // some mailers want "NAME" in parameters:
                     p = new MessageParameter("NAME", basename);
                     plist.push_back(p);

                     msg->AddPart
                        (
                           mc->GetMimeCategory(),
                           buffer, size,
                           strutil_after(mc->GetMimeType(),'/'), //subtype
                           "INLINE",
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

            case MimeContent::MIMECONTENT_DATA:
            {
               MessageParameterList dlist, plist;
               if(! strutil_isempty(mc->GetFileName()))
               {
                  MessageParameter *p
                      = new MessageParameter("FILENAME", mc->GetFileName());
                  dlist.push_back(p);

                  p = new MessageParameter("NAME", mc->GetFileName());
                  plist.push_back(p);
               }

               msg->AddPart
                  (
                     mc->GetMimeCategory(),
                     (char *)mc->GetData(),
                     mc->GetSize(),
                     strutil_after(mc->GetMimeType(),'/'),  //subtype
                     "INLINE",
                     &dlist,
                     &plist
                  );
            }
            break;

            default:
               FAIL_MSG(_("Unknwown part type"));
            }
            mc->DecRef();
         }
      }

      delete exp;
   }

   // setup the headers

   if ( m_encoding != wxFONTENCODING_DEFAULT )
   {
      msg->SetHeaderEncoding(m_encoding);
   }

   // first the standard ones
   msg->SetSubject(GetSubject());

   msg->SetAddresses(GetRecipients(Recipient_To),
                     GetRecipients(Recipient_Cc),
                     GetRecipients(Recipient_Bcc));

   String newsgroups = GetRecipients(Recipient_Newsgroup);
   if ( !newsgroups.empty() )
   {
      msg->SetNewsgroups(newsgroups);
   }

   String from = GetFrom();
   if ( !from.empty() && from != m_from )
   {
      msg->SetFrom(from);
   }

   // Add additional header lines: first for this time only and then also the
   // headers stored in the profile
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
      wxLogStatus(this, _("Message has been sent."));
   }
   else // message not sent
   {
      if ( mApplication->GetLastError() != M_ERROR_CANCEL )
      {
         wxLogError(_("The message couldn't be sent."));
      }
      //else: cancelled by user, don't give the error
   }

   // reenable the window disabled previously
   Enable();

   m_sending = false;

   return success;
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

/// sets From field using the current profile
void
wxComposeView::SetDefaultFrom()
{
   if ( m_txtFrom )
   {
      Address *addr = Address::CreateFromAddress(m_Profile);
      if ( addr )
      {
         m_from = addr->GetAddress();
         m_txtFrom->SetValue(m_from);

         delete addr;
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
   if ( m_LayoutWindow )
      m_LayoutWindow->SetModified(false);
}

// ----------------------------------------------------------------------------
// other wxComposeView operations
// ----------------------------------------------------------------------------

/// position the cursor in the composer window
void
wxComposeView::MoveCursorTo(int x, int y)
{
   m_LayoutWindow->GetLayoutList()->MoveCursorTo(wxPoint(x, y));
}

/// print the message
void
wxComposeView::Print(void)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else
   bool found;
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   //    set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(READ_CONFIG(m_Profile,MP_AFMPATH), false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
      wxSetAFMPath(afmpath);
#endif

   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrinter printer(& pdd);
#ifndef OS_WIN
   wxThePrintSetupData->SetAFMPath(afmpath);
#endif
   wxLayoutPrintout printout(m_LayoutWindow->GetLayoutList(),_("Mahogany: Printout"));
   if ( !printer.Print(this, &printout, TRUE)
        && printer.GetLastError() != wxPRINTER_CANCELLED )
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
   else
      (* ((wxMApp *)mApplication)->GetPrintData())
         = printer.GetPrintDialogData().GetPrintData();
}

/// save the first text part of the message to the given file
bool
wxComposeView::SaveMsgTextToFile(const String& filename) const
{
   // TODO write (and read later...) headers too!

   // write the text part of the message into a file
   wxFile file(filename, wxFile::write_append);
   if ( !file.IsOpened() )
   {
      wxLogError(_("Cannot open file for the message."));

      return false;
   }

   // export first text part of the message

   wxLayoutExportObject *exp;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());

   while( (exp = wxLayoutExport(&status, WXLO_EXPORT_AS_TEXT)) != NULL )
   {
      // non text objects get ignored
      if(exp->type == WXLO_EXPORT_TEXT)
         if ( !file.Write(*exp->content.text) )
         {
            wxLogError(_("Cannot write message to file."));

            return false;
         }
   }

   ((wxComposeView *)this)->ResetDirty(); // const_cast

   return true;
}

