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
#include "Mcommon.h"
#include "guidef.h"
#include "Message.h"

#ifndef USE_PCH
#  include "strutil.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"
#endif //USE_PCH

#include "Mdefaults.h"

#include "MApplication.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "MDialogs.h"

#include "MessageView.h"

#include "XFace.h"

#include "gui/wxMApp.h"
#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"
#include "gui/wxMessageView.h"
#include   "gui/wxFolderView.h"
#include   "gui/wxllist.h"
#include   "gui/wxlwindow.h"
#include   "gui/wxlparser.h"
#include <wx/dynarray.h>

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
static wxFrame *GetFrame(wxWindow *win)
{
   // find our frame for the status message
   while ( win && !win->IsKindOf(CLASSINFO(wxFrame)) ) {
      win = win->GetParent();
   }

   // might be NULL!
   return (wxFrame *)win;
}

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

// for clickable objects
struct ClickableInfo
{
   enum { CI_ICON, CI_URL } type;
   int id;
   String url;
};

#if !USE_WXWINDOWS2
static void popup_callback(wxMenu& menu, wxCommandEvent& ev);
#endif // wxWin 1/2

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------
class myPopup : public wxMenu
{
protected:
   wxWindow *popup_parent;
   int      menu_offset;  // all selections get added to this and passed
   // to popup_parent->OnMenuCommand(offset+entry)

#if !USE_WXWINDOWS2
   friend void popup_callback(wxMenu& menu, wxCommandEvent& ev);
#endif

public:
   myPopup(const char *name, wxWindow *frame, int offset)
#if USE_WXWINDOWS2
      : wxMenu(name)
#else  // wxWin 1
         : wxMenu((char *)name, (wxFunction) &popup_callback)
#endif // wxWin1/2
      {
         popup_parent = frame;
         menu_offset  = offset;
      }
};

#if USE_WXWINDOWS2
// @@@@ OnMenuCommand?
#else
static void popup_callback(wxMenu& menu, wxCommandEvent& ev)
{
   myPopup *popup = (myPopup *)&menu;
   popup->popup_parent->OnMenuCommand(popup->menu_offset + ev.GetSelection());
}
#endif


class MimePopup : public wxMenu
{
public:
   MimePopup()
      : wxMenu(_("MIME Menu"))
      {
         Append(WXMENU_MIME_INFO,_("&Info"));
         Append(WXMENU_MIME_HANDLE,_("&Open"));
         Append(WXMENU_MIME_SAVE,_("&Save"));
      }
};


#define   BUTTON_HEIGHT 24
#define   BUTTON_WIDTH  60

