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

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
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
#include "MDialogs.h"
#include "MessageView.h"
#include "XFace.h"
#include "miscutil.h"
#include "sysutil.h"

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

#include <ctype.h>  // for isspace
#include <time.h>   // for time stamping autocollected addresses

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
   MimePopup(wxMessageView *parent, int partno) : wxMenu(_("MIME Menu"))
      {
         // init member vars
         m_PartNo = partno;
         m_MessageView = parent;

         // create the menu items
         Append(WXMENU_MIME_INFO,_("&Info..."));
         AppendSeparator();
         Append(WXMENU_MIME_HANDLE,_("&Open"));
         Append(WXMENU_MIME_SAVE,_("&Save..."));
      }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   wxMessageView *m_MessageView;
   int m_PartNo;

   DECLARE_EVENT_TABLE()
};

// the message parameters for the MIME type manager
class MailMessageParameters : public wxFileType::MessageParameters
{
public:
   MailMessageParameters(const wxString& filename,
         const wxString& mimetype,
         Message *m_mailMessage,
         int part)
      : wxFileType::MessageParameters(filename, mimetype)
      {
         m_mailMessage = m_mailMessage;
         m_part = part;
      }

   virtual wxString GetParamValue(const wxString& name) const;

private:
   Message *m_mailMessage;
   int m_part;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MimePopup, wxMenu)
   EVT_MENU(-1, MimePopup::OnCommandEvent)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================


static String
GetParameter(Message *msg, int partno, const String &param)
{
   const MessageParameterList &plist = msg->GetParameters(partno);
   MessageParameterList::iterator plist_it;
   if(plist.size() > 0)
   {
      for(plist_it = plist.begin(); plist_it != plist.end();
          plist_it++)
      {
         if(Stricmp((*plist_it)->name, param) == 0)
            return (*plist_it)->value;
      }
   }

   const MessageParameterList &plist2 = msg->GetDisposition(partno);
   if(plist2.size() > 0)
   {
      for(plist_it = plist2.begin(); plist_it != plist2.end();
          plist_it++)
      {
         if(Stricmp((*plist_it)->name, param) == 0)
            return (*plist_it)->value;
      }
   }
   return "";
}


static
String GetFileNameForMIME(Message *message, int partNo)
{
   String fileName;
   fileName = GetParameter(message, partNo, "FILENAME");
   if(fileName.Length() == 0)
      fileName = GetParameter(message, partNo,"NAME");
   return fileName;
}


