/*-*- c++ -*-********************************************************
 * wxComposeView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$        *
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

#  include "PathFinder.h"
#  include "Profile.h"

#  include "MFrame.h"

#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include <ctype.h>          // for isspace()
#endif

#include "Mdefaults.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "Message.h"
#include "MailFolderCC.h"
#include "MessageCC.h"
#include "SendMessageCC.h"

#include "MDialogs.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"

#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxComposeView.h"

#include <wx/textfile.h>
#include <wx/mimetype.h>

#include "adb/AdbEntry.h"
#include "adb/AdbManager.h"

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
// private classes
// ----------------------------------------------------------------------------

class MimeContent : public wxLayoutObjectBase::UserData
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
   MimeContent() { m_Type = MIMECONTENT_NONE; }
   ~MimeContent()
      {
         if ( m_Type == MIMECONTENT_DATA )
            delete m_Data;
      }

   // initialize
   void SetMimeType(const String& mimeType);
   void SetData(char *data, size_t length); // we'll delete data!
   void SetFile(const String& filename);

   // accessors
   MimeContentType GetType() const { return m_Type; }

   const String& GetMimeType() const { return m_MimeType; }
   Message::ContentType GetMimeCategory() const { return m_NumericMimeType; }

   const String& GetFileName() const
      { ASSERT( m_Type == MIMECONTENT_FILE ); return m_FileName; }

   const char *GetData() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Data; }
   size_t GetSize() const
      { ASSERT( m_Type == MIMECONTENT_DATA ); return m_Length; }

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
                   wxTE_PROCESS_ENTER)
   {
      m_composeView = composeView;
      m_id = id;
   }

   // callbacks
   void OnChar(wxKeyEvent& event);
   void OnEnter(wxCommandEvent& event);

   #ifdef __WXMSW__
      // if we don't return this, we won't get TABs in OnChar() events
      long wxAddressTextCtrl::MSWGetDlgCode()
        { return DLGC_WANTTAB | wxTextCtrl::MSWGetDlgCode(); }
   #endif //MSW

private:
   wxComposeView              *m_composeView;
   wxComposeView::AddressField m_id;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables &c
// ----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(wxComposeView, wxMFrame)

BEGIN_EVENT_TABLE(wxComposeView, wxMFrame)
   // wxComposeView menu events
   EVT_MENU(WXMENU_COMPOSE_INSERTFILE, wxComposeView::OnInsertFile)
   EVT_MENU(WXMENU_COMPOSE_SEND,       wxComposeView::OnSend)
   EVT_MENU(WXMENU_COMPOSE_PRINT,      wxComposeView::OnPrint)
   EVT_MENU(WXMENU_COMPOSE_CLEAR,      wxComposeView::OnClear)

   // button notifications
   EVT_BUTTON(IDB_EXPAND, wxComposeView::OnExpand)

   // can we close now?
   EVT_CLOSE(wxComposeView::OnCloseWindow)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAddressTextCtrl, wxTextCtrl)
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

void MimeContent::SetData(char *data, size_t length)
{
   ASSERT( data != NULL );

   m_Data = data;
   m_Length = length;
   m_Type = MIMECONTENT_DATA;
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

// expand the address when <TAB> is pressed
void wxAddressTextCtrl::OnChar(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   m_composeView->SetLastAddressEntry(m_id);

   // we're only interested in TABs
   if ( event.KeyCode() == WXK_TAB && !event.ControlDown() && 
        !event.ShiftDown() && !event.AltDown() )
   {
      // try to expand the last component
      String text = GetValue();
      if ( text.IsEmpty() )
      {
         // don't do anything
         event.Skip();

         return;
      }

      // find the starting position of the last address in the address list
      size_t nLastAddr;
      for ( nLastAddr = text.length() - 1; nLastAddr > 0; nLastAddr-- )
      {
         char c = text[nLastAddr];
         if ( isspace(c) || (c == ',') || (c == ';') )
            break;
      }

      if ( nLastAddr > 0 )
      {
         // move beyond the ' ', ',' or ';' which stopped the scan
         nLastAddr++;
      }

      String expansion = AdbExpand(text.c_str() + nLastAddr, m_composeView);
      if ( !expansion.IsEmpty() )
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
               break;
         }
         
         // take what was there before...
         wxString newText(text, nPrevAddrEnd);  // first nPrevAddrEnd chars
         if ( !newText.IsEmpty() )
         {
            // there was something before, add separator
            newText += CANONIC_ADDRESS_SEPARATOR;
         }

         // ... and and the replacement string
         newText += expansion;

         SetValue(newText);
         SetInsertionPointEnd();
      }
      //else
      //  don't change the text
   }
   else
   {
      // let the text control process it normally
      event.Skip();
   }
}

// ----------------------------------------------------------------------------
// wxComposeView creation
// ----------------------------------------------------------------------------
void
wxComposeView::Create(const String &iname, wxWindow * WXUNUSED(parent),
                      ProfileBase *parentProfile,
                      String const &to, String const &cc, String const &bcc,
                      bool hide)
{
   CHECK_RET( !initialised, "wxComposeView created twice" );

   m_LayoutWindow = NULL;
   nextFileID = 0;

   if(!parentProfile)
      parentProfile = mApplication->GetProfile();
   m_Profile = ProfileBase::CreateProfile(iname,parentProfile);

   // build menu
   // ----------
   AddFileMenu();
   WXADD_MENU(m_MenuBar, COMPOSE, "&Compose");
   AddHelpMenu();
   SetMenuBar(m_MenuBar);

   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_COMPOSE);

   CreateStatusBar();

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

   // compose window
   CreateFTCanvas();
   c = new wxLayoutConstraints;
   c->left.SameAs(m_panel, wxLeft, LAYOUT_MARGIN);
   c->right.SameAs(m_panel, wxRight, LAYOUT_MARGIN);
   c->top.Below(box, LAYOUT_MARGIN);
   c->bottom.SameAs(m_panel, wxBottom, LAYOUT_MARGIN);
   m_LayoutWindow->SetConstraints(c);

   // layout the labels and text fields: label at the left of the field
   bool bDoShow[Field_Max];
   bDoShow[Field_To] =
      bDoShow[Field_Subject] = TRUE;  // To and subject always there
   bDoShow[Field_Cc] = READ_CONFIG(m_Profile, MP_SHOWCC) != 0;
   bDoShow[Field_Bcc] = READ_CONFIG(m_Profile, MP_SHOWBCC) != 0;

   // first determine the longest label caption
   static const char *aszLabels[Field_Max] =
   {
      "To:", "Subject:", "CC:", "BCC:",
   };

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
         // FIXME there might be other fields where we'd like to complete with
         // TABs - so we probably need some flag to distinguish them from the
         // others
         if ( n !=  Field_Subject ) {
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
            wxButton *btnExpand = new wxButton(m_panel, IDB_EXPAND, "Expand");
            wxLayoutConstraints *c2 = new wxLayoutConstraints;
            c2->top.SameAs(box, wxTop, 2*LAYOUT_MARGIN);
            c2->right.SameAs(box, wxRight, LAYOUT_MARGIN);
            c2->width.AsIs();
            c2->height.AsIs();
            btnExpand->SetConstraints(c2);

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

   // initialize the controls
   // -----------------------

   // set def values for the headers
   if(m_txtFields[Field_To]) {
      m_txtFields[Field_To]->SetValue(
         strutil_isempty(to) ? READ_CONFIG(m_Profile, MP_COMPOSE_TO) : to);
   }
   if(m_txtFields[Field_Cc]) {
      m_txtFields[Field_Cc]->SetValue(
         strutil_isempty(cc) ? READ_CONFIG(m_Profile, MP_COMPOSE_CC) : cc);
   }
   if(m_txtFields[Field_Bcc]) {
      m_txtFields[Field_Bcc]->SetValue(
         strutil_isempty(bcc) ? READ_CONFIG(m_Profile, MP_COMPOSE_BCC) : bcc);
   }

   // append signature
   if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE) )
   {
      String strSignFile = READ_CONFIG(m_Profile, MP_COMPOSE_SIGNATURE);
      if ( strSignFile.IsEmpty() ) {
         // no signature, propose to choose it now
         if ( MDialog_YesNoDialog(
               _("No signature file found. Would you like to choose one\n"
                 "right now (otherwise no signature will be used)?"),
                 this, MDIALOG_YESNOTITLE, true, "AskForSig") ) {
            wxFileDialog dialog(this, "", "", "", _("All files (*.*)|*.*"),
                                wxHIDE_READONLY | wxFILE_MUST_EXIST);
            if ( dialog.ShowModal() == wxID_OK ) {
               strSignFile = dialog.GetPath();
            }
         }
      }

      if ( !strSignFile.IsEmpty() ) {
         wxTextFile fileSig(strSignFile);
         if ( fileSig.Open() ) {
            // insert separator optionally
            if( READ_CONFIG(m_Profile, MP_COMPOSE_USE_SIGNATURE_SEPARATOR) ) {
               m_LayoutWindow->GetLayoutList().Insert("--");
               m_LayoutWindow->GetLayoutList().LineBreak();;
            }

            // read the whole file
            size_t nLineCount = fileSig.GetLineCount();
            for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
               m_LayoutWindow->GetLayoutList().Insert(fileSig[nLine]);
               m_LayoutWindow->GetLayoutList().LineBreak();;
            }

            // let's respect the netiquette
            static const size_t nMaxSigLines = 4;
            if ( nLineCount > nMaxSigLines ) {
               wxLogWarning(_("Your signature is too long: it should not be more"
                              " than %d lines"), nMaxSigLines);
            }

            m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
         }
      }
      else {
         // no signature file
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

void
wxComposeView::CreateFTCanvas(void)
{
   m_LayoutWindow = new wxLayoutWindow(m_panel);

   String
      fg = READ_CONFIG(m_Profile,MP_FTEXT_FGCOLOUR),
      bg = READ_CONFIG(m_Profile,MP_FTEXT_BGCOLOUR);

   m_LayoutWindow->Clear(
      READ_CONFIG(m_Profile,MP_FTEXT_FONT),
      READ_CONFIG(m_Profile,MP_FTEXT_SIZE),
      READ_CONFIG(m_Profile,MP_FTEXT_STYLE),
      READ_CONFIG(m_Profile,MP_FTEXT_WEIGHT),
      0,
      fg.c_str(),bg.c_str());
   m_LayoutWindow->GetLayoutList().SetEditable(true);
   //FIXMEm_LayoutWindow->SetWrapMargin(READ_CONFIG(profile, MP_COMPOSE_WRAPMARGIN));
}

wxComposeView::wxComposeView(const String &iname,
                             wxWindow *parent,
                             ProfileBase *parentProfile,
                             bool hide)
   : wxMFrame(iname,parent)
{
   initialised = false;
   m_fieldLast = Field_Max;
   Create(iname,parent,parentProfile, "","","",hide);
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

   String what = Str(m_txtFields[m_fieldLast]->GetValue());
   if ( what.IsEmpty() )
   {
     wxLogStatus(this, _("Nothing to expand - please enter something."));

     return;
   }

   // split the text into entries: they may be separated by commas, semicolons
   // or just whitespace (any amount of whitespace counts as one separator)
   wxArrayString addresses;
   String current;
   for ( const char *pc = what; ; pc++ )
   {
      switch ( *pc )
      {
         case ' ':
         case '\t':
            if ( current.IsEmpty() )
            {
               // repeated space, ignore
               continue;
            }
            // fall through, it's a separator

         case ',':
         case ';':
         case '\0':
            addresses.Add(current);
            if ( *pc == '\0' )
            {
               // will exit the loop
               break;
            }

            current.Empty();
            continue;

         default:
            // normal character
            current += *pc;
            continue;
      }

      break;
   }

   // expand all entries (TODO: some may be already expanded??)
   String expansion, total;
   size_t nEntries = addresses.Count();
   for ( size_t n = 0; n < nEntries; n++ )
   {
      expansion = AdbExpand(addresses[n], this);
      if ( expansion.IsEmpty() )
      {
         // expansion failed, leave as is
         total += addresses[n];
      }
      else
      {
         // replace with expanded address
         total += expansion;
      }

      total += CANONIC_ADDRESS_SEPARATOR;
   }

   m_txtFields[m_fieldLast]->SetValue(total);
}

void
wxComposeView::OnCloseWindow(wxCloseEvent& event)
{
   bool bDoClose = TRUE;
   if ( m_LayoutWindow && m_LayoutWindow->IsDirty() ) {
      bDoClose = MDialog_YesNoDialog
                 (
                  _("There are unsaved changes, close anyway?"),
                  this, // parent
                  MDIALOG_YESNOTITLE,
                  false // "yes" not default
                 );
   }

   if ( bDoClose ) {
      Destroy();
   }
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
            m_LayoutWindow->ResetDirty();
         Close();
      }
      break;

   case WXMENU_COMPOSE_PRINT:
      Print();
      break;

   case WXMENU_COMPOSE_CLEAR:
      m_LayoutWindow->Clear();
      break;

   default:
      wxMFrame::OnMenuCommand(id);
   }
}

// ----------------------------------------------------------------------------
// wxComposeView operations
// ----------------------------------------------------------------------------

void
wxComposeView::InsertData(char *data,
                          size_t length,
                          const char *mimetype)
{
   MimeContent *mc = new MimeContent();

   if(strutil_isempty(mimetype))
   {
      mimetype = "APPLICATION/OCTET-STREAM";
   }

   mc->SetMimeType(mimetype);
   mc->SetData(data, length);

   wxIcon icon = mApplication->GetIconManager()->GetIconFromMimeType(mimetype);

   wxLayoutObjectIcon *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(mc);
   m_LayoutWindow->GetLayoutList().Insert(obj);

   Refresh();
}

void
wxComposeView::InsertFile(const char *filename, const char *mimetype)
{
   MimeContent
      *mc = new MimeContent();

   if( strutil_isempty(filename) )
   {
      filename = MDialog_FileRequester(NULLstring, this, NULLstring,
                                       NULLstring, NULLstring,
                                       NULLstring, true, m_Profile);
      if( strutil_isempty(filename) )
      {
         // cancelled by user
         return;
      }
   }

   String strExt = wxString(filename).After('.');
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

   m_LayoutWindow->GetLayoutList().Insert(obj);

   wxLogStatus(this, _("Inserted file '%s' (as '%s')"),
               filename, strMimeType.c_str());

   Refresh();
}

bool
wxComposeView::Send(void)
{
   bool success = false;
   String
      tmp2, mimeType, mimeSubType;

   SendMessageCC sm
      (
         mApplication->GetProfile(),
         (const char *)m_txtFields[Field_Subject]->GetValue(),
         (const char *)m_txtFields[Field_To]->GetValue(),
         READ_CONFIG(m_Profile, MP_SHOWCC) ? m_txtFields[Field_Cc]->GetValue().c_str()
         : (const char *)NULL,
         READ_CONFIG(m_Profile, MP_SHOWBCC) ? m_txtFields[Field_Bcc]->GetValue().c_str()
         : (const char *)NULL
         );

   wxLayoutExportObject *export;
   wxLayoutList::iterator i = m_LayoutWindow->GetLayoutList().begin();
   wxLayoutObjectBase *lo = NULL;
   MimeContent *mc = NULL;

   while((export = wxLayoutExport(m_LayoutWindow->GetLayoutList(),
                                  i,WXLO_EXPORT_AS_TEXT)) != NULL)
   {
      if(export->type == WXLO_EXPORT_TEXT)
      {
         String* text = export->content.text;
         sm.AddPart
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

                        sm.AddPart
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
                        wxLogError(_("Can't read file '%s' included in "
                                     "this message!"), filename.c_str());
                     }

                     delete [] buffer;
                  }
                  else
                  {
                     wxLogError(_("Can't open file '%s' included in "
                                  "this message!"), filename.c_str());
                  }
               }
               break;

            case MimeContent::MIMECONTENT_DATA:
               {
                  MessageParameterList dlist;
                  MessageParameter *p = new MessageParameter;
                  p->name = "FILENAME";
                  p->value = wxFileNameFromPath(mc->GetFileName());
                  dlist.push_back(p);

                  sm.AddPart
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
         }
      }

      delete export;
   }

   success = sm.Send();  // true if sent

   if(READ_CONFIG(m_Profile,MP_USEOUTGOINGFOLDER))
   {
      String file;
      MailFolder *mf =
         MailFolder::OpenFolder(MailFolder::MF_PROFILE,
                                READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER));
      if(! mf) // no profile of such name
         mf = MailFolder::OpenFolder(MailFolder::MF_FILE,
                                     strutil_expandfoldername(READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER))); 
               
      if(mf)
      {
         file = READ_CONFIG(m_Profile,MP_FOLDER_PATH);
         if(strutil_isempty(file))
            file = READ_CONFIG(m_Profile,MP_OUTGOINGFOLDER);
         sm.WriteToFile(file,true/*append*/);
         mf->DecRef();
      }
   }
   return success;
}

// -----------------------------------------------------------------------------
// simple GUI accessors
// -----------------------------------------------------------------------------

/// sets To field
void
wxComposeView::SetTo(const String &to)
{
   m_txtFields[Field_To]->SetValue(WXCPTR to.c_str());
}

/// sets CC field
void
wxComposeView::SetCC(const String &cc)
{
   m_txtFields[Field_Cc]->SetValue(WXCPTR cc.c_str());
}

/// sets Subject field
void
wxComposeView::SetSubject(const String &subj)
{
   m_txtFields[Field_Subject]->SetValue(WXCPTR subj.c_str());
}

/// inserts a text
void
wxComposeView::InsertText(const String &txt)
{
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
   wxLayoutImportText(m_LayoutWindow->GetLayoutList(), txt);
   m_LayoutWindow->GetLayoutList().SetCursor(wxPoint(0,0));
}

/// print the message
void
wxComposeView::Print(void)
{
   m_LayoutWindow->Print();
}

// -----------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------

/// verify that the message is ready to be sent
bool
wxComposeView::IsReadyToSend() const
{
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
            ShowOptionsDialog((wxComposeView *)this, OptionPage_Network);
         }
         else
         {
            wxLogError(_("Can not send message - network is not configured."));

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
