/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMessageView.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef EXPERIMENTAL_karsten

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"

#  include "PathFinder.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"

#  include "MApplication.h"
#  include "gui/wxMApp.h"
#  include "gui/wxIconManager.h"
#endif //USE_PCH

#include "Mdefaults.h"
#include "MHelp.h"
#include "Message.h"
#include "FolderView.h"
#include "ASMailFolder.h"
#include "MFolder.h"
#include "MDialogs.h"
#include "MessageView.h"
#include "XFace.h"
#include "miscutil.h"
#include "sysutil.h"

#include "MessageTemplate.h"

#include "gui/wxIconManager.h"
#include "gui/wxMessageView.h"
#include "gui/wxFolderView.h"
#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxComposeView.h"

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/fontmap.h>
#include <wx/clipbrd.h>

#include <ctype.h>  // for isspace
#include <time.h>   // for time stamping autocollected addresses

#ifdef USE_NEW_MSGVIEW
#   error "wxMessageViewNew.cpp must be used instead"
#endif

#ifdef OS_UNIX
#   include <sys/stat.h>
#endif

// for testing only
#ifndef USE_PCH
//   extern "C"
//   {
//#     include <rfc822.h>
//   }
//#  include "MessageCC.h"
#endif //USE_PCH

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

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// make the first letter of the string capitalized and all the others of lower
// case, it looks nicer like this when presented to the user
static String NormalizeString(const String& s)
{
   String norm;
   if ( !s.empty() )
   {
      norm << String(s[0]).Upper() << String(s.c_str() + 1).Lower();
   }

   return norm;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Data about a process (external viewer typically) we launched: these objects
// are created by LaunchProcess() and deleted when the viewer process
// terminates. If it terminates with non 0 exit code, errormsg is given to the
// user. The tempfile is the name of a temp file containing the data passedto
// the viewer (or NULL if none) and will be removed after viewer terminates.
class ProcessInfo
{
public:
   ProcessInfo(wxProcess *process,

               int pid,
               const String& errormsg,
               const String& tempfilename)
      : m_errormsg(errormsg)
   {
      ASSERT_MSG( process && pid, "invalid process in ProcessInfo" );

      m_process = process;
      m_pid = pid;

      if ( !tempfilename.IsEmpty() )
         m_tempfile = new MTempFileName(tempfilename);
      else
         m_tempfile = NULL;
   }

   ~ProcessInfo()
   {
      if ( m_process )
         delete m_process;
      if ( m_tempfile )
         delete m_tempfile;
   }

   // get the pid of our process
   int GetPid() const { return m_pid; }

   // get the error message
   const String& GetErrorMsg() const { return m_errormsg; }

   // don't delete wxProcess object (must be called before destroying this
   // object if the external process is still running)
   void Detach() { m_process->Detach(); m_process = NULL; }

   // return the pointer to temp file object (may be NULL)
   MTempFileName *GetTempFile() const { return m_tempfile; }

private:
   String         m_errormsg; // error message to give if launch failed
   wxProcess     *m_process;  // wxWindows process info
   int            m_pid;      // pid of the process
   MTempFileName *m_tempfile; // the temp file (or NULL if none)
};

// data associated with the clickable objects
class ClickableInfo : public wxLayoutObject::UserData
{
public:
   enum Type
   {
      CI_ICON,
      CI_URL
   };

   ClickableInfo(const String& url)
      : m_url(url)
      { m_type = CI_URL; SetLabel(url); }
   ClickableInfo(int id, const String &label)
      {
         m_id = id;
         m_type = CI_ICON;
         SetLabel(label);
      }

   // accessors
   Type          GetType() const { return m_type; }
   const String& GetUrl()  const { return m_url;  }
   int           GetPart() const { return m_id;   }

private:
   ~ClickableInfo() {}
   Type   m_type;

   int    m_id;
   String m_url;

   GCC_DTOR_WARN_OFF
};

// the popup menu invoked by clicking on an attachment in the mail message
class MimePopup : public wxMenu
{
public:
   MimePopup(wxMessageView *parent, int partno)
   {
      // init member vars
      m_PartNo = partno;
      m_MessageView = parent;

      // create the menu items
      Append(WXMENU_MIME_INFO, _("&Info"));
      AppendSeparator();
      Append(WXMENU_MIME_OPEN, _("&Open"));
      Append(WXMENU_MIME_OPEN_WITH, _("Open &with..."));
      Append(WXMENU_MIME_SAVE, _("&Save..."));
      Append(WXMENU_MIME_VIEW_AS_TEXT, _("&View as text"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   wxMessageView *m_MessageView;
   int m_PartNo;

   DECLARE_EVENT_TABLE()
};

// the popup menu used with URLs
class UrlPopup : public wxMenu
{
public:
   UrlPopup(wxMessageView *parent, const String& url)
      : m_url(url)
   {
      m_MessageView = parent;

      SetTitle(NormalizeString(url.BeforeFirst(':')) + _(" url"));
      Append(WXMENU_URL_OPEN, _("&Open"));
      Append(WXMENU_URL_OPEN_NEW, _("Open in &new window"));
      Append(WXMENU_URL_COPY, _("&Copy to clipboard"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   wxMessageView *m_MessageView;
   String m_url;

   DECLARE_EVENT_TABLE()
};

// the message parameters for the MIME type manager
class MailMessageParameters : public wxFileType::MessageParameters
{
public:
   MailMessageParameters(const wxString& filename,
         const wxString& mimetype,
         Message *mailMessage,
         int part)
      : wxFileType::MessageParameters(filename, mimetype)
      {
         m_mailMessage = mailMessage;
         m_part = part;
      }

   virtual wxString GetParamValue(const wxString& name) const;

private:
   Message *m_mailMessage;
   int m_part;
};

// the var expander for message view frame title
//
// TODO: the code should be reused with VarExpander in wxComposeView.cpp!
class MsgVarExpander : public MessageTemplateVarExpander
{
public:
   MsgVarExpander(Message *msg) { m_msg = msg; SafeIncRef(m_msg); }
   virtual ~MsgVarExpander() { SafeDecRef(m_msg); }

   virtual bool Expand(const String& category,
                       const String& Name,
                       const wxArrayString& arguments,
                       String *value) const
   {
      if ( !m_msg )
         return false;

      // we only understand fields in the unnamed/default category
      if ( !category.empty() )
         return false;

      String name = Name.Lower();
      if ( name == "from" )
         *value = m_msg->From();
      else if ( name == "subject" )
         *value = m_msg->Subject();
      else if ( name == "date" )
         *value = m_msg->Date();
      else
         return false;

      return true;
   }

private:
    Message *m_msg;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MimePopup, wxMenu)
   EVT_MENU(-1, MimePopup::OnCommandEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(UrlPopup, wxMenu)
   EVT_MENU(-1, UrlPopup::OnCommandEvent)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================


static String
GetParameter(Message *msg, int partno, const String &param)
{
   String value;
   if ( !msg->ExpandParameter(msg->GetParameters(partno), param, &value) )
      (void)msg->ExpandParameter(msg->GetDisposition(partno), param, &value);

   return value;
}


static
String GetFileNameForMIME(Message *message, int partNo)
{
   String fileName = GetParameter(message, partNo, "FILENAME");
   if ( fileName.empty() )
      fileName = GetParameter(message, partNo,"NAME");

   return fileName;
}


void
MimePopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_MIME_INFO:
         m_MessageView->MimeInfo(m_PartNo);
         break;

      case WXMENU_MIME_OPEN:
         m_MessageView->MimeHandle(m_PartNo);
         break;

      case WXMENU_MIME_OPEN_WITH:
         m_MessageView->MimeOpenWith(m_PartNo);
         break;

      case WXMENU_MIME_VIEW_AS_TEXT:
         m_MessageView->MimeViewText(m_PartNo);
         break;

      case WXMENU_MIME_SAVE:
         m_MessageView->MimeSave(m_PartNo);
         break;
   }
}

wxString
MailMessageParameters::GetParamValue(const wxString& name) const
{
   const MessageParameterList &plist = m_mailMessage->GetParameters(m_part);
   MessageParameterList::iterator i;
   for ( i = plist.begin(); i != plist.end(); i++) {
      if ( name.CmpNoCase((*i)->name) == 0 ) {
         // found
         return (*i)->value;
      }
   }

   const MessageParameterList &dlist = m_mailMessage->GetDisposition(m_part);
   for ( i = dlist.begin(); i != dlist.end(); i++) {
      if ( name.CmpNoCase((*i)->name) == 0 ) {
         // found
         return (*i)->value;
      }
   }

   // if all else failed, call the base class

   // typedef is needed for VC++ 5.0 - otherwise you get a compile error!
   typedef wxFileType::MessageParameters BaseMessageParameters;
   return BaseMessageParameters::GetParamValue(name);
}

// ----------------------------------------------------------------------------
// UrlPopup
// ----------------------------------------------------------------------------

void
UrlPopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_URL_OPEN:
         m_MessageView->OpenURL(m_url, FALSE);
         break;

      case WXMENU_URL_OPEN_NEW:
         m_MessageView->OpenURL(m_url, TRUE);
         break;

      case WXMENU_URL_COPY:
         {
            wxClipboardLocker lockClip;
            if ( !lockClip )
            {
               wxLogError(_("Failed to lock clipboard, URL not copied."));
            }
            else
            {
               wxTheClipboard->SetData(new wxTextDataObject(m_url));
            }
         }
         break;

      default:
         FAIL_MSG( "unexpected URL popup menu command" );
         break;
   }
}
// ----------------------------------------------------------------------------
// wxMessageView::AllProfileValues
// ----------------------------------------------------------------------------

bool wxMessageView::AllProfileValues::operator==(const AllProfileValues& other)
{
   #define CMP(x) (x == other.x)

   return CMP(BgCol) && CMP(FgCol) && CMP(UrlCol) &&
          CMP(QuotedCol[0]) && CMP(QuotedCol[1]) && CMP(QuotedCol[2]) &&
          CMP(quotedColourize) && CMP(quotedCycleColours) &&
          CMP(quotedMaxWhitespace) && CMP(quotedMaxAlpha) &&
          CMP(HeaderNameCol) && CMP(HeaderValueCol) &&
          CMP(font) && CMP(size) &&
          CMP(showHeaders) && CMP(inlineRFC822) && CMP(inlinePlainText) &&
          CMP(highlightURLs) && CMP(inlineGFX) &&
          CMP(browser) && CMP(browserInNewWindow) &&
          CMP(autocollect) && CMP(autocollectNamed) &&
          CMP(autocollectBookName) &&
#ifdef OS_UNIX
          CMP(browserIsNS) && CMP(afmpath) &&
#endif // Unix
          CMP(showFaces);

   #undef CMP
}

// ----------------------------------------------------------------------------
// wxMessageView
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMessageView, wxLayoutWindow)
   // process termination notification
   EVT_END_PROCESS(-1, wxMessageView::OnProcessTermination)

   // mouse click processing
   EVT_MENU(WXLOWIN_MENU_RCLICK, wxMessageView::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_LCLICK, wxMessageView::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_DBLCLICK, wxMessageView::OnMouseEvent)

   // menu & toolbars
   EVT_MENU(-1, wxMessageView::OnMenuCommand)
   EVT_TOOL(-1, wxMessageView::OnMenuCommand)
   EVT_CHAR(wxMessageView::OnChar)
END_EVENT_TABLE()

void
wxMessageView::SetLanguage(int id)
{
   wxFontEncoding encoding = GetEncodingFromMenuCommand(id);
   SetEncoding(encoding);
}

void
wxMessageView::OnChar(wxKeyEvent& event)
{
   // FIXME: this should be more intelligent, i.e. use the
   // wxlayoutwindow key bindings:
   if(m_WrapMargin > 0 &&
      event.KeyCode() == 'q' || event.KeyCode() == 'Q')
   {
      // temporarily allow editing to enable manual word wrap:
      SetEditable(TRUE);
      event.Skip();
      SetEditable(FALSE);
      SetCursorVisibility(1);
   }
   else
      event.Skip();
}

void
wxMessageView::Create(wxFolderView *fv, wxWindow *parent)
{
   m_mailMessage = NULL;
   // mimeDisplayPart = 0; -- unused
   xface = NULL;
   xfaceXpm = NULL;
   m_Parent = parent;
   m_FolderView = fv;
   m_MimePopup = NULL;
   m_uid = UID_ILLEGAL;
   m_encodingUser = wxFONTENCODING_SYSTEM;
   SetMouseTracking();
   SetParentProfile(fv ? fv->GetProfile() : NULL);

   wxFrame *p = GetFrame(this);
   if ( p )
   {
      wxStatusBar *statusBar = p->GetStatusBar();
      if ( statusBar )
      {
         // we don't edit the message, so the cursor coordinates are useless
         SetStatusBar(statusBar, 0, -1);
      }
   }
   RegisterForEvents();
}


wxMessageView::wxMessageView(wxFolderView *fv,
                             wxWindow *parent,
                             bool show)
             : wxLayoutWindow(parent)
{
   m_folder = NULL;
   m_Profile = NULL;
   Create(fv,parent);
   Show(show);
}

wxMessageView::wxMessageView(ASMailFolder *folder,
                             long num,
                             wxFolderView *fv,
                             wxWindow *parent,
                             bool show)
             : wxLayoutWindow(parent)
{
   m_folder = folder;
   if(m_folder) m_folder->IncRef();
   m_Profile = NULL;
   Create(fv,parent);
   if(folder)
      ShowMessage(folder,num);
   Show(show);
}

wxMessageView::~wxMessageView()
{
   if ( m_eventReg )
      MEventManager::Deregister(m_eventReg);
   if(m_regCookieASFolderResult)
      MEventManager::Deregister(m_regCookieASFolderResult);

   size_t procCount = m_processes.GetCount();
   for ( size_t n = 0; n < procCount; n++ )
   {
      ProcessInfo *info = m_processes[n];
      info->Detach();

      MTempFileName *tempfile = info->GetTempFile();
      if ( tempfile )
      {
         String tempFileName = tempfile->GetName();
         wxLogWarning(_("Temporary file '%s' left because it is still in "
                        "use by an external process"), tempFileName.c_str());
      }

      delete info;
   }

   if(m_mailMessage) m_mailMessage->DecRef();
   if(xface) delete xface;
   if(xfaceXpm) wxIconManager::FreeImage(xfaceXpm);
   if(m_Profile) m_Profile->DecRef();
   if(m_folder) m_folder->DecRef();

   wxDELETE(m_MimePopup);
}

void
wxMessageView::RegisterForEvents()
{
   // register with the event manager
   m_eventReg = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_eventReg, "can't register for options change event");
   m_regCookieASFolderResult = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookieASFolderResult, "can't reg folder view with event manager");
}

ASMailFolder *
wxMessageView::GetFolder(void)
{
   return m_FolderView ? m_FolderView->GetFolder() : m_folder;
}

/// Tell it a new parent profile - in case folder changed.
void
wxMessageView::SetParentProfile(Profile *profile)
{
   SafeDecRef(m_Profile);

   if(profile)
   {
      m_Profile = profile;
      m_Profile->IncRef();
   }
   else
   {
      m_Profile = Profile::CreateEmptyProfile();
   }

   UpdateProfileValues();

   Clear();
}

void
wxMessageView::UpdateProfileValues()
{
   ReadAllSettings(&m_ProfileValues);
}

void
wxMessageView::ReadAllSettings(AllProfileValues *settings)
{
   // We also use this to set all values to be read to speed things up:
   #define GET_COLOUR_FROM_PROFILE(which, name) \
      GetColourByName(&settings->which, \
                      READ_CONFIG(m_Profile, MP_MVIEW_##name), \
                      MP_MVIEW_##name##_D)

   GET_COLOUR_FROM_PROFILE(FgCol, FGCOLOUR);
   GET_COLOUR_FROM_PROFILE(BgCol, BGCOLOUR);
   GET_COLOUR_FROM_PROFILE(UrlCol, URLCOLOUR);
   GET_COLOUR_FROM_PROFILE(QuotedCol[0], QUOTED_COLOUR1);
   GET_COLOUR_FROM_PROFILE(QuotedCol[1], QUOTED_COLOUR2);
   GET_COLOUR_FROM_PROFILE(QuotedCol[2], QUOTED_COLOUR3);
   GET_COLOUR_FROM_PROFILE(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderValueCol, HEADER_VALUES_COLOUR);

   #undef GET_COLOUR_FROM_PROFILE

   settings->quotedColourize = READ_CONFIG(m_Profile, MP_MVIEW_QUOTED_COLOURIZE) != 0;
   settings->quotedCycleColours = READ_CONFIG(m_Profile, MP_MVIEW_QUOTED_CYCLE_COLOURS) != 0;
   settings->quotedMaxWhitespace = READ_CONFIG(m_Profile, MP_MVIEW_QUOTED_MAXWHITESPACE) != 0;
   settings->quotedMaxAlpha = READ_CONFIG(m_Profile,MP_MVIEW_QUOTED_MAXALPHA);

   settings->font = READ_CONFIG(m_Profile,MP_MVIEW_FONT);
   ASSERT(settings->font >= 0 && settings->font <= NUM_FONTS);

   settings->font = wxFonts[settings->font];
   settings->size = READ_CONFIG(m_Profile,MP_MVIEW_FONT_SIZE);
   settings->showHeaders = READ_CONFIG(m_Profile,MP_SHOWHEADERS) != 0;
   settings->inlinePlainText = READ_CONFIG(m_Profile,MP_PLAIN_IS_TEXT) != 0;
   settings->inlineRFC822 = READ_CONFIG(m_Profile,MP_RFC822_IS_TEXT) != 0;
   settings->highlightURLs = READ_CONFIG(m_Profile,MP_HIGHLIGHT_URLS) != 0;

   // we set inlineGFX to -1 if we don't inline graphics at all and to the
   // max size limit of graphics to show inline otherwise (0 if no limit)
   settings->inlineGFX = READ_CONFIG(m_Profile, MP_INLINE_GFX)
                         ? READ_CONFIG(m_Profile, MP_INLINE_GFX_SIZE)
                         : -1;

   settings->browser = READ_CONFIG(m_Profile, MP_BROWSER);
   settings->browserInNewWindow = READ_CONFIG(m_Profile, MP_BROWSER_INNW) != 0;
   settings->autocollect =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT);
   settings->autocollectNamed =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT_NAMED);
   settings->autocollectBookName = READ_CONFIG(m_Profile, MP_AUTOCOLLECT_ADB);
   settings->showFaces = READ_CONFIG(m_Profile, MP_SHOW_XFACES) != 0;

   // these settings are used under Unix only
#ifdef OS_UNIX
   settings->browserIsNS = READ_CONFIG(m_Profile, MP_BROWSER_ISNS) != 0;
   settings->afmpath = READ_APPCONFIG(MP_AFMPATH);
#endif // Unix

#ifndef OS_WIN
   SetFocusFollowMode(READ_CONFIG(m_Profile,MP_FOCUS_FOLLOWSMOUSE) != 0);
#endif
   SetWrapMargin(READ_CONFIG(m_Profile, MP_VIEW_WRAPMARGIN));

   // update the parents menu as the show headers option might have changed
   UpdateShowHeadersInMenu();
}

void
wxMessageView::ClearWithoutReset(void)
{
   SetCursorVisibility(-1); // on demand
   wxLayoutWindow::Clear(m_ProfileValues.font, m_ProfileValues.size,
                         (int)wxNORMAL, (int)wxNORMAL, 0,
                         &m_ProfileValues.FgCol,
                         &m_ProfileValues.BgCol,
                         true /* no update */);
   SetBackgroundColour( m_ProfileValues.BgCol );
   GetLayoutList()->SetAutoFormatting(FALSE); // speeds up insertion of text
}

void
wxMessageView::Clear()
{
   ClearWithoutReset();
   RequestUpdate();

   m_uid = UID_ILLEGAL;
   if ( m_mailMessage )
   {
      m_mailMessage->DecRef();
      m_mailMessage = NULL;
   }
}

void
wxMessageView::SetEncoding(wxFontEncoding encoding)
{
   m_encodingUser = encoding;

   Update();

   if ( READ_CONFIG(m_Profile, MP_MSGVIEW_AUTO_ENCODING) )
   {
      // don't keep it for the other messages, just for this one
      m_encodingUser = wxFONTENCODING_SYSTEM;
   }
}

void
wxMessageView::Update(void)
{
   char const * cptr;
   String tmp,from,url, mimeType, fileName, disposition;
   bool   lastObjectWasIcon = false; // a flag
   ClickableInfo *ci;

   wxLayoutList *llist = GetLayoutList();
   wxLayoutObject *obj = NULL;

   ClearWithoutReset();

   if(! m_mailMessage)  // no message to display
      return;

   m_uid = m_mailMessage->GetUId();

   // if wanted, display all header lines
   if(m_ProfileValues.showHeaders)
   {
      wxLayoutImportText(llist, m_mailMessage->GetHeader());
      llist->LineBreak();
   }

#ifdef HAVE_XFACES
   if(m_ProfileValues.showFaces)
   {
      m_mailMessage->GetHeaderLine("X-Face", tmp);
      if(tmp.length() > 2)   //\r\n
      {
         xface = new XFace();
         xface->CreateFromXFace(tmp.c_str());
         if(xface->CreateXpm(&xfaceXpm))
         {
            llist->Insert(new wxLayoutObjectIcon(new wxBitmap(xfaceXpm)));
            llist->LineBreak();
         }
      }
   }
#endif // HAVE_XFACES

   // show the configurable headers
   wxFontEncoding encInHeaders = wxFONTENCODING_SYSTEM;
   String headersString(READ_CONFIG(m_Profile, MP_MSGVIEW_HEADERS));
   String headerName, headerValue;
   for ( const char *p = headersString.c_str(); *p != '\0'; p++ )
   {
      if ( *p == ':' )
      {
         // first get the value of this header
         wxFontEncoding encHeader;
         m_mailMessage->GetHeaderLine(headerName, headerValue, &encHeader);

         // we will usually detect the encoding in the headers properly (if a
         // mailer does use RFC 2047, it will use it properly), but if we
         // found none it might be because the mailer is broken and sends 8
         // bit strings (almost all Web mailers do!) in headers in which case
         // we shall take the user-specified encoding
         if ( m_encodingUser != wxFONTENCODING_SYSTEM &&
              encHeader == wxFONTENCODING_SYSTEM )
         {
            encHeader = m_encodingUser;
         }

         // check for several special cases
         if ( headerName == "From" )
         {
            String name;
            String from = m_mailMessage->Address(name, MAT_FROM);
            headerValue = GetFullEmailAddress(name, from);
         }
         else if ( headerName == "Date" )
         {
            // don't read the header line directly because Date() function
            // might return date in some format different from RFC822 one
            headerValue = m_mailMessage->Date();
         }

         // don't show the header if there is no value
         if ( !!headerValue )
         {
            // always terminate the header names with ": " - configurability
            // cannot be endless neither
            headerName += ": ";

            // insert the header name
            llist->SetFontWeight(wxBOLD);
            llist->SetFontColour(&m_ProfileValues.HeaderNameCol);
            llist->Insert(headerName);

            // insert the header value
            llist->SetFontWeight(wxNORMAL);
            llist->SetFontColour(&m_ProfileValues.HeaderValueCol);

            wxLayoutImportText(llist, headerValue, encHeader);

            llist->LineBreak();

            if ( encHeader != wxFONTENCODING_SYSTEM )
            {
               // remember that we had such encoding in the headers
               if ( encInHeaders == wxFONTENCODING_SYSTEM )
                  encInHeaders = encHeader;

               // restore the default encoding
               GetLayoutList()->SetFontEncoding(wxFONTENCODING_DEFAULT);
            }
         }

         headerName.Empty();
      }
      else
      {
         headerName += *p;
      }
   }

   // restore the normal colour
   llist->SetFontColour(&m_ProfileValues.FgCol);

   // get the encoding rom the message headers
   wxFontEncoding encBody;
   if ( m_encodingUser != wxFONTENCODING_SYSTEM )
   {
      // user-specified encoding overrides everything
      encBody = m_encodingUser;
   }
   else // auto-detect
   {
      // FIXME how to get the params of the top level message?
#if 0
      encBody = m_mailMessage->GetTextPartEncoding(-1);
      if ( encBody == wxFONTENCODING_SYSTEM )
#endif
      {
         // some broken mailers don't create correct "Content-Type" header,
         // but they may yet give us the necessary info in the other headers
         encBody = encInHeaders;
      }
   }

   // this var stores the MIME spec of the MESSAGE/RFC822 we're currently in, it
   // is empty if we're outside any embedded message
   String specMsg;

   // iterate over all parts
   int n = m_mailMessage->CountParts();
   for ( int i = 0; i < n; i++ )
   {
      String spec = m_mailMessage->GetPartSpec(i);

      // FIXME: there is a bug with this code as it only remembers the last
      //        embedded message and so will break if we have 2 embedded
      //        RFC822s - it will forget about the enclosing one after leaving
      //        the inner message
      //
      //        it's true that it happens rarely enough, but we should still
      //        maintain a stack of msg specs here instead of having only one
      //        variable...

      if ( !specMsg.empty() )
      {
         // check if this is a part of MULTIPART or MESSAGE message,
         // distinguish between the 2 as we do show MULTIPART subparts (even
         // though we should only do it for MIXED and DIGEST, probably, and
         // only show one part for ALTERNATIVE - TODO), but we don't show
         // MESSAGE subparts unless we're configured to inline them
         if ( spec.StartsWith(specMsg) )
         {
            // this is a part of embedded message
            if ( !m_ProfileValues.inlineRFC822 )
            {
               // don't show it
               continue;
            }
            //else: still show it as we're configured to do so
         }
         else
         {
            // we've finished with the embedded message
            specMsg.clear();
         }
      }
      //else: not part of an embedded message

      int t = m_mailMessage->GetPartType(i);
      if ( t == Message::MSG_TYPEMESSAGE )
      {
         // remember the part spec of the last embedded message found, used
         // above
         specMsg = spec;

         if ( m_ProfileValues.inlineRFC822 )
         {
            // we don't show the message itself, just its subparts
            continue;
         }
         //else: show the embedded message as an icon
      }

      if ( m_mailMessage->GetPartSize(i) == 0 )
      {
         // ignore empty parts but warn user as it might indicate a problem
         wxLogStatus(GetFrame(this), "Skipping the empty MIME part #%d.", i);

         continue;
      }

      mimeType = m_mailMessage->GetPartMimeType(i);
      strutil_tolower(mimeType);
      fileName = GetFileNameForMIME(m_mailMessage,i);
      (void) m_mailMessage->GetDisposition(i,&disposition);
      strutil_tolower(disposition);
#ifdef DEBUG
      obj = NULL;
#endif

      if ( t == Message::MSG_TYPEAPPLICATION ) // let's guess a little
      {
         wxString ext = fileName.AfterLast('.');
         wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
         wxFileType *ft = mimeManager.GetFileTypeFromExtension(ext);
         if(ft)
         {
            wxString mt;
            ft->GetMimeType(&mt);
            delete ft;
            if(wxMimeTypesManager::IsOfType(mt,"image/*"))
               t = Message::MSG_TYPEIMAGE;
            else if(wxMimeTypesManager::IsOfType(mt,"audio/*"))
               t = Message::MSG_TYPEAUDIO;
            else if(wxMimeTypesManager::IsOfType(mt,"video/*"))
               t = Message::MSG_TYPEVIDEO;
         }
      }

      /* Insert text:
         - if it is text/plain and not explicitly marked as "attachment"
           or has a filename and we don't override this
         - if it is rfc822 and it is configured to be displayed
         - HTML is for now displayed as normal text with the same rules
      */

      // FIXME: HTML is broken, so it should be at least an option, disabling
      //        it entirely for now (VZ)
      bool isHTML = FALSE; // mimeType == "text/html";
      if ( (disposition != "attachment") &&
           (
               ((mimeType == "text/plain" || isHTML) &&
                (fileName.empty() || m_ProfileValues.inlinePlainText))
               ||
               (t == Message::MSG_TYPEMESSAGE && m_ProfileValues.inlineRFC822)
           ) )
      {
         // get the encoding of the text
         wxFontEncoding encPart;
         if ( m_encodingUser != wxFONTENCODING_SYSTEM )
         {
            // user-specified encoding overrides everything
            encPart = m_encodingUser;
         }
         else if ( READ_CONFIG(m_Profile, MP_MSGVIEW_AUTO_ENCODING) )
         {
            encPart = m_mailMessage->GetTextPartEncoding(i);
            if ( encPart == wxFONTENCODING_SYSTEM )
            {
               // use the encoding of the last part which had it
               encPart = encBody;
            }
            else if ( encBody == wxFONTENCODING_SYSTEM )
            {
               // remember the encoding for the next parts
               encBody = encPart;
            }
         }
         else
         {
            // autodetecting encoding is disabled, don't use any
            encPart = wxFONTENCODING_SYSTEM;
         }

         unsigned long len;
         cptr = m_mailMessage->GetPartContent(i, &len);
         if(cptr == NULL)
            continue; // error ?
         llist->LineBreak();

         if( m_ProfileValues.highlightURLs || m_ProfileValues.quotedColourize )
         {
            tmp = cptr;
            String url;
            String before;

            int level = 0;
            size_t line_pos, line_lng, line_from;

// FIXME - should be put in some function
#define SET_QUOTING_COLOUR(qlevel)                      \
            switch (qlevel)                             \
            {                                           \
               case 0 :                                 \
                  llist->SetFontColour(& m_ProfileValues.FgCol);\
                  break;                                \
               default :                                \
                  llist->SetFontColour(& m_ProfileValues.QuotedCol[qlevel-1]);\
                  break;                                \
            }

#define SET_QUOTING_LEVEL(qlevel, text)                 \
            {                                           \
            qlevel = strutil_countquotinglevels(text,   \
                     m_ProfileValues.quotedMaxWhitespace,\
                     m_ProfileValues.quotedMaxAlpha);   \
            if (qlevel > 3)                             \
            {                                           \
               if (m_ProfileValues.quotedCycleColours)  \
                  qlevel = (qlevel-1) % 3 + 1;          \
               else                                     \
                  qlevel = 3;                           \
            }                                           \
            }

            SET_QUOTING_LEVEL(level, tmp)
            SET_QUOTING_COLOUR(level)
            do
            {
               before = strutil_findurl(tmp, url);

               if (m_ProfileValues.quotedColourize)
               {
                  line_from = 0;
                  line_lng = before.Length();
                  for (line_pos = 0; line_pos < line_lng; line_pos++)
                  {
                     if (before[line_pos] == '\n')
                     {
                        SET_QUOTING_LEVEL(level, before.c_str() + line_pos + 1)
                        if(isHTML)
                           wxLayoutImportHTML(llist,
                           before.Mid(line_from, line_pos - line_from + 1),
                           encPart);
                        else
                           wxLayoutImportText(llist,
                           before.Mid(line_from, line_pos - line_from + 1),
                           encPart);
                        SET_QUOTING_COLOUR(level)
                        line_from = line_pos + 1;
                     }
                  }
                  if (line_from < line_lng-1)
                  {
                     if(isHTML)
                        wxLayoutImportHTML(llist,
                                   before.Mid(line_from, line_lng - line_from),
                                   encPart);
                     else
                        wxLayoutImportText(llist,
                                   before.Mid(line_from, line_lng - line_from),
                                   encPart);
                  }
               }
               else // no quoted text colourizing
               {
                  if(isHTML)
                    wxLayoutImportHTML(llist, before, encPart);
                  else
                    wxLayoutImportText(llist, before, encPart);
               }
               if(!strutil_isempty(url) && m_ProfileValues.highlightURLs)
               {
                  ci = new ClickableInfo(url);
                  obj = new wxLayoutObjectText(url);
                  obj->SetUserData(ci);
                  ci->DecRef();
                  llist->SetFontColour(& m_ProfileValues.UrlCol);
                  llist->Insert(obj);
                  SET_QUOTING_COLOUR(level)
               }
            }
            while( !strutil_isempty(tmp) );
            lastObjectWasIcon = false;
#undef SET_QUOTING_COLOR
#undef SET_QUOTING_LEVEL
         }
      }
      else
         /* This block captures all non-inlined message parts. They get
            represented by an icon.
            In case of image content, we check whether it might be a
            Fax message. */
      {
         wxString mimeFileName = GetFileNameForMIME(m_mailMessage, i);

         bool showInline = t == Message::MSG_TYPEIMAGE;
         if ( showInline )
         {
            switch ( m_ProfileValues.inlineGFX )
            {
               default:
                  // check that the size of the image is less than configured
                  // maximum
                  if ( m_mailMessage->GetPartSize(i) >
                        1024*(size_t)m_ProfileValues.inlineGFX )
                  {
                     wxString msg;
                     msg.Printf
                         (
                           _("An image embedded in this message is bigger "
                             "than the currently configured limit of %luKb.\n"
                             "\n"
                             "Would you still like to see it?\n"
                             "\n"
                             "You can change this setting in the \"Message "
                             "View\" page of the preferences dialog to 0 if "
                             "you want to always show the images inline."),
                           (unsigned long)m_ProfileValues.inlineGFX
                         );

                     if ( MDialog_YesNoDialog
                          (
                           msg,
                           GetFrame(this),
                           MDIALOG_YESNOTITLE,
                           false, // [No] default
                           GetPersMsgBoxName(M_MSGBOX_GFX_NOT_INLINED)
                          ) )
                     {
                        // will inline
                        break;
                     }

                  }
                  else
                  {
                     // will inline
                     break;
                  }

                  // fall through

               case -1:
                  // never inline
                  showInline = false;
                  break;

               case 0:
                  // always inline
                  break;
            }
         }

         if ( showInline )
         {
            wxString filename = wxGetTempFileName("Mtemp");
            if(mimeFileName.Length() == 0)
               mimeFileName = GetParameter(m_mailMessage,i,"NAME");

            MimeSave(i,filename);
            bool ok;
            wxImage img =  wxIconManager::LoadImage(filename, &ok, true);
            wxRemoveFile(filename);
            if(ok)
               obj = new wxLayoutObjectIcon(img.ConvertToBitmap());
            else
               obj = new wxLayoutObjectIcon(
                  mApplication->GetIconManager()->
                  GetIconFromMimeType(m_mailMessage->GetPartMimeType(i),
                                      mimeFileName.AfterLast('.'))
                  );
         }
         else // show attachment as an icon
         {
            obj = new wxLayoutObjectIcon(
               mApplication->GetIconManager()->
               GetIconFromMimeType(m_mailMessage->GetPartMimeType(i),
                                   mimeFileName.AfterLast('.'))
               );
         }

         ASSERT(obj);
         {
            String label;
            label = mimeFileName;
            if(label.Length()) label << " : ";
            label << m_mailMessage->GetPartMimeType(i) << ", "
                  << strutil_ultoa(m_mailMessage->GetPartSize(i)) << _(" bytes");
            ci = new ClickableInfo(i, label);
            obj->SetUserData(ci); // gets freed by list
            ci->DecRef();
            llist->LineBreak(); //add a line break before each attachment

            // multiple images look better alligned vertically rather than
            // horizontally
            llist->Insert(obj);

            // add extra whitespace so lines with multiple icons can
            // be broken:
            llist->Insert(" ");
         }

         lastObjectWasIcon = true;
      }
   }

   // update the menu of the frame containing us to show the encoding used
   CheckLanguageInMenu(this, encBody == wxFONTENCODING_SYSTEM
                              ? wxFONTENCODING_DEFAULT
                              : encBody);

   llist->LineBreak();
   llist->MoveCursorTo(wxPoint(0,0));
   // we have modified the list directly, so we need to mark the
   // wxlwindow as dirty:
   SetDirty();
   // re-enable auto-formatting, seems safer for selection
   // highlighting, not sure if needed, though
   llist->SetAutoFormatting(TRUE);

   SetWrapMargin(READ_CONFIG(m_Profile, MP_VIEW_WRAPMARGIN));
   if(m_WrapMargin > 0 && READ_CONFIG(m_Profile, MP_VIEW_AUTOMATIC_WORDWRAP) != 0)
      llist->WrapAll(m_WrapMargin);
   // yes, we allow the user to edit the buffer, in case he wants to
   // modify it for pasting or wrap lines manually:
   SetEditable(FALSE);
   SetCursorVisibility(-1);
   GetLayoutList()->ForceTotalLayout();
   // for safety, this is required to set scrollbars
   RequestUpdate();
   ScrollToCursor();
}

// ----------------------------------------------------------------------------
// MIME attachments menu commands
// ----------------------------------------------------------------------------

// show information about an attachment
void
wxMessageView::MimeInfo(int mimeDisplayPart)
{
   String message;
   message << _("MIME type: ")
           << m_mailMessage->GetPartMimeType(mimeDisplayPart)
           << '\n';

   String desc = m_mailMessage->GetPartDesc(mimeDisplayPart);
   if ( !desc.empty() )
      message << '\n' << _("Description: ") << desc << '\n';

   message << _("Size: ")
           << strutil_ltoa(m_mailMessage->GetPartSize(mimeDisplayPart, true));

   // as we passed true to GetPartSize() above, it will return size in lines
   // for the text messages (and in bytes for everything else)
   Message::ContentType type = m_mailMessage->GetPartType(mimeDisplayPart);
   if(type == Message::MSG_TYPEMESSAGE || type == Message::MSG_TYPETEXT)
      message << _(" lines");
   else
      message << _(" bytes");
   message << '\n';

   // param name and value (used in 2 loops below)
   wxString name, value;

   // debug output with all parameters
   const MessageParameterList &plist = m_mailMessage->GetParameters(mimeDisplayPart);
   MessageParameterList::iterator plist_it;
   if ( !plist.empty() )
   {
      message += _("\nParameters:\n");
      for ( plist_it = plist.begin(); plist_it != plist.end(); plist_it++ )
      {
         name = (*plist_it)->name;
         message << NormalizeString(name) << ": ";

         // filenames are case-sensitive, don't modify them
         value = (*plist_it)->value;
         if ( name.CmpNoCase("name") != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   // now output disposition parameters too
   String disposition;
   const MessageParameterList& dlist =
      m_mailMessage->GetDisposition(mimeDisplayPart,&disposition);

   if ( !strutil_isempty(disposition) )
      message << _("\nDisposition: ") << disposition.Lower() << '\n';

   if ( !dlist.empty() )
   {
      message += _("\nDisposition parameters:\n");
      for ( plist_it = dlist.begin(); plist_it != dlist.end(); plist_it++ )
      {
         name = (*plist_it)->name;
         message << NormalizeString(name) << ": ";

         value = (*plist_it)->value;
         if ( name.CmpNoCase("filename") != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   String title;
   title << _("MIME information for attachment #") << mimeDisplayPart;
   MDialog_Message(message, this, title);
}

// open (execute) a message attachment
void
wxMessageView::MimeHandle(int mimeDisplayPart)
{
   // we'll need this filename later
   wxString filenameOrig = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);

   String mimetype = m_mailMessage->GetPartMimeType(mimeDisplayPart);
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   if ( wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM") )
   {
      // special handling of "APPLICATION/OCTET-STREAM": this is the default
      // MIME type for all binary attachments and many e-mail clients don't
      // use the correct type (e.g. IMAGE/GIF) always leaving this one as
      // default. Try to guess a better MIME type ourselves from the file
      // extension.
      if ( !filenameOrig.IsEmpty() )
      {
         wxString ext;
         wxSplitPath(filenameOrig, NULL, NULL, &ext);

         if ( !ext.IsEmpty() )
            fileType = mimeManager.GetFileTypeFromExtension(ext);
      }
   }

   if ( !fileType )
   {
      // non default MIME type (so use it) or couldn't get the MIME type from
      // extension
      fileType = mimeManager.GetFileTypeFromMimeType(mimetype);
   }

   // First, we check for those contents that we handle in M itself:

   // handle internally MESSAGE/*
   if ( wxMimeTypesManager::IsOfType(mimetype, "MESSAGE/*") )
   {
#if 0
      // It's a pity, but creating a MessageCC from a string doesn't
      // quite work yet. :-(
      unsigned long len;
      char const *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
      if( !content )
      {
         wxLogError(_("Cannot get attachment content."));
         return;
      }
      Message *msg = Message::Create(content, 1);
      wxMessageViewFrame * f = new wxMessageViewFrame(NULL, 1, NULL, m_Parent);
      f->ShowMessage(msg);
      f->SetTitle(mimetype);
      msg->DecRef();
#else // 1
      bool ok = false;
      char *filename = wxGetTempFileName("Mtemp");
      if ( MimeSave(mimeDisplayPart, filename) )
      {
         wxString name;
         name.Printf(_("Attached message '%s'"),
                     GetFileNameForMIME(m_mailMessage, mimeDisplayPart).c_str());

         MFolder_obj mfolder = MFolder::CreateTemp
                               (
                                 name,
                                 MF_FILE, 0,
                                 filename
                               );

         if ( mfolder )
         {
            ASMailFolder *mf = ASMailFolder::OpenFolder(mfolder);
            if ( mf )
            {
               wxMessageViewFrame * f =
                  new wxMessageViewFrame(mf, 1, NULL, m_Parent);
               f->SetTitle(mimetype);

               ok = true;

               mf->DecRef();
            }
         }
      }

      if ( !ok )
      {
         wxLogError(_("Failed to open attached message."));
      }

      wxRemoveFile(filename);
#endif // 0/1

      return;
   }

   String
      filename = wxGetTempFileName("Mtemp"),
      filename2 = "";

   wxString ext;
   wxSplitPath(filenameOrig, NULL, NULL, &ext);
   // get the standard extension for such files if there is no real one
   if ( fileType != NULL && !ext)
   {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) && exts.GetCount() )
      {
         ext = exts[0u];
      }
   }

   // under Windows some programs will do different things depending on the
   // extensions of the input file (case in point: WinZip), so try to choose a
   // correct one
   wxString path, name, extOld;
   wxSplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      filename << '.' << ext;
   }

   MailMessageParameters params(filename, mimetype,
                                m_mailMessage, mimeDisplayPart);

   // We might fake a file, so we need this:
   bool already_saved = false;

#ifdef OS_UNIX
   /* For IMAGE/TIFF content, check whether it comes from one of the
      fax domains. If so, change the mimetype to "IMAGE/TIFF-G3" and
      proceed in the usual fashion. This allows the use of a special
      image/tiff-g3 mailcap entry. */
   if ( READ_CONFIG(m_Profile,MP_INCFAX_SUPPORT) &&
        (wxMimeTypesManager::IsOfType(mimetype, "IMAGE/TIFF")
         || wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM")))
   {
      kbStringList faxdomains;
      char *faxlisting = strutil_strdup(READ_CONFIG(m_Profile,
                                                    MP_INCFAX_DOMAINS));
      strutil_tokenise(faxlisting, ":;,", faxdomains);
      delete [] faxlisting;
      bool isfax = false;
      wxString domain;
      wxString fromline = m_mailMessage->From();
      strutil_tolower(fromline);

      for(kbStringList::iterator i = faxdomains.begin();
          i != faxdomains.end(); i++)
      {
         domain = **i;
         strutil_tolower(domain);
         if(fromline.Find(domain) != -1)
            isfax = true;
      }

      if(isfax
         && MimeSave(mimeDisplayPart,filename))
      {
         wxLogDebug("Detected image/tiff fax content.");
         // use TIFF2PS command to create a postscript file, open that
         // one with the usual ps viewer
         filename2 = filename.BeforeLast('.') + ".ps";
         String command;
         command.Printf(READ_CONFIG(m_Profile,MP_TIFF2PS),
                        filename.c_str(), filename2.c_str());
         // we ignore the return code, because next viewer will fail
         // or succeed depending on this:
         //system(command);  // this produces a postscript file on  success
         RunProcess(command);
         // We cannot use launch process, as it doesn't wait for the
         // program to finish.
         //wxString msg;
         //msg.Printf(_("Running '%s' to create Postscript file failed."), command.c_str());
         //(void)LaunchProcess(command, msg );

         wxRemoveFile(filename);
         filename = filename2;
         mimetype = "application/postscript";
         if(fileType) delete fileType;
         fileType = mimeManager.GetFileTypeFromMimeType(mimetype);
         // proceed as usual
         MailMessageParameters new_params(filename, mimetype,
                                          m_mailMessage,
                                          mimeDisplayPart);
         params = new_params;
         already_saved = true; // use this file instead!
      }
   }
#endif // Unix

   // We must save the file before actually calling GetOpenCommand()
   if( !already_saved )
   {
      MimeSave(mimeDisplayPart,filename);
      already_saved = TRUE;
   }
   String command;
   if ( (fileType == NULL) || !fileType->GetOpenCommand(&command, params) )
   {
      // unknown MIME type, ask the user for the command to use
      String prompt;
      prompt.Printf(_("Please enter the command to handle '%s' data:"),
                    mimetype.c_str());
      if ( !MInputBox(&command, _("Unknown MIME type"), prompt,
                      this, "MimeHandler") )
      {
         // cancelled by user
         return;
      }

      if ( command.IsEmpty() )
      {
         wxLogWarning(_("Do not know how to handle data of type '%s'."),
                      mimetype.c_str());
      }
      else
      {
         // the command must contain exactly one '%s' format specificator!
         String specs = strutil_extract_formatspec(command);
         if ( specs.IsEmpty() )
         {
            // at least the filename should be there!
            command += " %s";
         }

         // do expand it
         command = wxFileType::ExpandCommand(command, params);

         // TODO save this command to mailcap!
      }
      //else: empty command means try to handle it internally
   }

   if ( fileType )
      delete fileType;

   if ( ! command.IsEmpty() )
   {
      if(already_saved || MimeSave(mimeDisplayPart,filename))
      {
         wxString errmsg;
         errmsg.Printf(_("Error opening attachment: command '%s' failed"),
                       command.c_str());
         (void)LaunchProcess(command, errmsg, filename);
      }
   }
}

void
wxMessageView::MimeOpenWith(int mimeDisplayPart)
{
   // we'll need this filename later
   wxString filenameOrig = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);

   String mimetype = m_mailMessage->GetPartMimeType(mimeDisplayPart);
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   fileType = mimeManager.GetFileTypeFromMimeType(mimetype);

   String filename = wxGetTempFileName("Mtemp");

   wxString ext;
   wxSplitPath(filenameOrig, NULL, NULL, &ext);
   // get the standard extension for such files if there is no real one
   if ( fileType != NULL && !ext )
   {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) && exts.GetCount() )
      {
         ext = exts[0u];
      }
   }

   // under Windows some programs will do different things depending on the
   // extensions of the input file (case in point: WinZip), so try to choose a
   // correct one
   wxString path, name, extOld;
   wxSplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      filename << '.' << ext;
   }

   MailMessageParameters params(filename, mimetype,
                                m_mailMessage, mimeDisplayPart);

   String command;
   // ask the user for the command to use
   String prompt;
   prompt.Printf(_("Please enter the command to handle '%s' data:"),
                 mimetype.c_str());
   if ( !MInputBox(&command, _("Open with"), prompt,
                   this, "MimeHandler") )
   {
      // cancelled by user
      return;
   }

   if ( command.IsEmpty() )
   {
      wxLogWarning(_("Do not know how to handle data of type '%s'."),
                   mimetype.c_str());
   }
   else
   {
      // the command must contain exactly one '%s' format specificator!
      String specs = strutil_extract_formatspec(command);
      if ( specs.IsEmpty() )
      {
         // at least the filename should be there!
         command += " %s";
      }

      // do expand it
      command = wxFileType::ExpandCommand(command, params);

   }

   if ( ! command.IsEmpty() )
   {
      if ( MimeSave(mimeDisplayPart,filename) )
      {
         wxString errmsg;
         errmsg.Printf(_("Error opening attachment: command '%s' failed"),
                       command.c_str());
         (void)LaunchProcess(command, errmsg, filename);
      }
   }
}


bool
wxMessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
{
   String filename;

   if ( strutil_isempty(ifilename) )
   {
      filename = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);
      wxString path, name, ext;
      wxSplitPath(filename, &path, &name, &ext);

      filename = wxPFileSelector("MimeSave",_("Save attachment as:"),
                                 NULL, // no default path
                                 name, ext,
                                 NULL, wxFILEDLG_USE_FILENAME | wxSAVE | wxOVERWRITE_PROMPT, this);
   }
   else
      filename = ifilename;

   if ( !filename )
   {
      // no filename and user cancelled the dialog
      return false;
   }

   unsigned long len;
   const char *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
   if( !content )
   {
      wxLogError(_("Cannot get attachment content."));
   }
   else
   {
      wxFile out(filename, wxFile::write);
      if ( out.IsOpened() )
      {
         bool ok = true;

         // when saving messages to a file we need to "From stuff" them to
         // make them readable in a standard mail client (including this one)
         if ( m_mailMessage->GetPartType(mimeDisplayPart) ==
               Message::MSG_TYPEMESSAGE )
         {
            // standard prefix
            String fromLine = "From ";

            // find the from address
            const MessageParameterList& params =
               m_mailMessage->GetParameters(mimeDisplayPart);
            MessageParameterList::iterator i;
            for ( i = params.begin(); i != params.end(); i++ )
            {
               if ( (*i)->name == "From" )
               {
                  // found, now extract just the address
                  fromLine += Message::GetEMailFromAddress((*i)->value);
               }
            }

            if ( i == params.end() )
            {
               // this shouldn't normally happen, but if it does just make it
               // up (FIXME: now it always happens!)
               wxLogDebug("Couldn't find from header in the message");

               fromLine += "MAHOGANY-DUMMY-SENDER";
            }

            fromLine += ' ';

            // time stamp
            time_t t;
            time(&t);
            fromLine += ctime(&t);

            ok = out.Write(fromLine);
         }

         if ( ok )
         {
            // write the body
            ok = out.Write(content, len) == len;
         }

         if ( ok )
         {
            // only display in interactive mode
            if ( strutil_isempty(ifilename) )
            {
               wxLogStatus(GetFrame(this), _("Wrote %lu bytes to file '%s'"),
                           len, filename.c_str());
            }

            return true;
         }
      }
   }

   wxLogError(_("Could not save the attachment."));

   return false;
}

void
wxMessageView::MimeViewText(int mimeDisplayPart)
{
   unsigned long len;
   const char *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
   if ( content )
   {
      String title;
      title.Printf(_("Attachment #%d"), mimeDisplayPart);

      // add the filename if any
      String filename = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);
      if ( !filename.empty() )
      {
         title << " ('" << filename << "')";
      }

      MDialog_ShowText(this, title, content, "MimeView");
   }
   else
   {
      wxLogError(_("Failed to view the attachment."));
   }
}

