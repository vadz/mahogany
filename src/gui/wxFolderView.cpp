/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.10  1998/06/12 16:07:00  KB
 * updated
 *
 * Revision 1.9  1998/06/09 14:11:29  VZ
 *
 * event tables for menu events added (wxWin2)
 *
 * Revision 1.8  1998/06/05 16:56:22  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.7  1998/05/24 08:22:58  KB
 * changed the creation/destruction of MailFolders, now done through
 * MailFolder::Open/CloseFolder, made constructor/destructor private,
 * this allows multiple view on the same folder
 *
 * Revision 1.6  1998/05/18 17:48:40  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.5  1998/05/15 21:59:35  VZ
 *
 * added 4th argument (id, unused in wxWin1) to CreateButton() calls
 *
 * Revision 1.4  1998/05/13 19:02:17  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
 * Revision 1.3  1998/05/11 20:57:30  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma  implementation "wxFolderView.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef USE_PCH
#  include "strutil.h"
#  include "MFrame.h"
#  include "MLogFrame.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#endif

#include <XFace.h>

#include "Mdefaults.h"

#include "MApplication.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#include "MessageView.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"

#ifdef USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxFolderView, wxMFrame)
      EVT_MENU(WXMENU_MSG_PRINT,       wxFolderView::OnPrint)
      EVT_MENU(WXMENU_MSG_DELETE,      wxFolderView::OnDelete)
      EVT_MENU(WXMENU_MSG_SAVE,        wxFolderView::OnSave)
      EVT_MENU(WXMENU_MSG_OPEN,        wxFolderView::OnOpen)
      EVT_MENU(WXMENU_MSG_REPLY,       wxFolderView::OnReply)
      EVT_MENU(WXMENU_MSG_FORWARD,     wxFolderView::OnForward)
      EVT_MENU(WXMENU_MSG_SELECTALL,   wxFolderView::OnSelectAll)
      EVT_MENU(WXMENU_MSG_DESELECTALL, wxFolderView::OnDeselectAll)
      EVT_MENU(WXMENU_MSG_EXPUNGE,     wxFolderView::OnExpunge)
   END_EVENT_TABLE()
#endif // wxWin2
 
wxFolderView::wxFolderView(MailFolder *iMailFolder,
                           const String &iname,
                           wxFrame *parent,
                           bool ownsFolderi)
   : wxMFrame(iname, parent)
{

   mailFolder = iMailFolder;
   initialised = mailFolder->IsInitialised();

   if(! initialised)
      return;
   
   listBoxEntries = NULL;
   panel = NULL;
   ownsFolder = ownsFolderi;

   GetSize(&width,&height);
   Build(width,height);
   mailFolder->RegisterView(this);

   timer = GLOBAL_NEW wxFVTimer(mailFolder);
   Update();
}

void
wxFolderView::Build(int x, int y)
{
   wxMenu   *messageMenu;

   AddMenuBar();
   AddFileMenu();
       
   messageMenu = GLOBAL_NEW wxMenu;
   messageMenu->Append(WXMENU_MSG_OPEN, (char *)_("&Open"));
   messageMenu->Append(WXMENU_MSG_REPLY, (char *)_("&Reply"));
   messageMenu->Append(WXMENU_MSG_FORWARD, (char *)_("&Forward"));
   messageMenu->Append(WXMENU_MSG_DELETE,(char *)_("&Delete"));
   messageMenu->Append(WXMENU_MSG_SAVE,(char *)_("&Save"));
   messageMenu->Append(WXMENU_MSG_SELECTALL, (char *)_("Select &all"));
   messageMenu->Append(WXMENU_MSG_DESELECTALL, (char *)_("&Deselect all"));
   messageMenu->AppendSeparator();
   messageMenu->Append(WXMENU_MSG_EXPUNGE,(char *)_("Ex&punge"));
   menuBar->Append(messageMenu, (char *)_("&Message"));

   AddHelpMenu();
   SetMenuBar(menuBar);

   width = x; height = y;
   x = (int) x - WXFRAME_WIDTH_DELTA;
   y = (int) y - WXFRAME_HEIGHT_DELTA;
   panel = GLOBAL_NEW wxFolderViewPanel(this);
   listBox = CreateListBox(panel, -1, -1, x, y);
}

void
wxFolderView::Update(void)
{
   long i;
   
   Message  *mptr;
   String   line;
   int   status;
   unsigned long size;
   unsigned day, month, year;
   char  buffer[200];
   const char *format;
   
#  ifdef USE_APPCONF
      bool doesExpand = Profile::GetAppConfig()->doesExpandVariables() != 0;
      Profile::GetAppConfig()->expandVariables(false);
#  endif

   format = READ_APPCONFIG(MC_DATE_FMT);

#  ifdef USE_APPCONF
      Profile::GetAppConfig()->expandVariables(doesExpand);
#  endif

   listBox->Clear();
   listBoxEntriesCount = mailFolder->CountMessages();
   for(i = 0; i < listBoxEntriesCount; i++)
   {
      mptr = mailFolder->GetMessage(i+1);
      status = mailFolder->GetMessageStatus(i+1,&size,&day,&month,&year);
      line = "";
      if(status & MSG_STAT_UNREAD)  line += 'U';
      if(status & MSG_STAT_DELETED) line += 'D';
      if(status & MSG_STAT_REPLIED) line += 'R';
      if(status & MSG_STAT_RECENT)  line += 'N';
      line += " - ";
      line += mptr->Subject() + " - " + mptr->From();
      sprintf(buffer,format, day, month, year);
      line += " - ";
      line += buffer;
      line += " - ";
      line += strutil_ultoa(size);
      listBox->Append((char *) line.c_str());
   }
}

