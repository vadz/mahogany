/*-*- c++ -*-********************************************************
 * wxComposeView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$   *
 *******************************************************************/

#ifdef __GNUG__
#pragma  implementation "wxComposeView.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"
#include "Mcommon.h"

#ifndef USE_PCH
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

#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/mimetype.h>
#include <wx/persctrl.h>

#include "Mdefaults.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "Message.h"
#include "MailFolderCC.h"
#include "MessageCC.h"
#include "SendMessageCC.h"

#include "MDialogs.h"
#include "HeadersDialogs.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"

#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxComposeView.h"

#include "adb/AdbEntry.h"
#include "adb/AdbManager.h"

// incredible, but true: cclient headers #define the symbol write...
#undef write

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   IDB_EXPAND = 100
};

#define CANONIC_ADDRESS_SEPARATOR   ", "

// code here was written with assumption that x and y margins are the same
#define LAYOUT_MARGIN LAYOUT_X_MARGIN

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return TRUE if the field should have address expansion
static inline bool IsAddressField(size_t field)
{
   return field != wxComposeView::Field_Subject;
}

// ----------------------------------------------------------------------------
// private classes
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
   void SetData(char *data, size_t length,
                const char *filename = NULL); // we'll delete data!
   void SetFile(const String& filename);

   // accessors
   MimeContentType GetType() const { return m_Type; }

   const String& GetMimeType() const { return m_MimeType; }
   Message::ContentType GetMimeCategory() const { return m_NumericMimeType; }

   const String& GetFileName() const
      { return m_FileName; }

   const char *GetData() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Data; }
   size_t GetSize() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Length; }

protected:
   ~MimeContent()
      {
         if ( m_Type == MIMECONTENT_DATA )
            delete [] m_Data;
      }


private:
   MimeContentType m_Type;

   char     *m_Data;
   size_t    m_Length;
   String    m_FileName;

   Message::ContentType m_NumericMimeType;
   String               m_MimeType;
};

// specialized text control which processes TABs to expand the text it
// contains and also notifies parent (i.e. wxComposeView) when it is
// modified.
//
// NB: use <Enter> to change the current control when wxAddressTextCtrl
//     has focus (TAB won't work!)
class wxAddressTextCtrl : public wxTextCtrl
{
public:
   // ctor
   wxAddressTextCtrl(wxComposeView *composeView,
                     wxComposeView::AddressField id,
                     wxWindow *parent)
      : wxTextCtrl(parent, -1, "",
                   wxDefaultPosition, wxDefaultSize,
                   wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB)
   {
      m_composeView = composeView;
      m_id = id;
      m_lookupMode = (READ_CONFIG(m_composeView->GetProfile(),
                                  MP_ADB_SUBSTRINGEXPANSION) )
                     ? AdbLookup_Substring
                     : AdbLookup_StartsWith;
   }

   // expand the text in the control using the address book(s): returns FALSE
   // if no expansion took place
   bool DoExpand();

   // callbacks
   void OnFocusSet(wxFocusEvent& event);
   void OnChar(wxKeyEvent& event);
   void OnEnter(wxCommandEvent& event);

private:
   wxComposeView              *m_composeView;
   wxComposeView::AddressField m_id;

   int  m_lookupMode;

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
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAddressTextCtrl, wxTextCtrl)
   EVT_SET_FOCUS(wxAddressTextCtrl::OnFocusSet)
   EVT_CHAR(wxAddressTextCtrl::OnChar)
   EVT_TEXT_ENTER(-1, wxAddressTextCtrl::OnEnter)
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

   // determin the numeric type
   String category = mimeType.Before('/');
   if ( category.CmpNoCase("VIDEO") == 0 )
      m_NumericMimeType = Message::MSG_TYPEVIDEO;
   else if ( category.CmpNoCase("AUDIO") == 0 )
      m_NumericMimeType = Message::MSG_TYPEAUDIO;
   else if ( category.CmpNoCase("IMAGE") == 0 )
      m_NumericMimeType = Message::MSG_TYPEIMAGE;
   else if ( category.CmpNoCase("TEXT") == 0 )
      m_NumericMimeType = Message::MSG_TYPETEXT;
   else if ( category.CmpNoCase("MESSAGE") == 0 )
      m_NumericMimeType = Message::MSG_TYPEMESSAGE;
   else if ( category.CmpNoCase("APPLICATION") == 0 )
      m_NumericMimeType = Message::MSG_TYPEAPPLICATION;
   else if ( category.CmpNoCase("MODEL") == 0 )
      m_NumericMimeType = Message::MSG_TYPEMODEL;
   else
      m_NumericMimeType = Message::MSG_TYPEOTHER;
}