// ----------------------------------------------------------------------------
// URL handling
// ----------------------------------------------------------------------------

void wxMessageView::PopupURLMenu(const String& url, const wxPoint& pt)
{
   UrlPopup menu(this, url);
   PopupMenu(&menu, pt);
}

void wxMessageView::OpenURL(const String& url, bool inNewWindow)
{
   wxFrame *frame = GetFrame(this);
   wxLogStatus(frame, _("Opening URL '%s'..."), url.c_str());

   wxBusyCursor bc;

   // the command to execute
   wxString command;

   bool bOk;
   if ( m_ProfileValues.browser.IsEmpty() )
   {
#ifdef OS_WIN
      // ShellExecute() always opens in the same window,
      // so do it manually for new window
      if ( inNewWindow )
      {
         wxRegKey key(wxRegKey::HKCR, url.BeforeFirst(':') + "\\shell\\open");
         if ( key.Exists() )
         {
            wxRegKey keyDDE(key, "DDEExec");
            if ( keyDDE.Exists() )
            {
               wxString ddeTopic = wxRegKey(keyDDE, "topic");

               // we only know the syntax of WWW_OpenURL DDE request
               if ( ddeTopic == "WWW_OpenURL" )
               {
                  wxString ddeCmd = keyDDE;

                  // this is a bit naive but should work as -1 can't appear
                  // elsewhere in the DDE topic, normally
                  if ( ddeCmd.Replace("-1", "0",
                                      FALSE /* only first occurence */) == 1 )
                  {
                     // and also replace the parameters
                     if ( ddeCmd.Replace("%1", url, FALSE) == 1 )
                     {
                        // magic incantation understood by wxMSW
                        command << "WX_DDE#"
                                << wxRegKey(key, "command") << '#'
                                << wxRegKey(keyDDE, "application") << '#'
                                << ddeTopic << '#'
                                << ddeCmd;
                     }
                  }
               }
            }
         }
      }

      if ( !command.empty() )
      {
         wxString errmsg;
         errmsg.Printf(_("Could not launch browser: '%s' failed."),
                       command.c_str());
         bOk = LaunchProcess(command, errmsg);
      }
      else // easy case: open in the same window
      {
         bOk = (int)ShellExecute(NULL, "open", url,
                                 NULL, NULL, SW_SHOWNORMAL ) > 32;
      }

      if ( !bOk )
      {
         wxLogSysError(_("Cannot open URL '%s'"), url.c_str());
      }
# else  // Unix
      // propose to choose program for opening URLs
      if (
         MDialog_YesNoDialog
         (
            _("No command configured to view URLs.\n"
              "Would you like to choose one now?"),
            frame,
            MDIALOG_YESNOTITLE,
            true,
            GetPersMsgBoxName(M_MSGBOX_ASK_URL_BROWSER)
            )
         )
         ShowOptionsDialog();

      if ( m_ProfileValues.browser.IsEmpty() )
      {
         wxLogError(_("No command configured to view URLs."));
         bOk = false;
      }
#endif // Win/Unix
   }
   else
   {
      // not empty, user provided his script - use it
      bOk = false;

#ifdef OS_UNIX
      if ( m_ProfileValues.browserIsNS ) // try re-loading first
      {
         wxString lockfile;
         wxGetHomeDir(&lockfile);
         lockfile << WXEXTHELP_SEPARATOR << ".netscape/lock";
         struct stat statbuf;

         if(lstat(lockfile.c_str(), &statbuf) == 0)
         // cannot use wxFileExists, because it's a link pointing to a
         // non-existing location      if(wxFileExists(lockfile))
         {
            command << m_ProfileValues.browser
                    << " -remote openURL(" << url;
            if ( inNewWindow )
            {
               command << ",new-window)";
            }
            else
            {
               command << ")";
            }
            wxString errmsg;
            errmsg.Printf(_("Could not launch browser: '%s' failed."),
                          command.c_str());
            bOk = LaunchProcess(command, errmsg);
         }
      }
#endif // Unix
      // either not netscape or ns isn't running or we have non-UNIX
      if(! bOk)
      {
         command = m_ProfileValues.browser;
         command << ' ' << url;

         wxString errmsg;
         errmsg.Printf(_("Couldn't launch browser: '%s' failed"),
                       command.c_str());

         bOk = LaunchProcess(command, errmsg);
      }

      if ( bOk )
         wxLogStatus(frame, _("Opening URL '%s'... done."),
                     url.c_str());
      else
         wxLogStatus(frame, _("Opening URL '%s' failed."),
                     url.c_str());
   }
}

