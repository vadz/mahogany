/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998, 1999 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
#pragma implementation "wxMessageView.h"
#endif

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
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "MDialogs.h"

#include "MessageView.h"

#include "XFace.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"
#include "gui/wxMessageView.h"
#include "gui/wxFolderView.h"
#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"

#include "gui/wxOptionsDlg.h"

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/mimetype.h>
#include "miscutil.h"
#include "gui/wxComposeView.h"

#include "sysutil.h"

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
      { m_type = CI_URL; }
   ClickableInfo(int id)
      {
         m_id = id;
         m_type = CI_ICON;
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

   GCC_DTOR_WARN_OFF();
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
END_EVENT_TABLE()

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
   m_uid = -1;
   SetFocus();
   SetMouseTracking();
   SetParentProfile(fv ? fv->GetProfile() : NULL);
}


wxMessageView::wxMessageView(wxFolderView *fv, wxWindow *parent)
             : wxLayoutWindow(parent)
{
   m_folder = NULL;
   m_Profile = NULL;
   Create(fv,parent);
   Show(TRUE);
}

wxMessageView::wxMessageView(MailFolder *folder,
                             long num,
                             wxFolderView *fv,
                             wxWindow *parent)
             : wxLayoutWindow(parent)
{
   m_folder = folder;
   m_Profile = NULL;
   Create(fv,parent);
   ShowMessage(folder,num);
   Show(TRUE);
}

MailFolder *
wxMessageView::GetFolder(void)
{
   return m_FolderView ? m_FolderView->GetFolder() : m_folder;
}

