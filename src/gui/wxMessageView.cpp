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

#include "adb/AdbManager.h"
#include "adb/AdbBook.h"
#include "gui/wxComposeView.h"

#include "sysutil.h"

#include <ctype.h>  // for isspace
#include <time.h>   // for time stamping autocollected addresses

#ifdef OS_UNIX
   #include <sys/stat.h>
#endif

// @@@@ for testing only
#ifndef USE_PCH
//   extern "C"
//   {
//#     include <rfc822.h>
//   }
//#  include "MessageCC.h"
#endif //USE_PCH

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
class ClickableInfo : public wxLayoutObjectBase::UserData
{
public:
   enum Type
   {
      CI_ICON,
      CI_URL
   };

   ClickableInfo(const String& url) : m_url(url) { m_type = CI_URL; }
   ClickableInfo(int id) { m_id = id; m_type = CI_ICON; }

   // accessors
   Type          GetType() const { return m_type; }
   const String& GetUrl()  const { return m_url;  }
   int           GetPart() const { return m_id;   }

private:
   Type   m_type;

   int    m_id;
   String m_url;
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
class MailMessageParamaters : public wxFileType::MessageParameters
{
public:
   MailMessageParamaters(const wxString& filename,
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
MailMessageParamaters::GetParamValue(const wxString& name) const
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
   mailMessage = NULL;
   mimeDisplayPart = 0;
   xface = NULL;
   xfaceXpm = NULL;
   m_Parent = parent;
   m_FolderView = fv;
   m_MimePopup = NULL;

   SetFocus();
   SetMouseTracking();

   SetBackgroundColour( wxColour("White") );

   
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

/// Tell it a new parent profile - in case folder changed.
void
wxMessageView::SetParentProfile(ProfileBase *profile)
{
   if(m_Profile) m_Profile->DecRef();
   m_Profile = ProfileBase::CreateProfile("MessageView", profile);

   // We also use this to set all values to be read to speed things up:
   m_ProfileValues.fg = READ_CONFIG(m_Profile,MP_FTEXT_FGCOLOUR);
   m_ProfileValues.bg = READ_CONFIG(m_Profile,MP_FTEXT_BGCOLOUR);

   m_ProfileValues.font = READ_CONFIG(m_Profile,MP_FTEXT_FONT);
   m_ProfileValues.size = READ_CONFIG(m_Profile,MP_FTEXT_SIZE);
   m_ProfileValues.style = READ_CONFIG(m_Profile,MP_FTEXT_STYLE);
   m_ProfileValues.weight = READ_CONFIG(m_Profile,MP_FTEXT_WEIGHT);

   m_ProfileValues.showHeaders = READ_CONFIG(m_Profile,MP_SHOWHEADERS) != 0;
   m_ProfileValues.rfc822isText = READ_CONFIG(m_Profile,MP_RFC822_IS_TEXT) != 0;
   m_ProfileValues.highlightURLs = READ_CONFIG(m_Profile,MP_HIGHLIGHT_URLS) != 0;
   m_ProfileValues.inlineGFX = READ_CONFIG(m_Profile, MP_INLINE_GFX) != 0;
   m_ProfileValues.browser = READ_CONFIG(m_Profile, MP_BROWSER);
   m_ProfileValues.browserIsNS = READ_CONFIG(m_Profile, MP_BROWSER_ISNS) != 0;
   m_ProfileValues.autocollect =  READ_CONFIG(profile, MP_AUTOCOLLECT);
   m_ProfileValues.autocollectNamed =  READ_CONFIG(profile, MP_AUTOCOLLECT_NAMED);
   m_ProfileValues.autoCollectBookName = READ_CONFIG(profile, MP_AUTOCOLLECT_ADB);
#ifdef OS_UNIX
   m_ProfileValues.afmpath = READ_APPCONFIG(MP_AFMPATH);
#endif // Unix
   m_ProfileValues.showFaces = READ_CONFIG(profile, MP_SHOW_XFACES) != 0;
}
   
void
wxMessageView::Update(void)
{
   int i,n,t;
   char const * cptr;
   String tmp,from,url;
   bool   lastObjectWasIcon = false; // a flag

   ClickableInfo *ci;

   wxLayoutList &llist = GetLayoutList();
   wxLayoutObjectBase *obj = NULL;

   llist.SetEditable(true);

   Clear();
   // if wanted, display all header lines
   if(m_ProfileValues.showHeaders)
   {
      String tmp = mailMessage->GetHeader();
      wxLayoutImportText(llist,tmp);
      llist.LineBreak();
   }

#ifdef HAVE_XFACES
   // need real XPM support in windows
#ifndef OS_WIN
   if(m_ProfileValues.showFaces)
   {
      mailMessage->GetHeaderLine("X-Face", tmp);
      if(tmp.length() > 2)   //\r\n
      {
         xface = GLOBAL_NEW XFace();
         tmp = tmp.c_str()+strlen("X-Face:");
         xface->CreateFromXFace(tmp.c_str());
         if(xface->CreateXpm(&xfaceXpm))
         {
            llist.Insert(new wxLayoutObjectIcon(new wxBitmap(xfaceXpm)));
            llist.LineBreak();
         }
      }
   }
#endif
#endif
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("From: "));
   llist.SetFontWeight(wxNORMAL);
   from = mailMessage->Address(tmp,MAT_FROM);
   if(tmp.length() > 0)
      from = tmp + String(" <") + from + '>';
   llist.Insert(from);
   llist.LineBreak();
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("Subject: "));
   llist.SetFontWeight(wxNORMAL);
   llist.Insert(mailMessage->Subject());
   llist.LineBreak();
   llist.SetFontWeight(wxBOLD);
   llist.Insert(_("Date: "));
   llist.SetFontWeight(wxNORMAL);
   llist.Insert(mailMessage->Date());
   llist.LineBreak();
   llist.LineBreak();

// iterate over all parts
   n = mailMessage->CountParts();
   for(i = 0; i < n; i++)
   {
      t = mailMessage->GetPartType(i);
      if(mailMessage->GetPartSize(i) == 0)
         continue; // ignore empty parts
#if 0
      // debug output with all parameters
      const MessageParameterList &plist = mailMessage->GetParameters(i);
      MessageParameterList::iterator plist_it;
      VAR(i);
      for(plist_it = plist.begin(); plist_it != plist.end();
          plist_it++)
      {
         VAR( (*plist_it)->name);
         VAR( (*plist_it)->value);
      }
      String disposition;
      const MessageParameterList &dlist =
         mailMessage->GetDisposition(i,&disposition);
      VAR(disposition);
      for(plist_it = dlist.begin(); plist_it != dlist.end();
          plist_it++)
      {
         VAR( (*plist_it)->name);
         VAR( (*plist_it)->value);
      }
#endif

      // insert text:
      if ( (t == Message::MSG_TYPETEXT) ||
           (t == Message::MSG_TYPEMESSAGE &&
            m_ProfileValues.rfc822isText) )
      {
         cptr = mailMessage->GetPartContent(i);
         if(cptr == NULL)
            continue; // error ?
         llist.LineBreak();
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

                  llist.SetFontColour("BLUE");  // @@PERS
                  llist.Insert(obj);
                  llist.SetFontColour("BLACK"); // @@PERS
               }
            }
            while( !strutil_isempty(tmp) );