// ----------------------------------------------------------------------------
// event handling
// ----------------------------------------------------------------------------

bool
wxMessageView::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_OptionsChange )
   {
      MEventOptionsChangeData& event = (MEventOptionsChangeData &)ev;

      // first of all, are we interested in this profile or not?
      Profile *profileChanged = event.GetProfile();
      if ( !profileChanged || !profileChanged->IsAncestor(m_Profile) )
      {
         // it's some profile which has nothing to do with us
         return true;
      }

      AllProfileValues settings;
      ReadAllSettings(&settings);
      if ( settings == m_ProfileValues )
      {
         // nothing changed
         return true;
      }

      switch ( event.GetChangeKind() )
      {
         case MEventOptionsChangeData::Apply:
         case MEventOptionsChangeData::Ok:
         case MEventOptionsChangeData::Cancel:
            // update everything
            m_ProfileValues = settings;
            break;

         default:
            FAIL_MSG("unknown options change event");
      }

      Update();
   }
   else if( ev.GetId() == MEventId_ASFolderResult )
   {
      OnASFolderResultEvent((MEventASFolderResultData &) ev);
   }

   return true;
}

void
wxMessageView::OnMouseEvent(wxCommandEvent &event)
{
   ClickableInfo *ci;

   wxLayoutObject *obj;
   obj = (wxLayoutObject *)event.GetClientData();
   ci = (ClickableInfo *)obj->GetUserData();
   if(ci)
   {
      switch( ci->GetType() )
      {
         case ClickableInfo::CI_ICON:
         {
            switch ( event.GetId() )
            {
               case WXLOWIN_MENU_RCLICK:
                  // show the menu
                  if ( m_MimePopup != NULL )
                     delete m_MimePopup; // recreate with new part number

                  // create the pop up menu now if not done yet
                  m_MimePopup = new MimePopup(this, ci->GetPart());
                  PopupMenu(m_MimePopup, m_ClickPosition);
                  break;

               case WXLOWIN_MENU_LCLICK:
                  // for now, do the same thing as double click: perhaps the
                  // left button behaviour should be configurable?

               case WXLOWIN_MENU_DBLCLICK:
                  // open
                  MimeHandle(ci->GetPart());
                  break;

               default:
                  FAIL_MSG("unknown mouse action");
            }
         }
         break;

         case ClickableInfo::CI_URL:
         {
            wxString url = ci->GetUrl();

            // treat mail urls separately:
            wxString protocol = url.BeforeFirst(':');
            if ( protocol == "mailto" )
            {
               wxComposeView *cv = wxComposeView::CreateNewMessage(m_Profile);
               cv->SetAddresses(ci->GetUrl().Right(ci->GetUrl().Length()-7));
               cv->InitText();
               cv->Show(TRUE);
               break;
            }

            if ( event.GetId() == WXLOWIN_MENU_RCLICK )
            {
               PopupURLMenu(url, m_ClickPosition);
            }
            else // left or double click
            {
               OpenURL(url, m_ProfileValues.browserInNewWindow);
            }
         }
         break;

         default:
            FAIL_MSG("unknown embedded object type");
      }

      ci->DecRef();
   }
}