void
MimePopup::OnCommandEvent(wxCommandEvent &event)
{
   switch(event.GetId())
   {
   case WXMENU_MIME_INFO:
      m_MessageView->MimeInfo(m_PartNo);
      break;
   case WXMENU_MIME_HANDLE:
      m_MessageView->MimeHandle(m_PartNo);
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
// wxMessageView::AllProfileValues
// ----------------------------------------------------------------------------

bool wxMessageView::AllProfileValues::operator==(const AllProfileValues& other)
{
   #define CMP(x) (x == other.x)

   return CMP(BgCol) && CMP(FgCol) && CMP(UrlCol) &&
          CMP(HeaderNameCol) && CMP(HeaderValueCol) &&
          CMP(font) && CMP(size) &&
          CMP(showHeaders) && CMP(rfc822isText) &&
          CMP(highlightURLs) && CMP(inlineGFX) &&
          CMP(browser) && CMP(browserIsNS) &&
          CMP(autocollect) && CMP(autocollectNamed) &&
          CMP(autocollectBookName) &&
#ifdef OS_UNIX
          CMP(afmpath) &&
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
   mimeDisplayPart = 0;
   xface = NULL;
   xfaceXpm = NULL;
   m_Parent = parent;
   m_FolderView = fv;
   m_MimePopup = NULL;
   m_uid = UID_ILLEGAL;
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


wxMessageView::wxMessageView(wxFolderView *fv, wxWindow *parent)
             : wxLayoutWindow(parent)
{
   m_folder = NULL;
   m_Profile = NULL;
   Create(fv,parent);
   Show(TRUE);
}

wxMessageView::wxMessageView(ASMailFolder *folder,
                             long num,
                             wxFolderView *fv,
                             wxWindow *parent)
             : wxLayoutWindow(parent)
{
   m_folder = folder;
   if(m_folder) m_folder->IncRef();
   m_Profile = NULL;
   Create(fv,parent);
   if(folder)
      ShowMessage(folder,num);
   Show(TRUE);
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
wxMessageView::SetParentProfile(ProfileBase *profile)
{
   SafeDecRef(m_Profile);

   if(profile)
   {
      m_Profile = profile;/*ProfileBase::CreateProfile("MessageView", profile);*/
      m_Profile->IncRef();
   }
   else
      m_Profile = ProfileBase::CreateEmptyProfile();

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
   GET_COLOUR_FROM_PROFILE(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderValueCol, HEADER_VALUES_COLOUR);

   #undef GET_COLOUR_FROM_PROFILE

   settings->font = READ_CONFIG(m_Profile,MP_MVIEW_FONT);
   ASSERT(settings->font >= 0 && settings->font <= NUM_FONTS);

   settings->font = wxFonts[settings->font];
   settings->size = READ_CONFIG(m_Profile,MP_MVIEW_FONT_SIZE);
   settings->showHeaders = READ_CONFIG(m_Profile,MP_SHOWHEADERS) != 0;
   settings->rfc822isText = READ_CONFIG(m_Profile,MP_RFC822_IS_TEXT) != 0;
   settings->highlightURLs = READ_CONFIG(m_Profile,MP_HIGHLIGHT_URLS) != 0;
   settings->inlineGFX = READ_CONFIG(m_Profile, MP_INLINE_GFX) != 0;
   settings->browser = READ_CONFIG(m_Profile, MP_BROWSER);
   settings->browserIsNS = READ_CONFIG(m_Profile, MP_BROWSER_ISNS) != 0;
   settings->autocollect =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT);
   settings->autocollectNamed =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT_NAMED);
   settings->autocollectBookName = READ_CONFIG(m_Profile, MP_AUTOCOLLECT_ADB);
   settings->showFaces = READ_CONFIG(m_Profile, MP_SHOW_XFACES) != 0;

#ifdef OS_UNIX
   settings->afmpath = READ_APPCONFIG(MP_AFMPATH);
#endif // Unix

#ifndef OS_WIN
   SetFocusFollowMode(READ_CONFIG(m_Profile,MP_FOCUS_FOLLOWSMOUSE) != 0);
#endif
   SetWrapMargin(READ_CONFIG(m_Profile, MP_WRAPMARGIN));
}

void
wxMessageView::Clear(void)
{
   SetCursorVisibility(-1); // on demand
   wxLayoutWindow::Clear(m_ProfileValues.font, m_ProfileValues.size,
                         (int)wxNORMAL, (int)wxNORMAL, 0,
                         &m_ProfileValues.FgCol,
                         &m_ProfileValues.BgCol);
   SetBackgroundColour( m_ProfileValues.BgCol );
   GetLayoutList()->SetAutoFormatting(FALSE); // speeds up insertion
   // of text
   m_uid = UID_ILLEGAL;
}

void
wxMessageView::Update(void)
{
   int i,n,t;
   char const * cptr;
   String tmp,from,url, mimeType, fileName, disposition;
   bool   lastObjectWasIcon = false; // a flag
   ClickableInfo *ci;

   wxLayoutList *llist = GetLayoutList();
   wxLayoutObject *obj = NULL;

   Clear();

   if(! m_mailMessage)  // no message to display
      return;

   m_uid = m_mailMessage->GetUId();

   // if wanted, display all header lines
   if(m_ProfileValues.showHeaders)
   {
      String
         tmp = m_mailMessage->GetHeader();
      char *buf = strutil_strdup(tmp);
      wxLayoutImportText(llist,buf);
      delete [] buf;
      llist->LineBreak();
   }

#ifdef HAVE_XFACES
   // need real XPM support in windows
#ifndef OS_WIN
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
#endif // !Windows
#endif // HAVE_XFACES

   // show the configurable headers
   String headersString(READ_CONFIG(m_Profile, MP_MSGVIEW_HEADERS));
   String headerName, headerValue;
   for ( const char *p = headersString.c_str(); *p != '\0'; p++ )
   {
      if ( *p == ':' )
      {
         // first get the value of this header
         m_mailMessage->GetHeaderLine(headerName, headerValue);

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

            wxLayoutImportText(llist,headerValue);

            llist->LineBreak();
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

   // iterate over all parts
   n = m_mailMessage->CountParts();
   for(i = 0; i < n; i++)
   {
      t = m_mailMessage->GetPartType(i);
      if(m_mailMessage->GetPartSize(i) == 0)
         continue; // ignore empty parts

      mimeType = m_mailMessage->GetPartMimeType(i);
      strutil_tolower(mimeType);
      fileName = GetFileNameForMIME(m_mailMessage,i);
      (void) m_mailMessage->GetDisposition(i,&disposition);
      strutil_tolower(disposition);
#ifdef DEBUG
      obj = NULL;
#endif

      if( t == Message::MSG_TYPEAPPLICATION) // let's guess a little
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
         - if it is text/plain and not "attachment" or with a filename
         - if it is rfc822 and it is configured to be displayed
         - HTML is for now displayed as normal text
      */
      if (
         ((fileName.Length() == 0) && (disposition != "attachment"))
         && ( (mimeType == "text/plain" || (mimeType == "text/html")
               || (t == Message::MSG_TYPEMESSAGE
                   && (m_ProfileValues.rfc822isText != 0)))
            )
         )
      {
         unsigned long len;
         cptr = m_mailMessage->GetPartContent(i, &len);
         if(cptr == NULL)
            continue; // error ?
         llist->LineBreak();
         if( m_ProfileValues.highlightURLs )
         {
            tmp = cptr;
            String url;
            String before;

            do
            {
               before =  strutil_findurl(tmp,url);
               wxLayoutImportText(llist,before);
               if(!strutil_isempty(url))
               {
                  ci = new ClickableInfo(url);
                  obj = new wxLayoutObjectText(url);
                  obj->SetUserData(ci);
                  ci->DecRef();
                  llist->SetFontColour(& m_ProfileValues.UrlCol);
                  llist->Insert(obj);
                  llist->SetFontColour(& m_ProfileValues.FgCol);
               }
            }
            while( !strutil_isempty(tmp) );

            lastObjectWasIcon = false;
         }
      }
      else
         /* This block captures all non-inlined message parts. They get
            represented by an icon.
            In case of image content, we check whether it might be a
            Fax message. */
      {
         wxString mimeFileName = GetFileNameForMIME(m_mailMessage, i);
         if(t == Message::MSG_TYPEIMAGE && m_ProfileValues.inlineGFX)
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
         else
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
                  << strutil_ultoa(m_mailMessage->GetPartSize(i, true)) << _(" bytes");
            ci = new ClickableInfo(i, label);
            obj->SetUserData(ci); // gets freed by list
            ci->DecRef();
            llist->Insert(obj);
            // add extra whitespace so lines with multiple icons can
            // be broken:
            llist->Insert(" ");
         }
         lastObjectWasIcon = true;
      }
   }
   llist->LineBreak();
   llist->MoveCursorTo(wxPoint(0,0));
   // we have modified the list directly, so we need to mark the
   // wxlwindow as dirty:
   SetDirty();
   // re-enable auto-formatting, seems safer for selection
   // highlighting, not sure if needed, though
   llist->SetAutoFormatting(TRUE);

   SetWrapMargin(READ_CONFIG(m_Profile, MP_WRAPMARGIN));
   if(m_WrapMargin > 0 && READ_CONFIG(m_Profile, MP_AUTOMATIC_WORDWRAP) != 0)
      llist->WrapAll(m_WrapMargin);
   // yes, we allow the user to edit the buffer, in case he wants to
   // modify it for pasting or wrap lines manually:
   SetEditable(FALSE);
   SetCursorVisibility(-1);
   GetLayoutList()->ForceTotalLayout();
   // for safety, this is required to set scrollbars
   RequestUpdate();
}

String
wxMessageView::HighLightURLs(const char *input)
{
   String out;
   const char *cptr = input;

   while(*cptr)
   {

      if(strncmp(cptr, "http:", 5) == 0 || strncmp(cptr, "ftp:", 4) == 0)
      {
         const char *cptr2 = cptr;
         out << " <a href=\"" << "\"";
         while(*cptr2 && ! isspace(*cptr2))
            out += *cptr2++;
         out += "\"> ";
      }
      else
      {
         // escape brackets:
         if(*cptr == '<')
            out += '<';
         else
            if(*cptr == '>')
               out += '>';
      }
      out += *cptr++;
   }

   return out;
}


// show information about an attachment
void
wxMessageView::MimeInfo(int mimeDisplayPart)
{
   String message;
   message << _("MIME type: ")
           << m_mailMessage->GetPartMimeType(mimeDisplayPart)
           << '\n';

   String tmp = m_mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message << '\n' << _("Description: ") << tmp << '\n';

   message << _("Size: ")
           << strutil_ltoa(m_mailMessage->GetPartSize(mimeDisplayPart));

   Message::ContentType type = m_mailMessage->GetPartType(mimeDisplayPart);
   if(type == Message::MSG_TYPEMESSAGE || type == Message::MSG_TYPETEXT)
      message << _(" lines");
   else
      message << _(" bytes");
   message << '\n';

   // debug output with all parameters
   const MessageParameterList &plist = m_mailMessage->GetParameters(mimeDisplayPart);
   MessageParameterList::iterator plist_it;
   if(plist.size() > 0)
   {
      message += _("\nParameters:\n");
      for(plist_it = plist.begin(); plist_it != plist.end();
          plist_it++)
      {
         message += (*plist_it)->name;
         message += ": ";
         message += (*plist_it)->value;
         message += '\n';
      }
   }
   String disposition;
   const MessageParameterList &dlist = m_mailMessage->GetDisposition(mimeDisplayPart,&disposition);
   if(! strutil_isempty(disposition))
      message << _("\nDisposition: ") << disposition << '\n';
   if(dlist.size() > 0)
   {
      message += _("\nDisposition Parameters:\n");
      for(plist_it = dlist.begin(); plist_it != dlist.end();
          plist_it++)
         message << (*plist_it)->name << ": "
                 << (*plist_it)->value << '\n';
   }

   MDialog_Message(message, this, _("MIME information"));
}

// open (execute) a message attachment
void
wxMessageView::MimeHandle(int mimeDisplayPart)
{
   // we'll need this filename later
   wxString filenameOrig = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);

#if 0
   // look for "FILENAME" parameter:
   (void)m_mailMessage->ExpandParameter
         (
            m_mailMessage->GetDisposition(mimeDisplayPart),
            "FILENAME",
            &filenameOrig
         );
   if(filenameOrig.Length() == 0)
   {
      // look for "NAME" parameter:
      (void)m_mailMessage->ExpandParameter
         (
            m_mailMessage->GetDisposition(mimeDisplayPart),
            "NAME",
            &filenameOrig
            );
   }
#endif // 0

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

   /* First, we check for those contents that we handle in M itself: */
   // this we handle internally
   if(mimetype.length() >= strlen("MESSAGE") &&
      mimetype.Left(strlen("MESSAGE")) == "MESSAGE")
   {
#if 0
      // It´s a pity, but creating a MessageCC from a string doesn´t
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
#endif

      char *filename = wxGetTempFileName("Mtemp");
      if(MimeSave(mimeDisplayPart,filename))
      {
         ASMailFolder *mf = ASMailFolder::OpenFolder(MF_FILE, filename);
         wxMessageViewFrame * f = new wxMessageViewFrame(mf, 1, NULL, m_Parent);
         f->SetTitle(mimetype);
         mf->DecRef();
      }
      wxRemoveFile(filename);
      return;
   }

   String
      filename = wxGetTempFileName("Mtemp"),
      filename2 = "";

#  ifdef OS_WIN
   // get the standard extension for such files
   wxString ext;
   if ( fileType != NULL ) {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) ) {
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

      filename = path + wxFILE_SEP_PATH + name + ext;
   }
#  endif // Win

   MailMessageParameters
      params(filename, mimetype, m_mailMessage, mimeDisplayPart);

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
         filename2 = filename + ".ps";
         String command;
         command.Printf(READ_CONFIG(m_Profile,MP_TIFF2PS),
                        filename.c_str(), filename2.c_str());
         // we ignore the return code, because next viewer will fail
         // or succeed depending on this:
         //system(command);  // this produces a postscript file on  success
         wxExecute(command);
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

bool
wxMessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
{
   String filename;

   if ( strutil_isempty(ifilename) )
   {
      filename = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);
#if 0
      (void)m_mailMessage->ExpandParameter
            (
               m_mailMessage->GetDisposition(mimeDisplayPart),
               "FILENAME",
               &filename
            );
      if(filename.Length() == 0)
         (void)m_mailMessage->ExpandParameter
            (
               m_mailMessage->GetDisposition(mimeDisplayPart),
               "NAME",
               &filename
               );
#endif

      wxString path, name, ext;
      wxSplitPath(filename, &path, &name, &ext);

      name << '.' << ext;
      filename = wxPFileSelector("MimeSave",_("Save attachment as:"),
                                 path, name, ext,
                                 NULL, 0, this);
   }
   else
      filename = ifilename;

   if ( !filename )
   {
      // no filename and user cancelled the dialog
      return false;
   }

   unsigned long len;
   char const *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
   if( !content )
   {
      wxLogError(_("Cannot get attachment content."));
   }
   else
   {
      wxFile out(filename, wxFile::write);
      if ( out.IsOpened() )
      {
         size_t written = out.Write(content, len);
         if ( written == len )
         {
            // only display in interactive mode
            if( strutil_isempty(ifilename) )
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

bool
wxMessageView::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_OptionsChange )
   {
      MEventOptionsChangeData& event = (MEventOptionsChangeData &)ev;

      // first of all, are we interested in this profile or not?
      ProfileBase *profileChanged = event.GetProfile();
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
                  PopupMenu(m_MimePopup, m_ClickPosition.x, m_ClickPosition.y);
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
            wxFrame *frame = GetFrame(this);

            // wxYield() hangs the program in the release build under Windows
            /*
              wxLogStatus(frame, _("Opening URL '%s'..."), ci->url.c_str());
              wxYield();  // let the status bar update itself
            */

            wxBeginBusyCursor();

            // treat mail urls separately:
            if(ci->GetUrl().Left(7) == "mailto:")
            {
               wxEndBusyCursor();
               wxComposeView *cv = wxComposeView::CreateNewMessage(frame, m_Profile);
               cv->SetAddresses(ci->GetUrl().Right(ci->GetUrl().Length()-7));
               cv->InitText();
               cv->Show(TRUE);
               break;
            }
            bool bOk;
            if ( m_ProfileValues.browser.IsEmpty() )
            {
#ifdef OS_WIN
               bOk = (int)ShellExecute(NULL, "open", ci->GetUrl(),
                                       NULL, NULL, SW_SHOWNORMAL ) > 32;
               if ( !bOk )
               {
                  wxLogSysError(_("Cannot open URL '%s'"),
                                ci->GetUrl().c_str());
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
                     "AskUrlBrowser"
                     )
                  )
                  ShowOptionsDialog();

               if ( m_ProfileValues.browser.IsEmpty() )
               {
                  wxLogError(_("No command configured to view URLs."));
                  bOk = FALSE;
               }
#                 endif
            }
            else
            {
               // not empty, user provided his script - use it
               wxString command;
               bOk = false;
#ifdef OS_UNIX
               if(m_ProfileValues.browserIsNS) // try re-loading first
               {
                  wxString lockfile;
                  wxGetHomeDir(&lockfile);
                  lockfile << WXEXTHELP_SEPARATOR << ".netscape/lock";
                  struct stat statbuf;
                  if(lstat(lockfile.c_str(), &statbuf) == 0)
                     // cannot use wxFileExists, because it's a link pointing to a
                     // non-existing location      if(wxFileExists(lockfile))
                  {
                     command = "";
                     command << m_ProfileValues.browser << " -remote openURL(" << ci->GetUrl() << ")";
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
                  command << ' ' << ci->GetUrl();

                  wxString errmsg;
                  errmsg.Printf(_("Couldn't launch browser: '%s' failed"),
                                command.c_str());

                  bOk = LaunchProcess(command, errmsg);
               }

               wxEndBusyCursor();

               if ( bOk )
                  wxLogStatus(frame, _("Opening URL '%s'... done."),
                              ci->GetUrl().c_str());
               else
                  wxLogStatus(frame, _("Opening URL '%s' failed."),
                              ci->GetUrl().c_str());
            }
            break;
         }
         default:
            FAIL_MSG("unknown embedded object type");
      }
   ci->DecRef();
   }
}

bool
wxMessageView::ShowRawText(void)
{
   String text;
   m_mailMessage->WriteToString(text, true);
   if ( text.IsEmpty() )
   {
      wxLogError(_("Failed to get the raw text of the message."));

      return false;
   }
   MDialog_ShowText(m_Parent, _("Raw message text"), text, "RawMsgPreview");
   return true;
}

bool
wxMessageView::DoMenuCommand(int id)
{
   UIdArray msgs;
   if( m_uid != UID_ILLEGAL )
      msgs.Add(m_uid);

   bool handled = true;
   switch ( id )
   {
   case WXMENU_MSG_FIND:
      if(m_uid != UID_ILLEGAL)
         Find("");
      break;
   case WXMENU_MSG_REPLY:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ReplyMessage(m_mailMessage,0,m_Profile,GetFrame(this));
      break;
   case WXMENU_MSG_FOLLOWUP:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ReplyMessage(m_mailMessage,MailFolder::REPLY_FOLLOWUP,m_Profile,GetFrame(this));
      break;
   case WXMENU_MSG_FORWARD:
      if(m_uid != UID_ILLEGAL)
         MailFolder::ForwardMessage(m_mailMessage,m_Profile,GetFrame(this));
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
      Update();
   }
   break;

   case WXMENU_MSG_SHOWRAWTEXT:
      ShowRawText();
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
      handled = false;
   }

   return handled;
}