void MimeContent::SetData(char *data, size_t length, const char *filename)
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
   ASSERT( !filename.IsEmpty() );

   m_FileName = filename;
   m_Type = MIMECONTENT_FILE;
}

// ----------------------------------------------------------------------------
// wxAddressTextCtrl
// ----------------------------------------------------------------------------

// pass to the next control when <Enter> is pressed
void wxAddressTextCtrl::OnEnter(wxCommandEvent& /* event */)
{
    wxNavigationKeyEvent event;
    event.SetDirection(TRUE);       // forward
    event.SetWindowChange(FALSE);   // control change
    event.SetEventObject(this);

    GetEventHandler()->ProcessEvent(event);
}

// mark this ctrl as being the last active - so the [Expand] btn will expand us
void wxAddressTextCtrl::OnFocusSet(wxFocusEvent& event)
{
   m_composeView->SetLastAddressEntry(m_id);

   event.Skip();
}

// expand the address when <TAB> is pressed
void wxAddressTextCtrl::OnChar(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   m_composeView->SetLastAddressEntry(m_id);

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
         if ( !GetValue().IsEmpty() )
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

bool wxAddressTextCtrl::DoExpand()
{
   // try to expand the last component
   String text = GetValue();
   text.Trim(FALSE); // trim spaces from both sides
   text.Trim(TRUE);

   // check for the lone '"' simplifies the code for finding the starting
   // position below: it should be done here, otherwise the following loop
   // will crash!
   if ( text.IsEmpty() || text == '"' )
   {
      // don't do anything
      wxLogStatus(m_composeView,
                  _("Nothing to expand - please enter something."));

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
   else
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

   wxArrayString expansions;
   if ( AdbExpand(expansions, text.c_str() + nLastAddr, m_lookupMode, m_composeView) )
   {
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

      // take what was there before...
      wxString newText(text, nPrevAddrEnd);  // first nPrevAddrEnd chars
      if ( !newText.IsEmpty() )
      {
         // there was something before, add separator
         newText += CANONIC_ADDRESS_SEPARATOR;
      }

      // ... and and the replacement string(s)
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

      SetValue(newText);
      SetInsertionPointEnd();
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxComposeView inline functions
// ----------------------------------------------------------------------------

void wxComposeView::EnableEditing(bool enable)
{
   // indicate the current state in the status bar
   SetStatusText(enable ? "" : "RO", 1);
   m_LayoutWindow->SetEditable(enable);
}

// ----------------------------------------------------------------------------
// wxComposeView creation
// ----------------------------------------------------------------------------
/** Constructor for posting news.
    @param parentProfile parent profile
    @param parent parent window
    @param hide if true, do not show frame
    @return pointer to the new compose view
*/
wxComposeView *
wxComposeView::CreateNewArticle(wxWindow *parent,
                                ProfileBase *parentProfile,
                                bool hide)
{
   wxComposeView *cv = new wxComposeView("_ComposeViewNews", parent);
   cv->m_mode = Mode_NNTP;
   cv->SetTitle(_("Article Composition"));
   cv->Create(parent,parentProfile);
   return cv;
}

/** Constructor for sending mail.
    @param parentProfile parent profile
    @param parent parent window
    @param hide if true, do not show frame
    @return pointer to the new compose view
*/
wxComposeView *
wxComposeView::CreateNewMessage(wxWindow *parent,
                                        ProfileBase *parentProfile,
                                        bool hide)
{
   wxComposeView *cv = new wxComposeView("_ComposeViewMail", parent);
   cv->m_mode = Mode_SMTP;
   cv->SetTitle(_("Message Composition"));
   cv->Create(parent,parentProfile);
   return cv;
}

void
wxComposeView::Create(wxWindow * WXUNUSED(parent),
                      ProfileBase *parentProfile,
                      bool hide)
{
   CHECK_RET( !initialised, "wxComposeView created twice" );

   m_LayoutWindow = NULL;
   nextFileID = 0;

   if(!parentProfile)
      parentProfile = mApplication->GetProfile();
   m_Profile = ProfileBase::CreateProfile(m_name,parentProfile);

   // build menu
   // ----------
   AddFileMenu();
   AddEditMenu();
   WXADD_MENU(m_MenuBar, COMPOSE, _("&Compose"));
   AddHelpMenu();
   SetMenuBar(m_MenuBar);

   // FIXME: provide some visual feedback for them, like
   // enabling/disabling them. Not used yet.
   m_MItemCut = m_MenuBar->FindItem(WXMENU_EDIT_CUT);
   m_MItemCopy = m_MenuBar->FindItem(WXMENU_EDIT_COPY);
   m_MItemPaste = m_MenuBar->FindItem(WXMENU_EDIT_CUT);

   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_COMPOSE);

   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);

   // create the child controls
   // -------------------------

   // NB: order of item creation is important for constraints algorithm:
   // create first the controls whose constraints can be fixed first!
   wxLayoutConstraints *c;
   SetAutoLayout(TRUE);

   // Panel itself (fills all the frame client area)
   m_panel = new wxPanel(this, -1);//,-1);
   c = new wxLayoutConstraints;
   c->top.SameAs(this, wxTop);
   c->left.SameAs(this, wxLeft);
   c->width.SameAs(this, wxWidth);
   c->height.SameAs(this, wxHeight);
   m_panel->SetConstraints(c);
   m_panel->SetAutoLayout(TRUE);

   // box which contains all the header fields
   wxStaticBox *box = new wxStaticBox(m_panel, -1, "");
   c = new wxLayoutConstraints;
   c->left.SameAs(m_panel, wxLeft, LAYOUT_MARGIN);
   c->right.SameAs(m_panel, wxRight, LAYOUT_MARGIN);
   c->top.SameAs(m_panel, wxTop, LAYOUT_MARGIN);
   box->SetConstraints(c);

   // layout the labels and text fields: label at the left of the field
   bool bDoShow[Field_Max];
   bDoShow[Field_To] =
   bDoShow[Field_Subject] = TRUE;  // To and subject always there
   if(m_mode == Mode_SMTP)
   {
      bDoShow[Field_Cc] = READ_CONFIG(m_Profile, MP_SHOWCC) != 0;
      bDoShow[Field_Bcc] = READ_CONFIG(m_Profile, MP_SHOWBCC) != 0;
   }
   else
      bDoShow[Field_Cc] = bDoShow[Field_Bcc] = FALSE; // never for news


   // first determine the longest label caption
   const char *aszLabels[Field_Max];

   aszLabels[Field_To]      = (m_mode == Mode_SMTP) ?  _("To:") : _("Newsgroups:");
   aszLabels[Field_Subject] = _("Subject:");
   aszLabels[Field_Cc]      = _("CC:");
   aszLabels[Field_Bcc]     = _("BCC:");

   // don't forget to update the labels when you add a new field
   ASSERT( WXSIZEOF(aszLabels) == Field_Max );

   wxClientDC dc(this);
   long width, widthMax = 0;
   size_t n;
   for ( n = 0; n < Field_Max; n++ )
   {
      if ( !bDoShow[n] )
         continue;

      dc.GetTextExtent(aszLabels[n], &width, NULL);
      if ( width > widthMax )
         widthMax = width;
   }
   widthMax += LAYOUT_MARGIN;

   wxStaticText *label;
   wxWindow  *last = NULL;
   for ( n = 0; n < Field_Max; n++ )
   {
      if ( bDoShow[n] )
      {
         if ( IsAddressField(n) ) {
            m_txtFields[n] = new wxAddressTextCtrl(this, (AddressField)n,
                                                   m_panel);
         }
         else {
            // fields which don't contain addresses don't need completition
            m_txtFields[n] = new wxTextCtrl(m_panel, -1, "");
         }

         // text entry goes from right border of the label to the right border
         // of the box except for the first one where we must also leave space
         // for the button
         c = new wxLayoutConstraints;
         c->left.Absolute(widthMax + LAYOUT_MARGIN);
         if ( last )
            c->top.Below(last, LAYOUT_MARGIN);
         else
            c->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
         if ( n == Field_To ) {
            // the [Expand] button
            wxButton *btnExpand = new wxButton(m_panel, IDB_EXPAND, _("Expand"));
            wxLayoutConstraints *c2 = new wxLayoutConstraints;
            c2->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
            c2->right.SameAs(box, wxRight, LAYOUT_MARGIN);
            c2->width.AsIs();
            c2->height.AsIs();
            btnExpand->SetConstraints(c2);

#if wxUSE_TOOLTIPS
            btnExpand->SetToolTip(_("Expand the address using address books"));
#endif // tooltips

            c->right.LeftOf(btnExpand, LAYOUT_MARGIN);
         }
         else
            c->right.SameAs(box, wxRight, LAYOUT_MARGIN);
         c->height.AsIs();
         m_txtFields[n]->SetConstraints(c);

         last = m_txtFields[n];

         // label is aligned to the left
         label = new wxStaticText(m_panel, -1, aszLabels[n],
                                  wxDefaultPosition, wxDefaultSize,
                                  wxALIGN_RIGHT);
         c = new wxLayoutConstraints;
         c->left.SameAs(box, wxLeft, LAYOUT_MARGIN);
         c->right.Absolute(widthMax);
         c->centreY.SameAs(last, wxCentreY, 2); // @@ looks better with 2...
         c->height.AsIs();
         label->SetConstraints(c);
      }
      else
      { // not shown
         m_txtFields[n] = NULL;
      }
   }

   // now we can fix the bottom of the box
   box->GetConstraints()->bottom.Below(last, -LAYOUT_MARGIN);

   // compose window
   CreateFTCanvas(); // creates a m_LayoutWindow
   m_LayoutWindow->SetStatusBar(GetStatusBar(),0,1);
   m_LayoutWindow->SetCursorVisibility(1);

   c = new wxLayoutConstraints;
   c->left.SameAs(m_panel, wxLeft, LAYOUT_MARGIN);
   c->right.SameAs(m_panel, wxRight, LAYOUT_MARGIN);
   c->top.Below(box, LAYOUT_MARGIN);
   c->bottom.SameAs(m_panel, wxBottom, LAYOUT_MARGIN);
   m_LayoutWindow->SetConstraints(c);

   // initialize the controls
   // -----------------------

   // set def values for the headers
   if(m_txtFields[Field_To])
      m_txtFields[Field_To]->SetValue(READ_CONFIG(m_Profile, MP_COMPOSE_TO));
   if(m_txtFields[Field_Cc])
      m_txtFields[Field_Cc]->SetValue(READ_CONFIG(m_Profile, MP_COMPOSE_CC));
   if(m_txtFields[Field_Bcc])
      m_txtFields[Field_Bcc]->SetValue(READ_CONFIG(m_Profile, MP_COMPOSE_BCC));

   // append signature
   if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      wxTextFile fileSig;

      bool hasSign = false;
      while ( !hasSign )
      {
         String strSignFile = READ_CONFIG(m_Profile, MP_COMPOSE_SIGNATURE);
         if ( !strSignFile.IsEmpty() )
            hasSign = fileSig.Open(strSignFile);

         if ( !hasSign )
         {
            // no signature at all or sig file not found, propose to choose or
            // change it now
            wxString msg;
            if ( strSignFile.IsEmpty() )
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
                                             _("All files (*.*)|*.*"),
                                             0, this);
            }
            else
            {
               // user doesn't want to use signature file
               break;
            }

            if ( strSignFile.IsEmpty() )
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
         if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) ) {
            layoutList->LineBreak();
            layoutList->Insert("--");
            layoutList->LineBreak();
         }

         // read the whole file
         size_t nLineCount = fileSig.GetLineCount();
         for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
            layoutList->Insert(fileSig[nLine]);
            layoutList->LineBreak();;
         }

         // let's respect the netiquette
         static const size_t nMaxSigLines = 4;
         if ( nLineCount > nMaxSigLines ) {
            wxLogWarning(_("Your signature is %stoo long: it should "
                           "not be more than %d lines."),
                         nLineCount > 10 ? _("way ") : "", nMaxSigLines);
         }

         layoutList->MoveCursorTo(wxPoint(0,0));
      }
      else
      {
         // don't ask the next time
         m_Profile->writeEntry(MP_COMPOSE_USE_SIGNATURE, false);
      }
   }

   // show the frame
   // --------------
   initialised = true;
   if ( !hide ) {
      Show(TRUE);
      m_txtFields[Field_To]->SetFocus();
   }
}