bool
wxMessageView::DoMenuCommand(int id)
{
   UIdArray msgs;
   if( m_uid != UID_ILLEGAL )
      msgs.Add(m_uid);

   switch ( id )
   {
   case WXMENU_MSG_FIND:
      if(m_uid != UID_ILLEGAL)
         Find("");
      break;

   case WXMENU_MSG_REPLY:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ReplyMessage(m_mailMessage,
                                  MailFolder::Params(),
                                  m_Profile,
                                  GetFrame(this));
      break;
   case WXMENU_MSG_FOLLOWUP:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ReplyMessage(m_mailMessage,
                                  MailFolder::Params(MailFolder::REPLY_FOLLOWUP),
                                  m_Profile,
                                  GetFrame(this));
      break;
   case WXMENU_MSG_FORWARD:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ForwardMessage(m_mailMessage,
                                    MailFolder::Params(),
                                    m_Profile,
                                    GetFrame(this));
      break;

   case WXMENU_MSG_SAVE_TO_FOLDER:
      if(m_uid != UID_ILLEGAL && GetFolder())
         GetFolder()->SaveMessagesToFolder(&msgs, GetFrame(this));
      break;
   case WXMENU_MSG_SAVE_TO_FILE:
      if(m_uid != UID_ILLEGAL && GetFolder())
         GetFolder()->SaveMessagesToFile(&msgs, GetFrame(this));
      break;

   case WXMENU_MSG_DELETE:
      if(m_uid != UID_ILLEGAL && GetFolder())
         GetFolder()->DeleteMessages(&msgs);
      break;

   case WXMENU_MSG_UNDELETE:
      if(m_uid != UID_ILLEGAL && GetFolder())
         GetFolder()->UnDeleteMessages(&msgs);
      break;

   case WXMENU_MSG_PRINT:
      Print();
      break;
   case WXMENU_MSG_PRINT_PREVIEW:
      PrintPreview();
      break;
#ifdef USE_PS_PRINTING
   case WXMENU_MSG_PRINT_PS:
      break;
#endif

   case WXMENU_HELP_CONTEXT:
      mApplication->Help(MH_MESSAGE_VIEW,this);
      break;

   case WXMENU_MSG_TOGGLEHEADERS:
      {
         m_ProfileValues.showHeaders = !m_ProfileValues.showHeaders;
         m_Profile->writeEntry(MP_SHOWHEADERS, m_ProfileValues.showHeaders);
         UpdateShowHeadersInMenu();
         Update();
      }
      break;

   case WXMENU_EDIT_PASTE:
      Paste();
      Refresh();
      break;
   case WXMENU_EDIT_COPY:
      Copy();
      Refresh();
      break;
   case WXMENU_EDIT_CUT:
      Cut();
      Refresh();
      break;

   default:
      if ( WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT) )
      {
         SetLanguage(id);
         break;
      }

      // not handled
      return false;
   }

   // message handled
   return true;
}