void
wxMessageView::ShowMessage(ASMailFolder *folder, UIdType uid)
{
   if(uid == UID_ILLEGAL)
   {
      Clear();
      return;
   }
   if ( m_uid == uid )
      return;
   SafeDecRef(m_folder);
   m_folder = folder;
   SafeIncRef(m_folder);
   (void) m_folder->GetMessage(uid, this); // file request
}

void
wxMessageView::ShowMessage(Message *mailMessage)
{
   mailMessage->IncRef();
   CHECK_RET( mailMessage, "no message with such uid" );

   unsigned long size = 0,
                 maxSize = (unsigned long)READ_CONFIG(m_Profile,
                                                      MP_MAX_MESSAGE_SIZE);

   // we're only interested in the size, not status flags
   (void)mailMessage->GetStatus(&size);
   size /= 1024;  // size is measured in KBytes

   if ( size > maxSize && (mailMessage->GetFolder()
                           &&
                           GetFolderType(mailMessage->GetFolder()->GetType()) != MF_FILE
                           && GetFolderType(mailMessage->GetFolder()->GetType()) != MF_MH))
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

   // ok, make this our new current message
   SafeDecRef(m_mailMessage);

   m_mailMessage = mailMessage;
   m_uid = mailMessage->GetUId();

   if( GetFolder() && ! (m_mailMessage->GetStatus() & MailFolder::MSG_STAT_SEEN))
      m_mailMessage->GetFolder()->SetMessageFlag(m_uid, MailFolder::MSG_STAT_SEEN, true);

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
   Update();
}