#define NUM_FONTS 7
static int wxFonts[NUM_FONTS] =
{
  wxDEFAULT,
  wxDECORATIVE,
  wxROMAN,
  wxSCRIPT,
  wxSWISS,
  wxMODERN,
  wxTELETYPE
};

void
wxComposeView::CreateFTCanvas(void)
{
   wxASSERT(m_LayoutWindow == NULL);
   m_LayoutWindow = new wxLayoutWindow(m_panel);

   wxString colourName;

   GetColourByName(&m_fg, READ_CONFIG(m_Profile, MP_CVIEW_FGCOLOUR), "black");
   GetColourByName(&m_bg, READ_CONFIG(m_Profile, MP_CVIEW_BGCOLOUR), "white");

   m_font = READ_CONFIG(m_Profile,MP_CVIEW_FONT);
   ASSERT( m_font >= 0 && m_font <= NUM_FONTS );

   m_font = wxFonts[m_font];
   m_size = READ_CONFIG(m_Profile,MP_CVIEW_FONT_SIZE);

   EnableEditing(true);
   m_LayoutWindow->Clear(m_font, m_size, (int) wxNORMAL, (int)wxNORMAL, 0, &m_fg, &m_bg);
   m_LayoutWindow->SetWrapMargin( READ_CONFIG(m_Profile, MP_COMPOSE_WRAPMARGIN));
}