void
wxMessageView::ShowMessage(ASMailFolder *folder, UIdType uid)
{
   if ( m_uid == uid )
      return;

   if ( uid == UID_ILLEGAL )
   {
      // don't show anything
      Clear();
   }
   else
   {
      SafeDecRef(m_folder);
      m_folder = folder;
      SafeIncRef(m_folder);

      // file request, our ShowMessage(Message *) will be called later
      (void)m_folder->GetMessage(uid, this);
   }
}

void
wxMessageView::ShowMessage(Message *mailMessage)
{
   CHECK_RET( mailMessage, "no message to show in wxMessageView" );

   mailMessage->IncRef();

   // size is measured in KBytes
   unsigned long size = mailMessage->GetSize() / 1024,
                 maxSize = (unsigned long)READ_CONFIG(m_Profile,
                                                      MP_MAX_MESSAGE_SIZE);

   if ( size > maxSize )
   {
      MailFolder *mf = mailMessage->GetFolder();
      CHECK_RET( mf, "message without folder?" );

      // local folders are supposed to be fast
      if ( !IsLocalQuickFolder(mf->GetType()) )
      {
         wxString msg;
         msg.Printf(_("The selected message is %u Kbytes long which is "
                      "more than the current threshold of %d Kbytes.\n"
                      "\n"
                      "Do you still want to download it?"),
                    size, maxSize);
         if ( !MDialog_YesNoDialog(msg, this) )
         {
            // don't do anything
            mailMessage->DecRef();
            return;
         }
      }
   }

   // ok, make this our new current message
   SafeDecRef(m_mailMessage);

   m_mailMessage = mailMessage;
   m_uid = mailMessage->GetUId();

   //have we not seen the message before?
   if( GetFolder() &&
     ! (m_mailMessage->GetStatus() & MailFolder::MSG_STAT_SEEN))
   {
      // mark it as seen
      m_mailMessage->GetFolder()->
        SetMessageFlag(m_uid, MailFolder::MSG_STAT_SEEN, true);

      // autocollect the address:
      /* FIXME for now it's here, should go somewhere else: */
      if ( m_ProfileValues.autocollect )
      {
         String addr, name;
         addr = m_mailMessage->Address(name, MAT_REPLYTO);

         String folderName = m_mailMessage->GetFolder() ?
            m_mailMessage->GetFolder()->GetName() : String(_("unknown"));

         AutoCollectAddresses(addr, name,
                              m_ProfileValues.autocollect,
                              m_ProfileValues.autocollectNamed != 0,
                              m_ProfileValues.autocollectBookName,
                              folderName,
                              (MFrame *)GetFrame(this));
         addr = m_mailMessage->Address(name, MAT_FROM);
         AutoCollectAddresses(addr, name,
                              m_ProfileValues.autocollect,
                              m_ProfileValues.autocollectNamed != 0,
                              m_ProfileValues.autocollectBookName,
                              folderName,
                              (MFrame *)GetFrame(this));
      }
   }

   Update();
}