/// Tell it a new parent profile - in case folder changed.
void
wxMessageView::SetParentProfile(ProfileBase *profile)
{
   SafeDecRef(m_Profile);

   m_Profile = ProfileBase::CreateProfile("MessageView", profile);

   // We also use this to set all values to be read to speed things up:
   #define GET_COLOUR_FROM_PROFILE(which, name) \
      GetColourByName(&m_ProfileValues.which, \
                      READ_CONFIG(m_Profile, MP_MVIEW_##name), \
                      MP_MVIEW_##name##_D)

   GET_COLOUR_FROM_PROFILE(FgCol, FGCOLOUR);
   GET_COLOUR_FROM_PROFILE(BgCol, BGCOLOUR);
   GET_COLOUR_FROM_PROFILE(UrlCol, URLCOLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderValueCol, HEADER_VALUES_COLOUR);

   #undef GET_COLOUR_FROM_PROFILE

   m_ProfileValues.font = READ_CONFIG(m_Profile,MP_MVIEW_FONT);
   ASSERT(m_ProfileValues.font >= 0 && m_ProfileValues.font <= NUM_FONTS);

   m_ProfileValues.font = wxFonts[m_ProfileValues.font];
   m_ProfileValues.size = READ_CONFIG(m_Profile,MP_MVIEW_FONT_SIZE);
   m_ProfileValues.showHeaders = READ_CONFIG(m_Profile,MP_SHOWHEADERS) != 0;
   m_ProfileValues.rfc822isText = READ_CONFIG(m_Profile,MP_RFC822_IS_TEXT) != 0;
   m_ProfileValues.highlightURLs = READ_CONFIG(m_Profile,MP_HIGHLIGHT_URLS) != 0;
   m_ProfileValues.inlineGFX = READ_CONFIG(m_Profile, MP_INLINE_GFX) != 0;
   m_ProfileValues.browser = READ_CONFIG(m_Profile, MP_BROWSER);
   m_ProfileValues.browserIsNS = READ_CONFIG(m_Profile, MP_BROWSER_ISNS) != 0;
   m_ProfileValues.autocollect =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT);
   m_ProfileValues.autocollectNamed =  READ_CONFIG(m_Profile, MP_AUTOCOLLECT_NAMED);
   m_ProfileValues.autocollectBookName = READ_CONFIG(m_Profile, MP_AUTOCOLLECT_ADB);
#ifdef OS_UNIX
   m_ProfileValues.afmpath = READ_APPCONFIG(MP_AFMPATH);
#endif // Unix
   m_ProfileValues.showFaces = READ_CONFIG(m_Profile, MP_SHOW_XFACES) != 0;
   Clear();
}

void
wxMessageView::Update(void)
{
   int i,n,t;
   char const * cptr;
   String tmp,from,url;
   bool   lastObjectWasIcon = false; // a flag
   ClickableInfo *ci;

   wxLayoutList *llist = GetLayoutList();
   wxLayoutObject *obj = NULL;

   Clear();

   if(! m_mailMessage)  // no message to display
      return;

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
         xface = GLOBAL_NEW XFace();
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

            llist->Insert(headerValue);

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

      // insert text:
      if ( (t == Message::MSG_TYPETEXT) ||
           (t == Message::MSG_TYPEMESSAGE &&
            m_ProfileValues.rfc822isText) )
      {
         cptr = m_mailMessage->GetPartContent(i);
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
         /* This block captures all non-text message parts. They get
            represented by an icon.
            In case of image content, we check whether it might be a
            Fax message. */
      {
         wxBitmap icn;
         if(t == Message::MSG_TYPEIMAGE && m_ProfileValues.inlineGFX)
         {
            wxString filename = wxGetTempFileName("Mtemp");
            MimeSave(i,filename);
            bool ok;
            wxImage img =  wxIconManager::LoadImage(filename, &ok, true);
            wxRemoveFile(filename);
            if(ok)
               icn = img.ConvertToBitmap();
            else
               icn = mApplication->GetIconManager()->
                  GetIconFromMimeType(m_mailMessage->GetPartMimeType(i));
            obj = new wxLayoutObjectIcon(icn);
         }
         else
         {
            icn = mApplication->GetIconManager()->
               GetIconFromMimeType(m_mailMessage->GetPartMimeType(i));
         }
         obj = new wxLayoutObjectIcon(icn);

         ci = new ClickableInfo(i);
         obj->SetUserData(ci); // gets freed by list
         ci->DecRef();
         llist->Insert(obj);

         lastObjectWasIcon = true;
      }
   }
   llist->LineBreak();
   Refresh();
   ResizeScrollbars(true);
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

wxMessageView::~wxMessageView()
{
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

   wxDELETE(m_MimePopup);

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
   wxString filenameOrig;
   (void)m_mailMessage->ExpandParameter
         (
            m_mailMessage->GetDisposition(mimeDisplayPart),
            "FILENAME",
            &filenameOrig
         );

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

#  ifdef OS_WIN
   // get the standard extension for such files - we'll use it below...
   wxString ext;
   if ( fileType != NULL ) {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) ) {
         ext = exts[0u];
      }
   }
#  endif // Win

   /* First, we check for those contents that we handle in M itself: */
   // this we handle internally
   if(mimetype.length() >= strlen("MESSAGE") &&
      mimetype.Left(strlen("MESSAGE")) == "MESSAGE")
   {
      char *filename = wxGetTempFileName("Mtemp");
      if(MimeSave(mimeDisplayPart,filename))
      {
         MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE,
                                                 filename);
         wxMessageViewFrame * f = GLOBAL_NEW
            wxMessageViewFrame(mf, 0, NULL, m_Parent);
         f->SetTitle(mimetype);
         mf->DecRef();
      }
      wxRemoveFile(filename);
      return;
   }

   String
      filename = wxGetTempFileName("Mtemp"),
      filename2 = "";

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
#endif
   String command;
   if ( (fileType == NULL) || !fileType->GetOpenCommand(&command,
                                                        params) )
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
#     ifdef OS_WIN
         // under Windows some programs will do different things depending on
         // the extensions of the input file (case in point: WinZip), so try to
         // choose a correct one
         if ( !ext.IsEmpty() )
         {
            String newFilename;
            newFilename << filename << '.' << ext;
            if ( rename(filename, newFilename) != 0 )
               wxLogSysError(_("Cannot rename temporary file."));
            else
               filename = newFilename;
         }
