/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$         *
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
#  include <wx/dynarray.h>
#if 0
   BEGIN_EVENT_TABLE(wxFolderView, wxPanel)
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
#endif
#endif // wxWin2
 
   wxFolderView::wxFolderView(String const & folderName,
                           wxFrame *iparent)
   : wxPanel(iparent)
{
   wxCHECK(iparent);
   parent = iparent;

   mailFolder = MailFolderCC::OpenFolder(folderName);
   wxCHECK(mailFolder);
   
   initialised = mailFolder->IsInitialised();
   
   if(! initialised)
      return;
   
   listBoxEntries = NULL;

   GetSize(&width,&height);
   Build(width,height);
   mailFolder->RegisterView(this);

   timer = GLOBAL_NEW wxFVTimer(mailFolder);
   Update();
}

void
wxFolderView::Build(int x, int y)
{
   width = x; height = y;
   x = (int) x - WXFRAME_WIDTH_DELTA;
   y = (int) y - WXFRAME_HEIGHT_DELTA;
   //listBox = CreateListBox(this, -1, -1, , y);
   // wxWin2:
   listBox = new wxListBox(this,-1);
   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->top.SameAs  (this, wxTop);
   c->left.SameAs (this, wxLeft);
   c->right.SameAs   (this, wxRight);
   c->bottom.SameAs  (this, wxBottom);
   listBox->SetConstraints(c);
   listBox->SetAutoLayout(true);
   this->SetAutoLayout(true);
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
      mailFolder->Close();
   }
}

void
wxFolderView::OnMenuCommand(int id)
{
   int n;
   wxArrayInt selections;
   
   switch(id)
   {
   case  WXMENU_MSG_EXPUNGE:
      mailFolder->ExpungeMessages();
      Update();
      break;
   case WXMENU_MSG_OPEN:
      GetSelections(selections);
      OpenMessages(selections);
      break;
   case WXMENU_MSG_SAVE:
      GetSelections(selections);
      SaveMessages(selections);
      break;
   case WXMENU_MSG_REPLY:
      GetSelections(selections);
      ReplyMessages(selections);
      break;
   case WXMENU_MSG_FORWARD:
      //n = GetSelections(selections);
      //SaveMessages(n, selections);
      break;
   case WXMENU_MSG_DELETE:
      GetSelections(selections);
      DeleteMessages(selections);
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
      wxFAIL_MSG("wxFolderView::OnMenuCommand() called with illegal id.");
   }
}

int
wxFolderView::GetSelections(wxArrayInt& selections)
{
   return listBox->GetSelections(selections);
}

void
wxFolderView::OpenMessages(const wxArrayInt& selections)
{
   String title;
   Message *mptr;
   wxMessageView *mv;

   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
   {
      mptr = mailFolder->GetMessage(selections[i]+1);
      title = mptr->Subject() + " - " + mptr->From();
      mv = GLOBAL_NEW wxMessageView(mailFolder,selections[i]+1,
              "wxMessageView",parent);
      mv->SetTitle(title);
   }
}

void
wxFolderView::DeleteMessages(const wxArrayInt& selections)
{
   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
      mailFolder->DeleteMessage(selections[i]+1);
}

void
wxFolderView::SaveMessages(const wxArrayInt& selections)
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
   
   int n = selections.Count();
   for(i = 0; i < n; i++)
   {
      str = "";
      mailFolder->GetMessage(selections[i]+1)->WriteToString(str);
      mf = MailFolderCC::OpenFolder(folderName);
      mf->AppendMessage(str.c_str());
      mf->Close();
   }
}

void
wxFolderView::ReplyMessages(const wxArrayInt& selections)
{
   int i;
   String str;
   String str2, prefix;
   const char *cptr;
   wxComposeView *cv;
   Message *msg;

   int n = selections.Count();
   prefix = mailFolder->GetProfile()->readEntry(MP_REPLY_MSGPREFIX,MP_REPLY_MSGPREFIX_D);
   for(i = 0; i < n; i++)
   {
      str = "";
      msg = mailFolder->GetMessage(selections[i]+1);
      msg->WriteToString(str, false);
      cv = GLOBAL_NEW wxComposeView(_("Reply"),parent,
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
   wxArrayInt selections;
   folderView->GetSelections(selections);
   folderView->OpenMessages(selections);
}

#ifdef USE_WXWINDOWS2
   // @@@ wxFolderViewPanel::OnCommand() doesn't eixst in wxWin2
#else // wxWin1
void
wxFolderViewPanel::OnCommand(wxWindow &win, wxCommandEvent &event)
{
}
#endif // wxWin1/2






#ifdef USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
      EVT_MENU(WXMENU_MSG_PRINT,       wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_DELETE,      wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_SAVE,        wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_OPEN,        wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_REPLY,       wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_FORWARD,     wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_SELECTALL,   wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_DESELECTALL, wxFolderViewFrame::OnCommandEvent)
      EVT_MENU(WXMENU_MSG_EXPUNGE,     wxFolderViewFrame::OnCommandEvent)
   END_EVENT_TABLE()
#endif // wxWin2

wxFolderViewFrame::wxFolderViewFrame(const String &folderName,
                                     wxFrame *parent)
   : wxMFrame("FolderView", parent)
{
   AddFileMenu();
   AddMessageMenu();
   SetMenuBar(menuBar);
   m_FolderView = new wxFolderView(folderName, this);
   Show();
}
   
void
wxFolderViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   wxCHECK(m_FolderView);

   int id = event.GetId();

   if(WXMENU_CONTAINS(MSG,id))
      m_FolderView->OnMenuCommand(id);
   else
      wxMFrame::OnMenuCommand(id);
}