bool
wxMessageView::Print(bool interactive)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   bool found;
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(m_ProfileValues.afmpath, false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
//      wxSetAFMPath(afmpath);
   {
      ((wxMApp *)mApplication)->GetPrintData()->SetFontMetricPath(afmpath);
      wxThePrintSetupData->SetAFMPath(afmpath);
   }
#endif // Win/Unix
   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrinter printer(& pdd);
#ifndef OS_WIN
   wxThePrintSetupData->SetAFMPath(afmpath);
#endif
   wxLayoutPrintout printout(GetLayoutList(), _("Mahogany: Printout"));

   if ( !printer.Print(this, &printout, interactive)
        && printer.GetLastError() != wxPRINTER_CANCELLED )
   {
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
      return FALSE;
   }
   else
   {
      (* ((wxMApp *)mApplication)->GetPrintData())
         = printer.GetPrintDialogData().GetPrintData();
      return TRUE;
   }
}

// tiny class to restore and set the default zoom level
class wxMVPreview : public wxPrintPreview
{
public:
   wxMVPreview(Profile *prof,
               wxPrintout *p1, wxPrintout *p2,
               wxPrintDialogData *dd)
      : wxPrintPreview(p1, p2, dd)
      {
         ASSERT(prof);
         m_Profile = prof;
         m_Profile->IncRef();
         SetZoom(READ_CONFIG(m_Profile, MP_PRINT_PREVIEWZOOM));
      }
   ~wxMVPreview()
      {
         m_Profile->writeEntry(MP_PRINT_PREVIEWZOOM, (long) GetZoom());
         m_Profile->DecRef();
      }
private:
   Profile *m_Profile;
};