void
wxMessageView::Print(void)
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
   if ( !printer.Print(this, &printout, TRUE)
      && ! printer.GetAbort() )
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
   else
      (* ((wxMApp *)mApplication)->GetPrintData())
         = printer.GetPrintDialogData().GetPrintData();
}

// tiny class to restore and set the default zoom level
class wxMVPreview : public wxPrintPreview
{
public:
   wxMVPreview(ProfileBase *prof,
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
   ProfileBase *m_Profile;
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
   else
   {
      ProcessInfo *procInfo = new ProcessInfo(process, pid, errormsg, filename);

      m_processes.Add(procInfo);

      return true;
   }
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
            Message *mptr = ((ASMailFolder::ResultMessage *)result)->GetMessage();

            if(mptr && mptr->GetUId() != m_uid)
            {
               ShowMessage(mptr);
               wxFrame *frame = GetFrame(this);
               if(frame && frame->IsKindOf(CLASSINFO(wxMessageViewFrame)))
               {
                  wxString title;
                  title << mptr->Subject() << _(" , from ") << mptr->From();
                  frame->SetTitle(title);
               }
            }
            SafeDecRef(mptr);
         }
         break;

         default:
            ASSERT_MSG(0,"Unexpected async result event");
      }
   }

   result->DecRef();
}



/// scroll down one page:
void
wxMessageView::PageDown(void)
{
   GetLayoutList()->MoveCursorVertically(20); // FIXME: ugly hard-coded line count
   ScrollToCursor();
}

/// scroll up one page:
void
wxMessageView::PageUp(void)
{
   GetLayoutList()->MoveCursorVertically(-20); // FIXME: ugly hard-coded line count
   ScrollToCursor();
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
                                       const String &iname)
{
   m_MessageView = NULL;

   wxString name = iname;
   if(name.Length() == 0)
      name = "Mahogany : MessageView";
   wxMFrame::Create(name, parent);

   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();

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
