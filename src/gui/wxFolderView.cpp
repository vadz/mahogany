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
#  include "gui/wxMApp.h"
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

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
END_EVENT_TABLE()

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   m_FolderView->PreviewMessage(event.m_itemIndex);
}

int
wxFolderListCtrl::GetSelections(wxArrayInt &selections) const
{
   long item = -1;

   while( (item = ((wxFolderListCtrl
                    *)this)->GetNextItem(item,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED)) != -1) 
      selections.Add(item);
   return selections.Count();
}
   
void
wxFolderListCtrl::Clear(void)
{
   DeleteAllItems();
   for (int i = 0; i < 5; i++)
      DeleteColumn( i );
   if (m_Style & wxLC_REPORT)
   {
      InsertColumn( 0, "Status",  wxLIST_FORMAT_LEFT, 50 );
      InsertColumn( 1, "Sender",  wxLIST_FORMAT_LEFT, 150 );
      InsertColumn( 2, "Date",    wxLIST_FORMAT_LEFT, 85 );
      InsertColumn( 3, "Size",    wxLIST_FORMAT_LEFT, 50 );
      InsertColumn( 4, "Subject", wxLIST_FORMAT_LEFT, 120 );
   };
}

void
wxFolderListCtrl::SetEntry(long index,
                           String const &status,
                           String const &sender,
                           String const &subject,
                           String const &date,
                           String const &size)
{
   if(index <= GetItemCount())
      SetItem(index, 0, status);
   else
      InsertItem(index, status); // column 0
   SetItem(index, 1, sender);
   SetItem(index, 2, date);
   SetItem(index, 3, size);
   SetItem(index, 4, subject);
}


wxFolderView::wxFolderView(String const & folderName, MWindow *iparent)
{
   wxCHECK_RET(iparent, "NULL parent frame in wxFolderView ctor");
   parent = iparent;
   m_NumOfMessages = 0; // At the beginning there was nothing.
   
   mailFolder = MailFolderCC::OpenFolder(folderName);
   wxCHECK_RET(mailFolder, "can't open folder in wxFolderView ctor" );
   
   initialised = mailFolder->IsInitialised();
   
   if(! initialised)
      return;
   
   listBoxEntries = NULL;

   int x,y;
   parent->GetClientSize(&x, &y);

   m_SplitterWindow = new wxSplitterWindow(parent,-1,wxDefaultPosition,wxSize(x,y),wxSP_3D);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
   m_MessagePreview = new wxMessageView(this,m_SplitterWindow,"MessagePreview");
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl,m_MessagePreview, y/3);
   
   mailFolder->RegisterView(this);
   timer = GLOBAL_NEW wxFVTimer(mailFolder);
   m_SplitterWindow->SetMinimumPaneSize(0);
   m_SplitterWindow->SetFocus();
   Update();
}

void
wxFolderView::Update(void)
{
   long i;
   
   Message  *mptr;
   String   line;
   int   nstatus;
   unsigned long nsize;
   unsigned day, month, year;
   char  buffer[200];
   const char *format;
   int n = mailFolder->CountMessages();
   String status, sender, subject, date, size;
   bool selected;
   
   format = READ_APPCONFIG(MC_DATE_FMT);

   if(n < m_NumOfMessages)  // messages have been deleted, start over
      m_FolderCtrl->Clear();
   
   for(i = 0; i < n; i++)
   {
      mptr = mailFolder->GetMessage(i+1);
      nstatus = mailFolder->GetMessageStatus(i+1,&nsize,&day,&month,&year);
      status = "";
      if(nstatus & MSG_STAT_UNREAD)  status += 'U';
      if(nstatus & MSG_STAT_DELETED) status += 'D';
      if(nstatus & MSG_STAT_REPLIED) status += 'R';
      if(nstatus & MSG_STAT_RECENT)  status += 'N';

      subject = mptr->Subject();
      sender  = mptr->From();
      sprintf(buffer,format, day, month, year);
      date = buffer;
      size = strutil_ultoa(nsize);

      selected = (i <= m_NumOfMessages) ?m_FolderCtrl->IsSelected(n) : false;
      m_FolderCtrl->SetEntry(i,status, sender, subject, date, size);
      m_FolderCtrl->Select(i,selected);
   }
   m_NumOfMessages = n;
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
      GetSelections(selections);
      ForwardMessages(selections);
      break;
   case WXMENU_MSG_DELETE:
      GetSelections(selections);
      DeleteMessages(selections);
      break;
   case WXMENU_MSG_SELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,true);
      break;
   case WXMENU_MSG_DESELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,false);
      break;
   default:
      wxFAIL_MSG("wxFolderView::OnMenuCommand() called with illegal id.");
   }
}