            lastObjectWasIcon = false;
         }
      }
      else // insert an icon
      {
         wxBitmap icn;
         if(t == Message::MSG_TYPEIMAGE && m_ProfileValues.inlineGFX)
         {
            char *filename = wxGetTempFileName("Mtemp");
            MimeSave(i,filename);
            bool ok;
            wxImage img =  wxIconManager::LoadImage(filename, &ok);
            if(ok)
               icn = img.ConvertToBitmap();
            else
               icn = mApplication->GetIconManager()->
                  GetIconFromMimeType(mailMessage->GetPartMimeType(i));
            obj = new wxLayoutObjectIcon(icn);
#if 0   
            char **xpmarray = NULL;
            xpmarray = wxIconManager::LoadImageXpm(filename);
            //if(icn.LoadFile(filename,0))
            if(xpmarray)
            {
               icn = wxBitmap(xpmarray);
               obj = new wxLayoutObjectIcon(icn);
               wxIconManager::FreeImage(xpmarray);
            }
            wxRemoveFile(filename);
#endif
         }
         else
         {
            icn = mApplication->GetIconManager()->
               GetIconFromMimeType(mailMessage->GetPartMimeType(i));
         }
         obj = new wxLayoutObjectIcon(icn);

         ci = new ClickableInfo(i);
         obj->SetUserData(ci); // gets freed by list
         llist.Insert(obj);

         lastObjectWasIcon = true;
      }
   }

   llist.LineBreak();
   llist.SetEditable(false);
   Refresh();
   UpdateScrollbars(true);
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

   if(mailMessage) mailMessage->DecRef();
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
           << mailMessage->GetPartMimeType(mimeDisplayPart)
           << '\n';

   String tmp = mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message << '\n' << _("Description: ") << tmp << '\n';

   message << _("Size: ")
           << strutil_ltoa(mailMessage->GetPartSize(mimeDisplayPart));

   Message::ContentType type = mailMessage->GetPartType(mimeDisplayPart);
   if(type == Message::MSG_TYPEMESSAGE || type == Message::MSG_TYPETEXT)
      message << _(" lines");
   else
      message << _(" bytes");
   message << '\n';

   // debug output with all parameters
   const MessageParameterList &plist = mailMessage->GetParameters(mimeDisplayPart);
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
   const MessageParameterList &dlist = mailMessage->GetDisposition(mimeDisplayPart,&disposition);
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
   (void)mailMessage->ExpandParameter
         (
            mailMessage->GetDisposition(mimeDisplayPart),
            "FILENAME",
            &filenameOrig
         );

   String mimetype = mailMessage->GetPartMimeType(mimeDisplayPart);
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

   // this we handle internally
   //if(mimetype == "MESSAGE/RFC822")
   if(mimetype.length() >= strlen("MESSAGE") &&
      mimetype.Left(strlen("MESSAGE")) == "MESSAGE")
   {
      char *filename = wxGetTempFileName("Mtemp");
      if(MimeSave(mimeDisplayPart,filename))
      {
         MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE,
                                                 filename);
         wxMessageViewFrame * f = GLOBAL_NEW
            wxMessageViewFrame(mf, 1, m_FolderView, m_Parent);
         f->SetTitle("M : " + mimetype);
         mf->DecRef();
      }
      wxRemoveFile(filename);
      return;
   }
   // need a filename for GetOpenCommand()
   String filename = tmpnam(NULL);
   MailMessageParamaters params(filename, mimetype,
                                mailMessage, mimeDisplayPart);
   String command;
   if ( (fileType == NULL) || !fileType->GetOpenCommand(&command, params) ) {
      // unknown MIME type, ask the user for the command to use
      String prompt;
      prompt.Printf(_("Please enter the command to handle '%s' data:"),
                    mimetype.c_str());
      if ( !MInputBox(&command, _("Unknown MIME type"), prompt,
                      this, "MimeHandler") ) {
         // cancelled by user
         return;
      }

      if ( !command.IsEmpty() )
         wxLogWarning(_("Don't know how to handle data of type '%s'."),
                      mimetype.c_str());
      else
      {
         // the command must contain exactly one '%s' format specificator!
         String specs = strutil_extract_formatspec(command);
         if ( specs.IsEmpty() ) {
            // at least the filename should be there!
            command += " %s";
         }

         // do expand it
         command = wxFileType::ExpandCommand(command, params);

         // TODO save this command to mailcap!
      }
      //else: empty command means try to handle it internally
   }

   delete fileType;  // may be NULL, ok

   if ( command.IsEmpty() )
   {
      // no command - try to handle it internally
   }
   else
   {
      // have a command to open this kind of data - so do it
      FILE *out = fopen(filename, "wb");
      if( !out )
      {
         wxLogSysError(_("Can't open temporary file"));
         return;
      }

      unsigned long len;
      const void *content = mailMessage->GetPartContent(mimeDisplayPart, &len);
      bool ok = fwrite(content, 1, len, out) == len;
      fclose(out);

      if ( !ok )
      {
         wxLogSysError(_("Can't write data to temporary file."));
         return;
      }

#     ifdef OS_WIN
      // under Windows some programs will do different things depending on
      // the extensions of the input file (case in point: WinZip), so try to
      // choose a correct one
      if ( !ext.IsEmpty() ) {
         String newFilename;
         newFilename << filename << '.' << ext;
         if ( rename(filename, newFilename) != 0 ) {
            wxLogSysError(_("Can't rename temporary file."));
         }
         else {
            filename = newFilename;
         }
      }
#     endif // Win

      wxString errmsg;
      errmsg.Printf(_("Error opening attachment: command '%s' failed"),
                    command.c_str());
      (void)LaunchProcess(command, errmsg, filename);
   }
}

