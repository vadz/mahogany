/*-*- c++ -*-********************************************************
 * wxMessageView.cc : a wxWindows look at a message                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$        *
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
#endif //USE_PCH

#include "Mdefaults.h"

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
#include <wx/mimetype.h>

#include <ctype.h>  // for isspace

// @@@@ for testing only
#ifndef USE_PCH
   extern "C"
   {
#     include <rfc822.h>
   }
#  include "MessageCC.h"
#endif //USE_PCH

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// go up the window hierarchy until we find the frame
extern wxFrame *GetFrame(wxWindow *win)
{
   // find our frame for the status message
   while ( win && !win->IsKindOf(CLASSINFO(wxFrame)) ) {
      win = win->GetParent();
   }

   // might be NULL!
   return (wxFrame *)win;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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

void
wxMessageView::Create(wxFolderView *fv, wxWindow *parent, const String &iname)
{
   if(initialised)
      return; // ERROR!

   mailMessage = NULL;
   mimeDisplayPart = 0;
   xface = NULL;
   xfaceXpm = NULL;
   m_Parent = parent;
   m_FolderView = fv;
   m_MimePopup = NULL;
   m_uid = -1;

   SetFocus();
   SetMouseTracking();

   SetBackgroundColour( wxColour("White") );

   m_Profile = ProfileBase::CreateProfile(iname, fv ? fv->GetProfile() : NULL);
   initialised = true;
}

wxMessageView::wxMessageView(wxFolderView *fv, wxWindow *parent, const String &iname)
   : wxLayoutWindow(parent)
{
   initialised = false;
   folder = NULL;
   Create(fv,parent,iname);
   Show(TRUE);
}

wxMessageView::wxMessageView(MailFolder *ifolder,
                             long num,
                             wxFolderView *fv,
                             wxWindow *parent,
                             const String &iname)
   : wxLayoutWindow(parent)
{
   initialised = false;
   folder = ifolder;
   Create(fv,parent,iname);
   ShowMessage(folder,num);

   String
      email, name;
   email = mailMessage->Address(name,MAT_FROM);
#if 0 // @@@@ FIXME
   mApplication.GetAdb()->UpdateEntry(email, name);
#endif
   Show(TRUE);
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

   String
      fg = READ_CONFIG(m_Profile,MP_FTEXT_FGCOLOUR),
      bg = READ_CONFIG(m_Profile,MP_FTEXT_BGCOLOUR);

   Clear(READ_CONFIG(m_Profile,MP_FTEXT_FONT),
         READ_CONFIG(m_Profile,MP_FTEXT_SIZE),
         READ_CONFIG(m_Profile,MP_FTEXT_STYLE),
         READ_CONFIG(m_Profile,MP_FTEXT_WEIGHT),
         0,
         fg.c_str(),bg.c_str());

   // if wanted, display all header lines
   if(READ_CONFIG(m_Profile,MP_SHOWHEADERS))
   {
      String tmp = mailMessage->GetHeader();
      wxLayoutImportText(llist,tmp);
      llist.LineBreak();
   }

#ifdef HAVE_XFACES
   // need real XPM support in windows
#ifndef OS_WIN
   if(m_Profile->readEntry(MP_SHOW_XFACES, MP_SHOW_XFACES_D))
   {
      mailMessage->GetHeaderLine("X-Face", tmp);
      if(tmp.length() > 2)   //\r\n
      {
         xface = GLOBAL_NEW XFace();
         tmp = tmp.c_str()+strlen("X-Face:");
         xface->CreateFromXFace(tmp.c_str());
         if(xface->CreateXpm(&xfaceXpm))
         {
            llist.Insert(new wxLayoutObjectIcon(new wxIcon(xfaceXpm,-1,-1)));
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
            READ_CONFIG(m_Profile, MP_RFC822_IS_TEXT)) )
      {
         cptr = mailMessage->GetPartContent(i);
         if(cptr == NULL)
            continue; // error ?
         llist.LineBreak();
         if( READ_CONFIG(m_Profile, MP_HIGHLIGHT_URLS) )
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
         wxIcon icn;
         if(t == Message::MSG_TYPEIMAGE && READ_CONFIG(m_Profile, MP_INLINE_GFX))
         {
            char *filename = wxGetTempFileName("Mtemp");
            MimeSave(i,filename);
            if(icn.LoadFile(filename,0))
               obj = new wxLayoutObjectIcon(icn);
            else
            {
               icn = mApplication->GetIconManager()->
                        GetIconFromMimeType(mailMessage->GetPartMimeType(i));
            }
            wxRemoveFile(filename);
         }
         else {
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
   UpdateScrollbars();
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
   if( !initialised )
      return;

   GLOBAL_DELETE mailMessage;
   if(xface)
      delete xface;
   if(xfaceXpm)
      delete [] xfaceXpm;

   wxDELETE(m_MimePopup);

   m_Profile->DecRef();
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
      message << _(" lines\n");
   else
      message << _(" bytes\n");

   // debug output with all parameters
   const MessageParameterList &plist = mailMessage->GetParameters(mimeDisplayPart);
   MessageParameterList::iterator plist_it;
   if(plist.size() > 0)
   {
      message += "\nParameters:\n";
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
      message << "\nDisposition: " << disposition << '\n';
   if(dlist.size() > 0)
   {
      message += "\nDisposition Parameters:\n";
      for(plist_it = dlist.begin(); plist_it != dlist.end();
          plist_it++)
         message << (*plist_it)->name << ": "
                 << (*plist_it)->value << '\n';
   }

   wxMessageBox(message.c_str(), _("MIME information"),
                wxOK|wxCENTRE|wxICON_INFORMATION, this);
}

// open (execute) a message attachment
void
wxMessageView::MimeHandle(int mimeDisplayPart)
{
   String mimetype = mailMessage->GetPartMimeType(mimeDisplayPart);
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
   wxFileType *fileType = mimeManager.GetFileTypeFromMimeType(mimetype);

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
      if(mimetype == "MESSAGE/RFC822")
      {
         char *filename = wxGetTempFileName("Mtemp");
         MimeSave(mimeDisplayPart,filename);
         MailFolder *mf = MailFolder::OpenFolder(MailFolder::MF_PROFILE,
                                                 filename);
         (void) GLOBAL_NEW wxMessageViewFrame(mf, 1, m_FolderView, m_Parent);
         mf->DecRef();
         wxRemoveFile(filename);
      }
      else
      {
         wxLogWarning(_("Don't know how to handle data of type '%s'."),
                      mimetype.c_str());
      }
   }
   else
   {
      // have a command to open this kind of data - so do it
      FILE *out = fopen(filename,"wb");
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

      if ( !wxExecute(command, TRUE) )
      {
         wxLogError(_("Error opening attachment: command '%s' failed."),
                    command.c_str());
      }

      // clean up
      if ( remove(filename) != 0 )
      {
         wxLogSysError(_("Can't delete temporary file '%s'"), filename.c_str());
      }
   }
}

void
wxMessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
{
   String message;
   String filename;

   if(! ifilename)
   {
      mailMessage->ExpandParameter(
         mailMessage->GetDisposition(mimeDisplayPart),
         "FILENAME", &filename);
      message = _("Please choose a filename to save as:");
      filename = MDialog_FileRequester(message, m_Parent,
                                       NULLstring, filename,
                                       NULLstring, NULLstring, true,
                                       m_FolderView ? m_FolderView->GetProfile() :
                                       NULL);
      if ( strutil_isempty(filename) ) {
         // cancelled
         return;
      }
   }
   else
      filename = ifilename;

   if(filename)
   {
      unsigned long len;
      char const *content = mailMessage->GetPartContent(mimeDisplayPart,&len);
      if(! content)
      {
         ERRORMESSAGE((_("Cannot get message content.")));
         return;
      }
      FILE *out = fopen(filename,"wb");
      if(out)
      {
         if(fwrite(content,1,len,out) != len)
            ERRORMESSAGE((_("Cannot write file.")));
         else if(! ifilename) // only display in interactive mode
         {
            wxLogStatus(GetFrame(this), _("Wrote %lu bytes to file '%s'"),
                        len, filename.c_str());
         }
         fclose(out);
      }
   }
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
               case WXMENU_LAYOUT_RCLICK:
                  // show the menu
                  if ( m_MimePopup == NULL )
                  {
                     // create the pop up menu now if not done yet
                     m_MimePopup = new MimePopup(this, ci->GetPart());
                  }

                  PopupMenu(m_MimePopup, m_ClickPosition.x, m_ClickPosition.y);
                  break;

               case WXMENU_LAYOUT_LCLICK:
                  // for now, do the same thing as double click: perhaps the
                  // left button behaviour should be configurable?

               case WXMENU_LAYOUT_DBLCLICK:
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

            bool bOk;
            String cmd = READ_CONFIG(m_Profile, MP_BROWSER);
            if ( cmd.IsEmpty() ) {
#                 ifdef OS_WIN
                  bOk = (int)ShellExecute(NULL, "open", ci->GetUrl(),
                                          NULL, NULL, SW_SHOWNORMAL ) > 32;
                  if ( !bOk ) {
                     wxLogSysError(_("Can't open URL '%s'"),
                                   ci->GetUrl().c_str());
                  }
#                 else  // Unix
                  // propose to choose program for opening URLs
                  if (
                       MDialog_YesNoDialog
                       (
                        _("No command configured to view URLs.\n"
                          "Would you like to choose one now?"),
                        frame
                       )
                     )
                  {
                     ShowOptionsDialog();
                     cmd = READ_CONFIG(m_Profile, MP_BROWSER);
                  }

                  if ( cmd.IsEmpty() )
                  {
                     wxLogError(_("No command configured to view URLs."));
                     bOk = FALSE;
                  }
#                 endif
            }
            else {
               // not empty, user provided his script - use it
               cmd << ' ' << ci->GetUrl();
               bOk = wxExecute(Str(cmd)) != 0;
            }

            wxEndBusyCursor();

            if ( bOk ) {
               wxLogStatus(frame, _("Opening URL '%s'... done."),
                           ci->GetUrl().c_str());
            }
            else {
               wxLogStatus(frame, _("Opening URL '%s' failed."),
                           ci->GetUrl().c_str());
            }
         }
         break;

         default:
            FAIL_MSG("unknown embedded object type");
      }
   }
}


void
wxMessageView::OnMenuCommand(int id)
{
   wxArrayInt msgs;
   msgs.Insert(0,m_uid);

   switch(id)
   {
   case WXMENU_MSG_PRINT:
      Print();
      break;
   case WXMENU_MSG_REPLY:
      // passed to folderview:
      if(m_FolderView && m_uid != -1)
         m_FolderView->ReplyMessages(msgs);
      break;
   case WXMENU_MSG_FORWARD:
      if(m_FolderView && m_uid != -1)
         m_FolderView->ForwardMessages(msgs);
      break;
   case WXMENU_MSG_SAVE_TO_FOLDER:
      if(m_FolderView && m_uid != -1)
         m_FolderView->SaveMessagesToFolder(msgs);
      break;
   case WXMENU_MSG_SAVE_TO_FILE:
      if(m_FolderView && m_uid != -1)
         m_FolderView->SaveMessagesToFile(msgs);
      break;
   case WXMENU_MSG_DELETE:
      if(m_FolderView && m_uid != -1)
         m_FolderView->DeleteMessages(msgs);
      break;

   }
}

void
wxMessageView::ShowMessage(MailFolder *folder, long num)
{
   if(initialised) GLOBAL_DELETE mailMessage;

   m_uid = num;
   mailMessage = folder->GetMessage(num);
   if(mailMessage->IsInitialised())
      initialised = true;
   else
   {
      initialised = false;
      GLOBAL_DELETE mailMessage;
      return;
   }
   Update();
}


void
wxMessageView::Print(void)
{
#ifdef  OS_UNIX
   // set AFM path (recursive!)
   PathFinder pf(READ_APPCONFIG(MC_AFMPATH), true);
   pf.AddPaths(mApplication->GetGlobalDir(), true);
   pf.AddPaths(mApplication->GetLocalDir(), true);

   String afmpath = pf.FindDirFile("Cour.afm");
   wxSetAFMPath((char *) afmpath.c_str());
#endif // Unix

   wxLayoutWindow::Print();
}


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

   Show(true);
   m_MessageView = new wxMessageView(folder, num, fv, this);

   wxSizeEvent se; // unused
   OnSize(se);
}

void
wxMessageViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   switch ( id )
   {
      case WXMENU_MSG_REPLY:
      case WXMENU_MSG_FORWARD:
      case WXMENU_MSG_PRINT:
         m_MessageView->OnMenuCommand(id);
         break;

      case WXMENU_LAYOUT_LCLICK:
      case WXMENU_LAYOUT_RCLICK:
      case WXMENU_LAYOUT_DBLCLICK:
         m_MessageView->OnMouseEvent(event);
         break;

      default:
         wxMFrame::OnCommandEvent(event);
   }
}

void
wxMessageViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x, y;
   GetClientSize( &x, &y );
   if(m_MessageView)
      m_MessageView->SetSize(0,0,x,y);
};