class MimeDialog : public wxDialog
{
public:
   MimeDialog(wxMessageView *parent, wxPoint const & pos, int partno)
      : wxDialog(parent, -1, _("MIME Menu"), pos,
                 wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
      {
         m_PartNo = partno;
         m_MessageView = parent;
         (void) new wxButton(this, WXMENU_MIME_INFO, _("Info"),wxPoint(0,0),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         (void) new wxButton(this, WXMENU_MIME_HANDLE, _("Open"),wxPoint(0,BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         (void) new wxButton(this, WXMENU_MIME_SAVE, _("Save"), wxPoint(0,2*BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         (void) new wxButton(this, WXMENU_MIME_DISMISS, _("Dismiss"), wxPoint(0,3*BUTTON_HEIGHT),wxSize(BUTTON_WIDTH,BUTTON_HEIGHT));
         Fit();
      }
   void OnCommandEvent(wxCommandEvent &event);
   DECLARE_EVENT_TABLE()
      private:
   wxMessageView *m_MessageView;
   int m_PartNo;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MimeDialog, wxDialog)
   EVT_BUTTON(-1,    MimeDialog::OnCommandEvent)
   END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================
void
MimeDialog::OnCommandEvent(wxCommandEvent &event)
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
   case WXMENU_MIME_DISMISS:
      Show(false);
      break;
   }
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
  
  // wxLayoutWindow:
  SetEventId(WXMENU_LAYOUT_CLICK);
  SetFocus();

#ifndef __WXMSW__
  SetBackgroundColour( wxColour("White") ); 
#endif

  m_Profile = ProfileBase::CreateProfile(iname, folder ? folder->GetProfile() : NULL);
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
      if(t == TYPETEXT
         || (t == TYPEMESSAGE && READ_CONFIG(m_Profile, MP_RFC822_IS_TEXT)))
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
                  ci = new ClickableInfo;
                  ci->type = ClickableInfo::CI_URL;
                  ci->url = url;
                  obj = new wxLayoutObjectText(url);
                  obj->SetUserData(ci);
                  llist.SetFontColour("BLUE");  // @@PERS
                  llist.Insert(obj);
                  llist.SetFontColour("BLACK"); // @@PERS
               }
            }while(! strutil_isempty(tmp));
            lastObjectWasIcon = false;
         }
      }
      else // insert an icon
      {
         wxIcon icn;
         if(t == TYPEIMAGE
            && m_Profile->readEntry(MP_INLINE_GFX,MP_INLINE_GFX_D))
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

         ci = new ClickableInfo;
         ci->type = ClickableInfo::CI_ICON;
         ci->id = i;
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

void
wxMessageView::HighLightURLs(const char *input, String &out)
{
   const char *cptr = input;

   while(*cptr)
   {

      if(strncmp(cptr, "http:", 5) == 0 || strncmp(cptr, "ftp:", 4) == 0)
      {
         const char *cptr2 = cptr;
         out += " <a href=\"";
         out += "\"";
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
   if(m_MimePopup)
      delete m_MimePopup;
   m_Profile->DecRef();
}

void
wxMessageView::MimeInfo(int mimeDisplayPart)
{
   String message, tmp;
   int type;

   message =
      String(  _("MIME type: "))
      + mailMessage->GetPartMimeType(mimeDisplayPart) + "\n";

   tmp = mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message << "\n" << _("Description: ") << tmp << '\n';
   message << _("Size: ")
           << strutil_ltoa(mailMessage->GetPartSize(mimeDisplayPart));
   type = mailMessage->GetPartType(mimeDisplayPart);
   if(type == TYPEMESSAGE || type == TYPETEXT)
      message += String(_(" lines\n"));
   else
      message += String(_(" bytes\n"));

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
   wxMessageBox((char *)message.c_str(), _("MIME information"),
                wxOK|wxCENTRE|wxICON_INFORMATION, this);
}

// @@@@ FIXME it's a mess here
void
wxMessageView::MimeHandle(int mimeDisplayPart)
{
   String tmp, mimetype;
   int type;
   unsigned long len;
   char const *content;

   mimetype = mailMessage->GetPartMimeType(mimeDisplayPart);
   String message = String(  _("MIME type: ")) + mimetype + "\n";

   tmp = mailMessage->GetPartDesc(mimeDisplayPart);
   if(tmp.length() > 0)
      message += String(_("Description: ")) + tmp + String("\n");
   message += String(_("Size: "))
      + strutil_ltoa(mailMessage->GetPartSize(mimeDisplayPart));
   type = mailMessage->GetPartType(mimeDisplayPart);
   if(type == TYPEMESSAGE || type == TYPETEXT)
      message += String(_(" lines"));
   else
      message += String(_(" bytes"));

#  ifdef OS_WIN
      // @@@@ FIXME should somehow adapt MimeList interface for this...
      wxString strExt;
      if ( !GetExtensionFromMimeType(&strExt, mimetype) ) {
         ERRORMESSAGE((_("Unknown MIME type '%s', can't execute."), mimetype));
         return;
      }

      const char *filename = tmpnam(NULL);
      FILE *out = fopen(filename,"wb");
      if( !out ) {
         ERRORMESSAGE((_("Can't open temporary file.")));
         return;
      }

      content = mailMessage->GetPartContent(mimeDisplayPart,&len);
      if(fwrite(content,1,len,out) != len)
      {
         ERRORMESSAGE((_("Cannot write file.")));
         fclose(out);
         return;
      }
      fclose(out);

      wxString strNewName = filename;
      strNewName += strExt;
      if ( rename(filename, strNewName) != 0 ) {
         ERRORMESSAGE((_("Can't rename '%s' to '%s'."),
                      filename, strNewName.c_str()));
         return;
      }

      bool bOk = (int)ShellExecute(NULL, "open", strNewName,
                              NULL, NULL, SW_SHOWNORMAL ) > 32;
      if ( !bOk ) {
         wxLogSysError(_("Can't open attachment"));
         return;
      }

      return;

#  else // Unix
      MimeList *ml = mApplication->GetMimeList();
      String command, flags;
      bool found = ml->GetCommand(mimetype, command, flags);
      if(found)
      {
         const char *filename = tmpnam(NULL);
         FILE *out = fopen(filename,"wb");
         if(out)
         {
            content = mailMessage->GetPartContent(mimeDisplayPart,&len);
            if(fwrite(content,1,len,out) != len)
            {
               ERRORMESSAGE((_("Cannot write file.")));
               fclose(out);
               return;
            }
            fclose(out);
            command = ml->ExpandCommand(command,filename,mimetype);

            // @@@@ what about error handling here (fork, system, ...)?
            if(fork() == 0)
            {
               system(command.c_str());
               unlink(command.c_str());
            }
         }

         return;
      }
#  endif // Unix

   // what can we handle internally?
   if(mimetype == "MESSAGE/RFC822")
   {
      char *filename = wxGetTempFileName("Mtemp");
      MimeSave(mimeDisplayPart,filename);
      MailFolderCC *mf = MailFolderCC::OpenFolder(filename);
      (void) GLOBAL_NEW wxMessageViewFrame(mf, 1, m_FolderView, m_Parent);
      mf->Close();
      wxRemoveFile(filename);
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
                                       folder ? folder->GetProfile() :
                                       NULL); 
      if ( strutil_isempty(filename) )
         return;
   }
   else
      filename = ifilename;

   if(filename)
   {
      VAR(filename.c_str());
      unsigned long len;
      char const *content = mailMessage->GetPartContent(mimeDisplayPart,&len);
      if(! content)
      {
         FATALERROR(("Cannot get message content."));
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
wxMessageView::OnCommandEvent(wxCommandEvent &event)
{
   ClickableInfo *ci;
   
   if(event.GetId() == WXMENU_LAYOUT_CLICK)
   {
      wxLayoutObjectBase *obj;
      obj = (wxLayoutObjectBase *)event.GetClientData();
      ci = (ClickableInfo *)obj->GetUserData();
      if(ci)
      {
         switch(ci->type)
         {
            case ClickableInfo::CI_ICON:
            {
               int x,y;
               wxPoint pos;// = GetClickPosition();
               wxWindow *p = GetParent();
               while(p)
               {
                  p->GetPosition(&x,&y);
                  pos.x += x; pos.y += y;
                  if(p->IsKindOf(CLASSINFO(wxFrame)))
                     break;
                  p = p->GetParent();
               }
               GetPosition(&x,&y);
               pos.x += x; pos.y += y;
               if(m_MimePopup)
                  delete m_MimePopup;
               m_MimePopup = new MimeDialog(this,pos,ci->id);
               m_MimePopup->Show(true);
               break;
            }
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
               String cmd = m_Profile->readEntry(MP_BROWSER,MP_BROWSER_D);
               if ( cmd.IsEmpty() ) {
#                 ifdef OS_WIN
                     bOk = (int)ShellExecute(NULL, "open", ci->url,
                                             NULL, NULL, SW_SHOWNORMAL ) > 32;
                     if ( !bOk ) {
                        wxLogSysError(_("Can't open URL '%s'"), ci->url.c_str());
                     }
#                 else  // Unix
                     // @@@ propose to change it right now
                     wxLogError(_("No command configured to view URLs."));
                     bOk = FALSE;
#                 endif
               }
               else {
                  // not empty, user provided his script - use it
                  cmd += ' '; 
                  cmd += ci->url;
                  bOk = wxExecute(Str(cmd)) != 0;
               }

               wxEndBusyCursor();

               if ( bOk ) {
                  wxLogStatus(frame, _("Opening URL '%s'... done."),
                              ci->url.c_str());
               }
               else {
                  wxLogStatus(frame, _("Opening URL '%s' failed."),
                              ci->url.c_str());
               }

               break;
            }
         }
      }
   }
   else
      OnMenuCommand(event.GetId());
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
#endif
    
   wxLayoutWindow::Print();
} 


#ifdef USE_WXWINDOWS2
BEGIN_EVENT_TABLE(wxMessageViewFrame, wxMFrame)
   EVT_SIZE    (    wxMessageViewFrame::OnSize)
   EVT_MENU    (    -1, wxMessageViewFrame::OnCommandEvent)
   EVT_TOOL    (    -1, wxMessageViewFrame::OnCommandEvent)
   END_EVENT_TABLE()
#endif // wxWin2


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
wxMessageViewFrame::OnCommandEvent(wxCommandEvent &ev)
{
   int id = ev.GetId();
   if(id == WXMENU_MSG_REPLY ||
      id == WXMENU_MSG_FORWARD ||
      id == WXMENU_MSG_PRINT ||
      id == WXMENU_LAYOUT_CLICK)
      m_MessageView->OnCommandEvent(ev);
   else
      wxMFrame::OnCommandEvent(ev);
}

void
wxMessageViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x = 0;
   int y = 0;
   GetClientSize( &x, &y );
   if(m_ToolBar)
      m_ToolBar->SetSize( 1, 0, x-2, 30 );
   if(m_MessageView)
      m_MessageView->SetSize(1,30,x-2,y-30);
};
