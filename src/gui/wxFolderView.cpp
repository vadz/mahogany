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
// toolbar icons
#   include   "../src/icons/tb_exit.xpm"
#   include   "../src/icons/tb_close.xpm"
#   include   "../src/icons/tb_help.xpm"
#   include   "../src/icons/tb_open.xpm"
#   include   "../src/icons/tb_mail_compose.xpm"
#   include   "../src/icons/tb_mail_reply.xpm"
#   include   "../src/icons/tb_mail_forward.xpm"
#   include   "../src/icons/tb_print.xpm"
#   include   "../src/icons/tb_trash.xpm"
#   include   "../src/icons/tb_book_open.xpm"
#   include   "../src/icons/tb_preferences.xpm"
#  include <wx/dynarray.h>
#endif
 
wxFolderView::wxFolderView(String const & folderName, wxFrame *iparent)
            : wxPanel(iparent)
{
   wxCHECK_RET(iparent, "NULL parent frame in wxFolderView ctor");
   parent = iparent;

   mailFolder = MailFolderCC::OpenFolder(folderName);
   wxCHECK_RET(mailFolder, "can't open folder in wxFolderView ctor" );
   
   initialised = mailFolder->IsInitialised();
   
   if(! initialised)
      return;
   
   listBoxEntries = NULL;

   //wxwin2
   listBox = new wxListBox(this,-1, wxPoint(50,50), wxSize(200,200));

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->top.SameAs  (this, wxTop);
   c->left.SameAs (this, wxLeft);
   c->right.SameAs   (this, wxRight);
   c->bottom.SameAs  (this, wxBottom);
   listBox->SetConstraints(c);
   listBox->SetAutoLayout(true);
   this->SetAutoLayout(true);
   
   mailFolder->RegisterView(this);
   timer = GLOBAL_NEW wxFVTimer(mailFolder);
   Update();
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
      mf = MailFolderCC::OpenFolder(Str(folderName));
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
      EVT_SIZE    (    wxFolderViewFrame::OnSize)
   END_EVENT_TABLE()
#endif // wxWin2

wxFolderViewFrame::wxFolderViewFrame(const String &folderName,
                                     wxFrame *parent)
   : wxMFrame(folderName, parent)
{
   VAR(folderName);
   m_FolderView = NULL;
//   wxMFrame::Create(folderName, parent);
   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   SetMenuBar(menuBar);

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
#ifdef USE_WXWINDOWS2
   int width, height;
   GetClientSize(&width, &height);
   m_ToolBar = new wxMToolBar( this, /*id*/-1, wxPoint(2,60), wxSize(width-4,26) );
   m_ToolBar->SetMargins( 2, 2 );
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, tb_open, WXMENU_MSG_OPEN, "Open message");
   TB_AddTool(m_ToolBar, tb_close, WXMENU_FILE_CLOSE, "Close folder");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, tb_mail_compose, WXMENU_FILE_COMPOSE, "Compose message");
   TB_AddTool(m_ToolBar, tb_mail_forward, WXMENU_MSG_FORWARD, "Forward message");
   TB_AddTool(m_ToolBar, tb_mail_reply, WXMENU_MSG_REPLY, "Reply to message");
   TB_AddTool(m_ToolBar, tb_print, WXMENU_MSG_PRINT, "Print message");
   TB_AddTool(m_ToolBar, tb_trash, WXMENU_MSG_DELETE, "Delete message");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, tb_book_open, WXMENU_EDIT_ADB, "Edit Database");
   TB_AddTool(m_ToolBar, tb_preferences, WXMENU_EDIT_PREFERENCES, "Edit Preferences");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, tb_help, WXMENU_HELP_ABOUT, "Help");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, tb_exit, WXMENU_FILE_EXIT, "Exit M");
#endif

   m_FolderView = new wxFolderView(folderName,this);
   Show(true);
}
   
void
wxFolderViewFrame::OnMenuCommand(int id)
{
   if(WXMENU_CONTAINS(MSG,id))
      m_FolderView->OnMenuCommand(id);
   else
      wxMFrame::OnMenuCommand(id);
}

void
wxFolderViewFrame::OnSize( wxSizeEvent &event )
   
{
  int x = 0;
  int y = 0;
  GetClientSize( &x, &y );
  if(m_ToolBar)
     m_ToolBar->SetSize( 1, 0, x-2, 30 );
  if(m_FolderView)
     m_FolderView->SetSize(1,30,x-2,y);
};