wxComposeView::wxComposeView(const String &iname,
                             wxWindow *parent)
             : wxMFrame(iname,parent)
{
   initialised = false;

   m_name = iname;
   m_pidEditor = 0;
   m_procExtEdit = NULL;
   m_fieldLast = Field_Max;
   m_sent = false;
}

wxComposeView::~wxComposeView()
{
   if ( initialised )
   {
      m_Profile->DecRef();
      delete m_LayoutWindow;
   }
}

// ----------------------------------------------------------------------------
// wxComposeView callbacks
// ----------------------------------------------------------------------------

// expand (using the address books) the value of the last active text zone
void
wxComposeView::OnExpand(wxCommandEvent &WXUNUSED(event))
{
   if ( m_fieldLast == Field_Max )
   {
      // assume "To:" by default
      m_fieldLast = Field_To;
   }

   wxCHECK_RET( IsAddressField(m_fieldLast), "no expansion for this field" );

   (void)((wxAddressTextCtrl *)m_txtFields[m_fieldLast])->DoExpand();
}

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
                  false // "yes" not default
                 );
   }
   else
   {
      canClose = true;
   }

   return canClose;
}

void
wxComposeView::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_COMPOSE_INSERTFILE:
      InsertFile();
      break;

   case WXMENU_COMPOSE_SEND:
      if ( IsReadyToSend() )
      {
         if ( Send() )
         {
            ResetDirty();
            Close();
         }
      }
      break;

   case WXMENU_COMPOSE_SEND_KEEP_OPEN:
      if ( IsReadyToSend() )
      {
         if ( Send() )
         {
            ResetDirty();
            MDialog_Message(_("Message has been sent."),
                            this, _("Message sent"), "MessageSent");

         }
      }
      break;
   case WXMENU_COMPOSE_PRINT:
      Print();
      break;

   case WXMENU_COMPOSE_CLEAR:
      m_LayoutWindow->Clear(m_font, m_size, (int) wxNORMAL, (int)
                            wxNORMAL, 0,
                            &m_fg, &m_bg);
      break;

   case WXMENU_COMPOSE_LOADTEXT:
      {
         String filename = wxPFileSelector("MsgInsertText",
                                           _("Please choose a file to insert."),
                                           NULL, "dead.letter", NULL,
                                           _("All files (*.*)|*.*"),
                                           wxOPEN | wxHIDE_READONLY,
                                           this);

         if ( filename.IsEmpty() )
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
         String filename = wxPFileSelector("MsgSaveText",
                                           _("Choose file to save message to"),
                                           NULL, "dead.letter", NULL,
                                           _("All files (*.*)|*.*"),
                                           wxSAVE | wxOVERWRITE_PROMPT,
                                           this);

         if ( filename.IsEmpty() )
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
      {
         if ( m_procExtEdit )
         {
            wxLogError(_("External editor is already running (PID %d)"),
                       m_pidEditor);

            break;
         }

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
               if ( !SaveMsgTextToFile(tmpFileName.GetName(), false) )
               {
                  wxLogError(_("Failed to pass message to external editor."));

                  break;
               }

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
               else
               {
                  tmpFileName.Ok();
                  m_tmpFileName = tmpFileName.GetName();

                  wxLogStatus(this, _("Started external editor (PID %d)"),
                              m_pidEditor);

                  // disable editing with the internal editor to avoid
                  // interference with the external one
                  EnableEditing(false);
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
                                 _("Enter the command name (%%s will be "
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
      }
      break;

   case WXMENU_COMPOSE_CUSTOM_HEADERS:
      {
         String headerName, headerValue;
         bool storedInProfile;
         if ( ConfigureCustomHeader(m_Profile, this,
                                    &headerName, &headerValue,
                                    &storedInProfile,
                                    m_mode == Mode_NNTP ? CustomHeader_News
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
         (m_mode == Mode_NNTP)?
         MH_COMPOSE_MAIL : MH_COMPOSE_NEWS, this);
      break;
   case WXMENU_EDIT_PASTE:
      m_LayoutWindow->Paste();
      m_LayoutWindow->Refresh();
      break;
   case WXMENU_EDIT_COPY:
      m_LayoutWindow->Copy();
      m_LayoutWindow->Refresh();
      break;
   case WXMENU_EDIT_CUT:
      m_LayoutWindow->Cut();
      m_LayoutWindow->Refresh();
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
}

void wxComposeView::OnExtEditorTerm(wxProcessEvent& event)
{
   // reenable editing the text in the built-in editor
   EnableEditing(true);

   CHECK_RET( event.GetPid() == m_pidEditor , "unknown program terminated" );

   bool ok = false;

   // check the return code of the editor process
   if ( event.GetExitCode() != 0 )
   {
      wxLogError(_("External editor terminated with non null exit code."));
   }
   else
   {
      // 'true' means "replace the first text part"
      if ( !InsertFileAsText(m_tmpFileName, true) )
      {
         wxLogError(_("Failed to insert back the text from external editor."));
      }
      else
      {
         if ( remove(m_tmpFileName) != 0 )
         {
            wxLogDebug("Stale temp file '%s' left.", m_tmpFileName.c_str());
         }

         ok = true;
         wxLogStatus(this, _("Inserted text from external editor."));
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
}

// ----------------------------------------------------------------------------
// wxComposeView operations
// ----------------------------------------------------------------------------

void
wxComposeView::InsertData(char *data,
                          size_t length,
                          const char *mimetype,
                          const char *filename)
{
   MimeContent *mc = new MimeContent();

   if(strutil_isempty(mimetype))
      mimetype = "APPLICATION/OCTET-STREAM";


   mc->SetMimeType(mimetype);
   mc->SetData(data, length, filename);

   wxIcon icon = mApplication->GetIconManager()->GetIconFromMimeType(mimetype);

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   mc->DecRef();
   m_LayoutWindow->GetLayoutList()->Insert(obj);

   Refresh();
}

void
wxComposeView::InsertFile(const char *fileName, const char *mimetype)
{
   MimeContent
      *mc = new MimeContent();

   String filename(fileName);
   if( strutil_isempty(filename) )
   {
      filename = wxPFileSelector("MsgInsert",
                                 _("Please choose a file to insert."),
                                 NULL, "dead.letter", NULL,
                                 _("All files (*.*)|*.*"),
                                 wxOPEN | wxHIDE_READONLY | wxFILE_MUST_EXIST,
                                 this);

      if( !filename )
      {
         // empty string means it was cancelled by user
         return;
      }
   }

   // if there is a slash after the dot, it is not extension (otherwise it
   // might be not an extension too, but consider that it is - how can we
   // decide otherwise?)
   String strExt = filename.AfterLast('.');
   if ( strExt == filename || strchr(strExt, '/') )
      strExt.Empty();

   String strMimeType;
   if ( strutil_isempty(mimetype) )
   {
      wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
      wxFileType *fileType = mimeManager.GetFileTypeFromExtension(strExt);
      if ( (fileType == NULL) || !fileType->GetMimeType(&strMimeType) )
      {
         // can't find MIME type from file extension, set some default one
         strMimeType = "APPLICATION/OCTET-STREAM";
      }

      delete fileType;  // may be NULL, ok
   }
   else
   {
      strMimeType = mimetype;
   }

   mc->SetMimeType(strMimeType);
   mc->SetFile(filename);

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxIcon icon = iconManager->GetIconFromMimeType(strMimeType);

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   mc->DecRef();
   m_LayoutWindow->GetLayoutList()->Insert(obj);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"),
               filename.c_str(), strMimeType.c_str());

   Refresh();
}

bool
wxComposeView::Send(void)
{
   bool success = false;
   String
      tmp2, mimeType, mimeSubType;

   wxLayoutObject *lo = NULL;
   MimeContent *mc = NULL;
   wxLayoutExportObject *export;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());

   /// The message to be composed.
   class SendMessageCC * msg = NULL;

   switch(m_mode)
   {
   case Mode_SMTP:
      msg = new SendMessageCC(m_Profile, SendMessageCC::Prot_SMTP);
      break;
   case Mode_NNTP:
      msg = new SendMessageCC(m_Profile, SendMessageCC::Prot_NNTP);
      break;
   }

   ASSERT(msg);
   while((export = wxLayoutExport( &status,
                                   WXLO_EXPORT_AS_TEXT,
                                   WXLO_EXPORT_WITH_CRLF)) != NULL)
   {
      if(export->type == WXLO_EXPORT_TEXT)
      {
         String* text = export->content.text;
         msg->AddPart
            (
               Message::MSG_TYPETEXT,
               text->c_str(), text->length(),
               "PLAIN"
               );
      }
      else
      {
         lo = export->content.object;
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
                  char *buffer = new char[size];
                  if ( file.Read(buffer, size) )
                  {
                     MessageParameterList dlist;
                     MessageParameter *p = new MessageParameter;
                     p->name = "FILENAME";
                     p->value = wxFileNameFromPath(filename);
                     dlist.push_back(p);

                     msg->AddPart
                        (
                           mc->GetMimeCategory(),
                           buffer, size,
                           strutil_after(mc->GetMimeType(),'/'), //subtype
                           "INLINE",
                           &dlist
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
               MessageParameterList dlist;
               if(! strutil_isempty(mc->GetFileName()))
               {
                  MessageParameter *p = new MessageParameter;
                  p->name = "FILENAME";
                  p->value = wxFileNameFromPath(mc->GetFileName());
                  dlist.push_back(p);
               }
               msg->AddPart
                  (
                     mc->GetMimeCategory(),
                     mc->GetData(), mc->GetSize(),
                     strutil_after(mc->GetMimeType(),'/'),  //subtype
                     "INLINE"
                     ,&dlist,
                     NULL
                     );
            }
            break;

            default:
               FAIL_MSG(_("Unknwown part type"));
            }
            mc->DecRef();
         }
      }

      delete export;
   }

   // Add additional header lines: first for this time only and then also the
   // headers stored in the profile
   kbStringList::iterator
      i, i2;
   i = m_ExtraHeaderLinesNames.begin();
   i2 = m_ExtraHeaderLinesValues.begin();
   for(; i != m_ExtraHeaderLinesNames.end(); i++, i2++)
      msg->AddHeaderEntry(**i, **i2);

   wxArrayString headerNames, headerValues;
   size_t nHeaders = GetCustomHeaders(m_Profile,
                                      m_mode == Mode_SMTP ? CustomHeader_Mail
                                                          : CustomHeader_News,
                                      &headerNames,
                                      &headerValues);
   for ( size_t nHeader = 0; nHeader < nHeaders; nHeader++ )
   {
      msg->AddHeaderEntry(headerNames[nHeader], headerValues[nHeader]);
   }

   msg->SetSubject(m_txtFields[Field_Subject]->GetValue());
   switch(m_mode)
   {
      case Mode_SMTP:
      {
         // although 'To' field is always present, the others may not be shown
         // (nor created) at all
         String to = m_txtFields[Field_To]->GetValue();
         String cc, bcc;
         if ( m_txtFields[Field_Cc] )
            cc = m_txtFields[Field_Cc]->GetValue();
         if ( m_txtFields[Field_Bcc] )
            bcc = m_txtFields[Field_Bcc]->GetValue();

         msg->SetAddresses(to, cc, bcc);
      }
      break;

      case Mode_NNTP:
         msg->SetNewsgroups(m_txtFields[Field_To]->GetValue());
         break;
   }

   success = msg->Send();  // true if sent

   if ( success && READ_CONFIG(m_Profile,MP_USEOUTGOINGFOLDER) )
   {
      msg->WriteToFolder(READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER),
                         MF_PROFILE_OR_FILE);
   }

   delete msg;

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


/// sets To field
void
wxComposeView::SetAddresses(const String &to,
                            const String &cc,
                            const String &bcc)
{
   wxTextCtrl *text;

   if ( !!to )
   {
      text = m_txtFields[Field_To];
      if ( text )
         text->SetValue(WXCPTR to.c_str());
   }

   if ( !!cc )
   {
      text = m_txtFields[Field_Cc];
      if ( text )
         text->SetValue(WXCPTR cc.c_str());
   }

   if ( !!bcc )
   {
      text = m_txtFields[Field_Bcc];
      if ( text )
         text->SetValue(WXCPTR bcc.c_str());
   }
}

/// sets Subject field
void
wxComposeView::SetSubject(const String &subj)
{
   m_txtFields[Field_Subject]->SetValue(WXCPTR subj.c_str());
}

void wxComposeView::ResetDirty()
{
   if ( m_LayoutWindow )
      m_LayoutWindow->SetModified(false);
}

/// inserts a text
void
wxComposeView::InsertText(const String &txt)
{
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), txt);
   m_LayoutWindow->ResizeScrollbars(true);
   m_LayoutWindow->SetModified();
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
   wxPrinter printer;
   wxLayoutPrintout printout(m_LayoutWindow->GetLayoutList(),_("Mahogany: Printout"));
   if (! printer.Print(this, &printout, TRUE))
      wxMessageBox(
         _("There was a problem with printing the message:\n"
           "perhaps your current printer is not set up correctly?"),
         _("Printing"), wxOK);
}

// -----------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------

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
      if ( host.IsEmpty() )
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

   // did we forget the recipients?
   if ( m_txtFields[Field_To]->GetValue().IsEmpty() )
   {
      wxLogError(_("Please specify at least one recipient in the \"To:\" "
                   "field!"));

      return false;
   }

   // did we forget the subject?
   if ( m_txtFields[Field_Subject]->GetValue().IsEmpty() )
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
         m_txtFields[Field_Subject]->SetFocus();

         return false;
      }
   }

   // everything is ok
   return true;
}