#     endif // Win

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
      (void)m_mailMessage->ExpandParameter
            (
               m_mailMessage->GetDisposition(mimeDisplayPart),
               "FILENAME",
               &filename
            );

      wxString name, ext;
      wxSplitPath(filename, NULL, &name, &ext);

      name << '.' << ext;
      filename = wxFileSelector(_("Save attachment as:"),
                                NULLstring, name, NULLstring,
                                NULLstring, 0, this);
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
wxMessageView::ShowRawText(MailFolder *folder)
{
   if ( !folder )
      folder = m_folder;

   if ( !folder )
      folder = m_FolderView->GetFolder();

   CHECK( folder, false, "no MailFolder in message view" );

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
   wxArrayInt msgs;
   if( m_uid != -1 )
      msgs.Add(m_uid);

   bool handled = true;
   switch ( id )
   {
      case WXMENU_MSG_REPLY:
         if(m_uid != -1)
            GetFolder()->ReplyMessages(&msgs, GetFrame(this), m_Profile);
         break;
      case WXMENU_MSG_FORWARD:
         if(m_uid != -1)
            GetFolder()->ForwardMessages(&msgs, GetFrame(this), m_Profile);
         break;

      case WXMENU_MSG_SAVE_TO_FOLDER:
         if(m_uid != -1)
            GetFolder()->SaveMessagesToFolder(&msgs, GetFrame(this));
         break;
      case WXMENU_MSG_SAVE_TO_FILE:
         if(m_uid != -1)
            GetFolder()->SaveMessagesToFile(&msgs, GetFrame(this));
         break;

      case WXMENU_MSG_DELETE:
         if(m_uid != -1)
            GetFolder()->DeleteMessages(&msgs);
         break;

      case WXMENU_MSG_UNDELETE:
         if(m_uid != -1)
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

      default:
         handled = false;
   }

   return handled;
}

void
wxMessageView::ShowMessage(MailFolder *folder, long uid)
{
   if ( m_uid == uid )
      return;

   // check the message size
   Message *mailMessage = folder->GetMessage(uid);
   CHECK_RET( mailMessage, "no message with such uid" );

   unsigned long size = 0,
                 maxSize = (unsigned long)READ_CONFIG(m_Profile,
                                                      MP_MAX_MESSAGE_SIZE);

   // we're only interested in the size, not status flags
   (void)mailMessage->GetStatus(&size);
   size /= 1024;  // size is measured in KBytes

   if ( size > maxSize )
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

   m_uid = uid;
   m_mailMessage = mailMessage;

   if(! (m_mailMessage->GetStatus() & MailFolder::MSG_STAT_SEEN))
      folder->SetMessageFlag(m_uid, MailFolder::MSG_STAT_SEEN, true);

   /* FIXME for now it's here, should go somewhere else: */
   if ( m_ProfileValues.autocollect )
   {
      String addr, name;
      addr = m_mailMessage->Address(name, MAT_REPLYTO);

      String folderName = folder->GetName();

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
      wxSetAFMPath(afmpath);
#endif // Win/Unix

   wxPrinter printer(&((wxMApp *)mApplication)->GetPrintDialogData());
   wxLayoutPrintout printout(GetLayoutList(), _("Mahogany: Printout"));
   if ( !printer.Print(this, &printout, TRUE) )
   {
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
   }
}

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
   wxPrintPreview *preview = new wxPrintPreview(
      new wxLayoutPrintout(GetLayoutList()),
      new wxLayoutPrintout(GetLayoutList()),
      &((wxMApp *)mApplication)->GetPrintDialogData()
      );
   if( !preview->Ok() )
   {
      delete preview;
      wxMessageBox(_("There was a problem with showing the preview:\n"
                     "perhaps your current printer is not set correctly?"),
                   _("Previewing"), wxOK);
      return;
   }

   wxPreviewFrame *frame = new wxPreviewFrame(preview, GetFrame(m_Parent),
                                              _("Print Preview"),
                                              wxPoint(100, 100), wxSize(600, 650));
   frame->Centre(wxBOTH);
   frame->Initialize();
   frame->Show(TRUE);
}

bool
wxMessageView::RunProcess(const String& command)
{
   return wxExecute(command, true) == 0;
}

bool
wxMessageView::LaunchProcess(const String& command,
                             const String& errormsg,
                             const String& filename)
{
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

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMessageViewFrame, wxMFrame)
   EVT_SIZE(wxMessageViewFrame::OnSize)
   EVT_MENU(-1, wxMessageViewFrame::OnCommandEvent)
   EVT_TOOL(-1, wxMessageViewFrame::OnCommandEvent)
END_EVENT_TABLE()

wxMessageViewFrame::wxMessageViewFrame(MailFolder *folder,
                                       long num,
                                       wxFolderView *fv,
                                       MWindow  *parent,
                                       const String &name)
{
   m_MessageView = NULL;
   m_ToolBar = NULL;

   wxMFrame::Create(name, parent);

   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   SetMenuBar(m_MenuBar);

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_MESSAGE);
   CreateStatusBar();

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