void
wxMessageView::PrintPreview(void)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(m_ProfileValues.afmpath, false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   bool found;
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
      wxSetAFMPath(afmpath);
#endif // in/Unix

   // Pass two printout objects: for preview, and possible printing.
   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrintPreview *preview = new wxMVPreview(
      m_Profile,
      new wxLayoutPrintout(GetLayoutList()),
      new wxLayoutPrintout(GetLayoutList()),
      &pdd
      );
   if( !preview->Ok() )
   {
      wxMessageBox(_("There was a problem with showing the preview:\n"
                     "perhaps your current printer is not set correctly?"),
                   _("Previewing"), wxOK);
      return;
   }

   (* ((wxMApp *)mApplication)->GetPrintData())
      = preview->GetPrintDialogData().GetPrintData();

   wxPreviewFrame *frame = new wxPreviewFrame(preview, NULL, //GetFrame(m_Parent),
                                              _("Print Preview"),
                                              wxPoint(100, 100), wxSize(600, 650));
   frame->Centre(wxBOTH);
   frame->Initialize();
   frame->Show(TRUE);
}

bool
wxMessageView::RunProcess(const String& command)
{
   wxLogStatus(GetFrame(this),
               _("Calling external viewer '%s'"),
               command.c_str());
   return wxExecute(command, true) == 0;
}