/// insert a text file at the current cursor position
bool
wxComposeView::InsertFileAsText(const String& filename,
                                bool replaceFirstTextPart)
{
   // read the text from the file
   char *text = NULL;
   wxFile file(filename);

   bool ok = file.IsOpened();
   if ( ok )
   {
      off_t lenFile = file.Length();
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
      // this is not as simple as it sounds, because we normally exported all
      // the text which was in the beginning of the message and it's not one
      // text object, but possibly several text objects and line break
      // objects, so now we must delete them and then recreate the new ones...

      wxLayoutList * layoutList = m_LayoutWindow->GetLayoutList();
      wxLayoutList * other_list = new wxLayoutList;
      wxLayoutObject *obj;
      wxLayoutExportStatus status(layoutList);
      wxLayoutExportObject *export;
      while((export = wxLayoutExport( &status,
                                      WXLO_EXPORT_AS_OBJECTS)) != NULL)
      {
         // ignore WXLO_EXPORT_EMPTYLINE:
         if(export->type == WXLO_EXPORT_OBJECT)
         {
            obj = export->content.object;
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
         delete export;
      }
      layoutList->Empty();
      //now we move the non-text objects back:
      wxLayoutExportStatus status2(other_list);
      while((export = wxLayoutExport( &status2,
                                      WXLO_EXPORT_AS_OBJECTS)) != NULL)
         if(export->type == WXLO_EXPORT_EMPTYLINE)
            layoutList->LineBreak();
         else
            layoutList->Insert(export->content.object->Copy());
      delete other_list;
   }

   // now insert the new text
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), text);
   m_LayoutWindow->ResizeScrollbars(true);
   m_LayoutWindow->SetModified();
   m_LayoutWindow->Refresh();
   delete [] text;
   return true;
}

/// save the first text part of the message to the given file
bool
wxComposeView::SaveMsgTextToFile(const String& filename,
                                 bool errorIfNoText) const
{
   // TODO write (and read later...) headers too!

   // write the text part of the message into a file
   wxFile file(filename, wxFile::write);

   // export first text part of the message

   wxLayoutExportObject *export;
   wxLayoutExportStatus status(m_LayoutWindow->GetLayoutList());

   while((export = wxLayoutExport( &status, WXLO_EXPORT_AS_TEXT)) != NULL)
   {
      // non text objects get ignored
      if(export->type == WXLO_EXPORT_TEXT)
         if ( !file.Write(*export->content.text) )
         {
            wxLogError(_("Cannot write message to file '%s'."),
                       filename.c_str());

            return false;
         }
   }

   ((wxComposeView *)this)->ResetDirty(); // const_cast

   return true;
}