int
wxFolderView::GetSelections(wxArrayInt& selections)
{
   return m_FolderCtrl->GetSelections(selections);
}

void
wxFolderView::OpenMessages(const wxArrayInt& selections)
{
   String title;
   Message *mptr;
   wxMessageViewFrame *mv;

   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
   {
      mptr = mailFolder->GetMessage(selections[i]+1);
      title = mptr->Subject() + " - " + mptr->From();
      mv = GLOBAL_NEW wxMessageViewFrame(mailFolder,selections[i]+1,
                                         this);
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
   Update();
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
                                     _("Save Message"),"",parent);
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
      str2 += prefix;
      while(*cptr)
      {
         str2 += *cptr;
         if(*cptr == '\n' && *(cptr+1))
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


void
wxFolderView::ForwardMessages(const wxArrayInt& selections)
{
   int i;
   String str;
   String str2, prefix;
   wxComposeView *cv;
   Message *msg;

   int n = selections.Count();
   prefix = mailFolder->GetProfile()->readEntry(MP_REPLY_MSGPREFIX,MP_REPLY_MSGPREFIX_D);
   for(i = 0; i < n; i++)
   {
      str = "";
      msg = mailFolder->GetMessage(selections[i]+1);
      cv = GLOBAL_NEW wxComposeView(_("Forward"),parent,
                                    mailFolder->GetProfile());
      cv->SetSubject(mailFolder->GetProfile()
                     ->readEntry(MP_FORWARD_PREFIX,MP_FORWARD_PREFIX_D) + msg->Subject());

      mailFolder->GetMessage(selections[i]+1)->WriteToString(str);
#pragma warning FIXME: needs completion or even easier interface
//      cv->InsertFile(tmpfilename,"MESSAGE/RFC822",TYPEMESSAGE);
   }

}

void
wxFolderView::SetSize(const int x, const int y, const int width, int
                      height)
{
//   wxPanel::SetSize(x,y,width,height);
   m_SplitterWindow->SetSize( x, y, width, height );
}

BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
   EVT_SIZE(    wxFolderViewFrame::OnSize)
   EVT_MENU(-1,    wxFolderViewFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxFolderViewFrame::OnCommandEvent)
END_EVENT_TABLE()

   wxFolderViewFrame::wxFolderViewFrame(const String &folderName,
                                        wxFrame *parent)
      : MFrame(folderName, parent)
{
   VAR(folderName);
   m_FolderView = NULL;
   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   SetMenuBar(menuBar);

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
#ifdef USE_WXWINDOWS2
   int width, height;
   GetClientSize(&width, &height);
   m_ToolBar = CreateToolBar();
   m_ToolBar->SetSize( -1, -1, width, 30 );
   m_ToolBar->SetMargins( 2, 2 );
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_open"), WXMENU_MSG_OPEN, "Open message");
   TB_AddTool(m_ToolBar, ICON("tb_close"), WXMENU_FILE_CLOSE, "Close folder");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_mail_compose"), WXMENU_FILE_COMPOSE, "Compose message");
   TB_AddTool(m_ToolBar, ICON("tb_mail_forward"), WXMENU_MSG_FORWARD, "Forward message");
   TB_AddTool(m_ToolBar, ICON("tb_mail_reply"), WXMENU_MSG_REPLY, "Reply to message");
   TB_AddTool(m_ToolBar, ICON("tb_print"), WXMENU_MSG_PRINT, "Print message");
   TB_AddTool(m_ToolBar, ICON("tb_trash"), WXMENU_MSG_DELETE, "Delete message");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_book_open"), WXMENU_EDIT_ADB, "Edit Database");
   TB_AddTool(m_ToolBar, ICON("tb_preferences"), WXMENU_EDIT_PREFERENCES, "Edit Preferences");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_help"), WXMENU_HELP_ABOUT, "Help");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_exit"), WXMENU_FILE_EXIT, "Exit M");
#endif

   m_FolderView = new wxFolderView(folderName,this);
   Show(true);
}
   
void
wxFolderViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
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
//   if(m_ToolBar)
//      m_ToolBar->SetSize( 1, 0, x-2, 30 );
   if(m_FolderView)
      m_FolderView->SetSize(0,0,x,y);
};