bool
wxMessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
{
   String filename;

   if ( strutil_isempty(ifilename) )
   {
      (void)mailMessage->ExpandParameter
            (
               mailMessage->GetDisposition(mimeDisplayPart),
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
   char const *content = mailMessage->GetPartContent(mimeDisplayPart, &len);
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

   wxLogError(_("Couldn't save the attachment."));

   return false;
}

void
wxMessageView::OnMouseEvent(wxCommandEvent &event)
{
   ClickableInfo *ci;

   wxLayoutObjectBase *obj;
   obj = (wxLayoutObjectBase *)event.GetClientData();
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
               wxComposeView *cv = new wxComposeView("Reply",frame,m_Profile);
               cv->SetTo(ci->GetUrl().Right(ci->GetUrl().Length()-7));
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
                  wxLogSysError(_("Can't open URL '%s'"),
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

                     bOk = RunProcess(command);
                  }
               }
#endif
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
   mailMessage->WriteToString(text, true);
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
   msgs.Add(m_seqno-1);  // index in listctrl is seqno in folder -1
   bool handled = true;
   switch(id)
   {
   case WXMENU_MSG_PRINT:
      Print();
      break;
   case WXMENU_MSG_PRINT_PREVIEW:
      PrintPreview();
      break;
   case WXMENU_MSG_REPLY:
      // passed to folderview:
      if(m_FolderView && m_seqno != -1)
         m_FolderView->ReplyMessages(msgs);
      break;
   case WXMENU_MSG_FORWARD:
      if(m_FolderView && m_seqno != -1)
         m_FolderView->ForwardMessages(msgs);
      break;

   case WXMENU_MSG_SAVE_TO_FOLDER:
      if(m_FolderView && m_seqno != -1)
         m_FolderView->SaveMessagesToFolder(msgs);
      break;
   case WXMENU_MSG_SAVE_TO_FILE:
      if(m_FolderView && m_seqno != -1)
         m_FolderView->SaveMessagesToFile(msgs);
      break;
   case WXMENU_MSG_DELETE:
      if(m_FolderView && m_seqno != -1)
         m_FolderView->DeleteMessages(msgs);
      break;
   case WXMENU_MSG_UNDELETE:
     if(m_FolderView && m_seqno != -1)
         m_FolderView->UnDeleteMessages(msgs);
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
         if(m_ProfileValues.showHeaders)
            m_ProfileValues.showHeaders = false;
         else
            m_ProfileValues.showHeaders = true;
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
wxMessageView::ShowMessage(MailFolder *folder, long num)
{
   // don't redisplay the already shown message
   if ( m_seqno == num )
      return;
   if(mailMessage) mailMessage->DecRef();
   mailMessage = folder->GetMessage(num);
   m_seqno = num;

   /* FIXME for now it's here, should go somewhere else: */
   if ( m_ProfileValues.autocollect )
   {
      String name;
      String email = mailMessage->Address(name, MAT_REPLYTO);

      // we need an address and a name
      bool hasEmailAndName = true;
      if ( email.IsEmpty() )
      {
         // won't do anything without email address
         hasEmailAndName = false;
      }
      else if ( name.IsEmpty() )
      {
         // if there is no name and we want to autocollect such addresses
         // (it's a global option), take the first part of the e-mail address
         // for the name
         if ( ! m_ProfileValues.autocollectNamed )
         {
            // will return the whole string if '@' not found - ok
            name = email.BeforeFirst('@');
         }
         else
            hasEmailAndName = false;
      }

      if ( hasEmailAndName )
      {
         AdbManager_obj manager;
         CHECK_RET( manager, "can't get AdbManager" );

         // load all address books mentioned in the profile
         manager->LoadAll();

         // and also explicitly load autocollect book - it might have not been
         // loaded by LoadAll(), yet we want to search in it too
         // won't recreate it if it already exists
         AdbBook *autocollectbook = manager->CreateBook(m_ProfileValues.autoCollectBookName);

         if ( !autocollectbook )
         {
            wxLogError(_("Failed to create the address book '%s' "
                         "for autocollected e-mail addresses."),
                       m_ProfileValues.autoCollectBookName.c_str());

            // TODO ask the user if he wants to disable autocollec?
         }

         ArrayAdbEntries matches;
         if( !AdbLookup(matches, email, AdbLookup_EMail, AdbLookup_Match) )
         {
            if ( AdbLookup(matches, name, AdbLookup_FullName, AdbLookup_Match) )
            {
               // found: add another e-mail (it can't already have it, otherwise
               // our previous search would have succeeded)
               AdbEntry *entry = matches[0];
               entry->AddEMail(email);

               wxString name;
               entry->GetField(AdbField_NickName, &name);
               wxLogStatus(GetFrame(this),
                           _("Auto collected e-mail address '%s' "
                             "(added to the entry '%s')."),
                           email.c_str(), name.c_str());
            }
            else // no such address, no such name - create a new entry
            {
               // the value is either 1 or 2, if 1 we have to ask
               bool askUser = m_ProfileValues.autocollect == 1;
               if (
                  !askUser ||
                  MDialog_YesNoDialog(_("Add new e-mail entry to database?"),
                                      this)
                  )
               {
                  // filter invalid characters in the record name
                  wxString entryname;
                  for ( const char *pc = name; *pc != '\0'; pc++ )
                  {
                     entryname << (isspace(*pc) ? '_' : *pc);
                  }

                  AdbEntry *entry = autocollectbook->CreateEntry(entryname);

                  if ( !entry )
                  {
                     wxLogError(_("Couldn't create an entry in the address "
                                  "book '%s' for autocollected address."),
                                m_ProfileValues.autoCollectBookName.c_str());

                     // TODO ask the user if he wants to disable autocollec?

                  }
                  else
                  {
                     entry->SetField(AdbField_NickName, entryname);
                     entry->SetField(AdbField_FullName, entryname);
                     entry->SetField(AdbField_EMail, email);

                     wxString comment;
                     {
                        // get the timestamp
                        time_t timeNow;
                        time(&timeNow);
                        struct tm *ptmNow = localtime(&timeNow);

                        char szBuf[128];
                        strftime(szBuf, WXSIZEOF(szBuf),
                                 "%d/%b/%Y %H:%M:%S", ptmNow);

                        comment.Printf(_("This entry was automatically "
                                         "added by M on %s."), szBuf);
                     }

                     entry->SetField(AdbField_Comments, comment);
                     entry->DecRef();

                     wxLogStatus(GetFrame(this),
                                 _("Auto collected e-mail address '%s' "
                                   "(created new entry '%s')."),
                                 email.c_str(), entryname.c_str());
                  }
               }
            }
         }
         else
         {
            // there is already an entry which has this e-mail, don't create
            // another one (even if the name is different it's more than
            // likely that it's the same person)

            // clear the status line
            wxLogStatus(GetFrame(this), "");
         }

         // release the found items (if any)
         size_t count = matches.Count();
         for ( size_t n = 0; n < count; n++ )
         {
            matches[n]->DecRef();
         }

         // release the book
         autocollectbook->DecRef();
      }
      else
      {
         // it's not very intrusive and the user might wonder why the address
         // wasn't autocollected otherwise
         wxLogStatus(GetFrame(this),
                     _("'%s': the name is missing, address was not "
                       "autocollected."), email.c_str());
      }
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

   //    set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(m_ProfileValues.afmpath, false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
      wxSetAFMPath(afmpath);
#endif // Win/Unix
   wxPrinter printer;
   wxLayoutPrintout printout(GetLayoutList(),_("M: Printout"));
   if (! printer.Print(this, &printout, TRUE))
      wxMessageBox(
         _("There was a problem with printing the message:\n"
           "perhaps your current printer is not set up correctly?"),
         _("Printing"), wxOK);
}

void
wxMessageView::PrintPreview(void)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   //    set AFM path (recursive!)
   PathFinder pf(m_ProfileValues.afmpath, true);
   pf.AddPaths(mApplication->GetGlobalDir(), true);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   String afmpath = pf.FindDirFile("Cour.afm");
   wxSetAFMPath((char *) afmpath.c_str());
#endif // in/Unix

   // Pass two printout objects: for preview, and possible printing.
   wxPrintPreview *preview = new wxPrintPreview(new wxLayoutPrintout(GetLayoutList()),
                                                new wxLayoutPrintout(GetLayoutList()),
                                               & ((wxMApp *)mApplication)->GetPrintData());
   if(!preview->Ok())
   {
      delete preview;
      wxMessageBox(
         _("There was a problem with showing the preview:\n"
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

   MTempFileName *tempfile = info->GetTempFile();
   if ( tempfile )
   {
      // tell it that it's ok to delete the temp file
      tempfile->Ok();
   }

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