wxFolderView::~wxFolderView()
{
   if(initialised)
   {
      timer->Stop();
      GLOBAL_DELETE timer;
      mailFolder->RegisterView(this,false);
      if(ownsFolder)
    GLOBAL_DELETE mailFolder;
   }
}


void
wxFolderView::OnMenuCommand(int id)
{
   int
      n,
      *selections;
   
   switch(id)
   {
   case  WXMENU_MSG_EXPUNGE:
      mailFolder->ExpungeMessages();
      Update();
      break;
   case WXMENU_MSG_OPEN:
      n = GetSelections(&selections);
      OpenMessages(n, selections);
      break;
   case WXMENU_MSG_SAVE:
      n = GetSelections(&selections);
      SaveMessages(n, selections);
      break;
   case WXMENU_MSG_REPLY:
      n = GetSelections(&selections);
      ReplyMessages(n, selections);
      break;
   case WXMENU_MSG_FORWARD:
      //n = GetSelections(&selections);
      //SaveMessages(n, selections);
      break;
   case WXMENU_MSG_DELETE:
      n = GetSelections(&selections);
      DeleteMessages(n, selections);
      Update();
      break;
   case WXMENU_MSG_SELECTALL:
      for(n = 0; n < listBox->Number(); n++)
         listBox->SetSelection(n);
      break;
   case WXMENU_MSG_DESELECTALL:
      for(n = 0; n < listBox->Number(); n++)
    listBox->Deselect(n);
      break;
   default:
      wxMFrame::OnMenuCommand(id);
   }
}

int
wxFolderView::GetSelections(int **selections)
{
   int n = listBox->GetSelections(selections);
   VAR(n);
   return n;
}

void
wxFolderView::OpenMessages(int n, int *selections)
{
   String title;
   Message *mptr;
   wxMessageView *mv;

   int i;
   for(i = 0; i < n; i++)
   {
      mptr = mailFolder->GetMessage(selections[i]+1);
      title = mptr->Subject() + " - " + mptr->From();
      mv = GLOBAL_NEW wxMessageView(mailFolder,selections[i]+1,
              "wxMessageView",
              this);
      mv->SetTitle(title);
   }
}

void
wxFolderView::DeleteMessages(int n, int *selections)
{
   int i;
   for(i = 0; i < n; i++)
      mailFolder->DeleteMessage(selections[i]+1);
}

void
wxFolderView::SaveMessages(int n, int *selections)
{
   int i;
   String str;

#  ifdef  USE_WXWINDOWS2
      wxString 
#  else
      char *
#  endif
   folderName = wxGetTextFromUser(_("Name of the folder to write to?"),
                                  _("Save Message"),"",this);
   MailFolderCC   *mf;
   
   if(! folderName || strlen(folderName) == 0)
      return;
   
   for(i = 0; i < n; i++)
   {
      str = "";
      mailFolder->GetMessage(selections[i]+1)->WriteToString(str);
      mf = MailFolderCC::OpenFolder(folderName);
      mf->AppendMessage(str.c_str());
      mf->CloseFolder();
   }
}

void
wxFolderView::ReplyMessages(int n, int *selections)
{
   int i;
   String str;
   String str2, prefix;
   const char *cptr;
   wxComposeView *cv;
   Message *msg;

   prefix = mailFolder->GetProfile()->readEntry(MP_REPLY_MSGPREFIX,MP_REPLY_MSGPREFIX_D);
   for(i = 0; i < n; i++)
   {
      str = "";
      msg = mailFolder->GetMessage(selections[i]+1);
      msg->WriteToString(str, false);
      cv = GLOBAL_NEW wxComposeView(_("Reply"), this,
              mailFolder->GetProfile());
      cptr = str.c_str();
      str2 = "";
      while(*cptr)
      {
    str2 += *cptr;
    if(*cptr == '\n')
       str2 += prefix;
    cptr++;
      }
      cv->InsertText(str2);
      cv->Show(TRUE);
      String
    name, email;
      email = msg->Address(name, MAT_REPLYTO);
      if(name.length() > 0)
    email = name + String(" <") + email + String(">");
      cv->SetTo(email);
      cv->SetSubject(mailFolder->GetProfile()
           ->readEntry(MP_REPLY_PREFIX,MP_REPLY_PREFIX_D) + msg->Subject());
   }
}

wxFolderViewPanel::wxFolderViewPanel(wxFolderView *iFolderView)
{
   folderView = iFolderView;

#  ifdef USE_WXWINDOWS2
      Create(folderView, -1);
#  else
      Create(folderView);
#  endif
}

void
wxFolderViewPanel::OnDefaultAction(wxItem *item)
{
   int
      n, *selections;
   n = folderView->GetSelections(&selections);
   folderView->OpenMessages(n, selections);
}

#ifdef USE_WXWINDOWS2
   // @@@ wxFolderViewPanel::OnCommand() doesn't eixst in wxWin2
#else // wxWin1
void
wxFolderViewPanel::OnCommand(wxWindow &win, wxCommandEvent &event)
{
}
#endif // wxWin1/2