bool
wxMessageView::LaunchProcess(const String& command,
                             const String& errormsg,
                             const String& filename)
{
   wxLogStatus(GetFrame(this),
               _("Calling external viewer '%s'"),
               command.c_str());

   wxProcess *process = new wxProcess(this);
   int pid = wxExecute(command, false, process);
   if ( !pid )
   {
      delete process;

      if ( !errormsg.IsEmpty() )
         wxLogError("%s.", errormsg.c_str());

      return false;
   }

   if ( pid != -1 )
   {
      ProcessInfo *procInfo = new ProcessInfo(process, pid, errormsg, filename);

      m_processes.Add(procInfo);
   }

   return true;
}

void
wxMessageView::OnProcessTermination(wxProcessEvent& event)
{
   // find the corresponding entry in m_processes
   size_t n, procCount = m_processes.GetCount();
   for ( n = 0; n < procCount; n++ )
   {
      if ( m_processes[n]->GetPid() == event.GetPid() )
         break;
   }

   CHECK_RET( n != procCount, "unknown process terminated!" );

   ProcessInfo *info = m_processes[n];
   if ( event.GetExitCode() != 0 )
   {
      wxLogError(_("%s (external viewer exited with non null exit code)"),
                 info->GetErrorMsg().c_str());
   }

#if 0
   MTempFileName *tempfile = info->GetTempFile();
   if ( tempfile )
   {
      // tell it that it's ok to delete the temp file
      //FIXME: Ok() tells it not to delete it!tempfile->Ok();
   }
#endif

   m_processes.Remove(n);
   delete info;
}


void
wxMessageView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   ASMailFolder::Result *result = event.GetResult();
   if (result->GetUserData() == this )
   {
      switch(result->GetOperation())
      {
         case ASMailFolder::Op_GetMessage:
         {
            /* The only situation where we receive a Message, is if we
               want to open it in a separate viewer. */
            Message *mptr =
               ((ASMailFolder::ResultMessage *)result)->GetMessage();

            if(mptr && mptr->GetUId() != m_uid)
            {
               ShowMessage(mptr);
               wxFrame *frame = GetFrame(this);
               if ( wxDynamicCast(frame, wxMessageViewFrame) )
               {
                  wxString fmt = READ_CONFIG(m_Profile, MP_MVIEW_TITLE_FMT);
                  MsgVarExpander expander(mptr);
                  frame->SetTitle(ParseMessageTemplate(fmt, expander));
               }
            }
            SafeDecRef(mptr);
         }
         break;

         default:
            FAIL_MSG("Unexpected async result event");
      }
   }

   result->DecRef();
}

void
wxMessageView::UpdateShowHeadersInMenu()
{
   wxFrame *frame = GetFrame(this);
   CHECK_RET( frame, "message view without parent frame?" );

   wxMenuBar *mbar = frame->GetMenuBar();
   CHECK_RET( mbar, "message view frame without menu bar?" );

   mbar->Check(WXMENU_MSG_TOGGLEHEADERS, m_ProfileValues.showHeaders);
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void wxMessageView::EmulateKeyPress(int keycode)
{
   wxKeyEvent event;
   event.m_keyCode = keycode;

   wxScrolledWindow::OnChar(event);
}

/// scroll down one line:
void
wxMessageView::LineDown(void)
{
   EmulateKeyPress(WXK_DOWN);
}

/// scroll down one page:
void
wxMessageView::PageDown(void)
{
   EmulateKeyPress(WXK_PAGEDOWN);
}

/// scroll up one page:
void
wxMessageView::PageUp(void)
{
   EmulateKeyPress(WXK_PAGEUP);
}

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMessageViewFrame, wxMFrame)
   EVT_SIZE(wxMessageViewFrame::OnSize)
   EVT_MENU(-1, wxMessageViewFrame::OnCommandEvent)
   EVT_TOOL(-1, wxMessageViewFrame::OnCommandEvent)
END_EVENT_TABLE()

wxMessageViewFrame::wxMessageViewFrame(ASMailFolder *folder,
                                       long num,
                                       wxFolderView *fv,
                                       MWindow  *parent,
                                       const String& name)
                  : wxMFrame(name.empty() ? String(_("Mahogany: Message View"))
                                          : name, parent)
{
   m_MessageView = NULL;

   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   AddLanguageMenu();

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
   AddToolbarButtons(CreateToolBar(), WXFRAME_MESSAGE);
   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);

   Show(true);
   m_MessageView = new wxMessageView(folder, num, fv, this);
   wxSizeEvent se; // unused
   OnSize(se);
}

void
wxMessageViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   if(! m_MessageView->DoMenuCommand(id))
   {
      switch(id)
      {
      case WXMENU_MSG_EXPUNGE:
      case WXMENU_MSG_SELECTALL:
      case WXMENU_MSG_DESELECTALL:
         if(m_MessageView->GetFolderView())
            m_MessageView->GetFolderView()->OnCommandEvent(event);
         break;

      default:
         wxMFrame::OnMenuCommand(event.GetId());
      }
   }
}

void
wxMessageViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x, y;
   GetClientSize( &x, &y );
   if(m_MessageView)
      m_MessageView->SetSize(0,0,x,y);
}

IMPLEMENT_DYNAMIC_CLASS(wxMessageViewFrame, wxMFrame)

#endif // EXPERIMENTAL_karsten

