/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxFolderView.h"
#endif

#include	<Mcommon.h>
#include	<strutil.h>
#include	<CommonBase.h>
#include	<MApplication.h>
#include	<MailFolderCC.h>
#include	<Message.h>
#include	<wxFolderView.h>
#include	<wxMessageView.h>
#include	<wxComposeView.h>

wxFolderView::wxFolderView(MailFolder *iMailFolder, const String
			   &iname, wxFrame *parent, bool ownsFolderi)
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

   timer = NEW wxFVTimer(mailFolder);
   Update();
}

void
wxFolderView::Build(int x, int y)
{
   wxMenu	*messageMenu;

   AddMenuBar();
   AddFileMenu();
       
   messageMenu = NEW wxMenu;
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
   panel = NEW wxFolderViewPanel(this);
   listBox = NEW wxListBox(panel, (wxFunction) NULL, "",
			   wxMULTIPLE,
			   -1,-1,x,y,
			   0,
			   NULL,
			   wxALWAYS_SB);
}

void
wxFolderView::Update(void)
{
   long i;
   Message	*mptr;
   String	line;
   int	status;
   unsigned long size;
   unsigned day, month, year;
   char	buffer[200];
   const char *format;
   
   bool doesExpand = mApplication.doesExpandVariables();
   mApplication.expandVariables(false);
   format = mApplication.readEntry(MC_DATE_FMT,MC_DATE_FMT_D);
   mApplication.expandVariables(doesExpand);
   listBox->Clear();
   listBoxEntriesCount = mailFolder->CountMessages();
   for(i = 0; i < listBoxEntriesCount; i++)
   {
      mptr = mailFolder->GetMessage(i+1);
      status = mailFolder->GetMessageStatus(i+1,&size,&day,&month,&year);
      line = "";
      if(status & MSG_STAT_UNREAD)	line += 'U';
      if(status & MSG_STAT_DELETED)	line += 'D';
      if(status & MSG_STAT_REPLIED)	line += 'R';
      if(status & MSG_STAT_RECENT)	line += 'N';
      line += " - ";
      line += mptr->Subject() + " - " + mptr->From();
      sprintf(buffer,format,
	      day, month, year);
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
      DELETE timer;
      mailFolder->RegisterView(this,false);
      if(ownsFolder)
	 DELETE mailFolder;
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
   case	WXMENU_MSG_EXPUNGE:
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
      mv = NEW wxMessageView(mailFolder,selections[i]+1,
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

   char *folderName = wxGetTextFromUser(
	 _("Name of the folder to write to?"),
	 _("Save Message"),
	 "",this);
   MailFolderCC	*mf;
   
   if(! folderName || strlen(folderName) == 0)
      return;
   
   for(i = 0; i < n; i++)
   {
      str = "";
      mailFolder->GetMessage(selections[i]+1)->WriteToString(str);
      mf = NEW MailFolderCC(folderName);
      mf->AppendMessage(str.c_str());
      DELETE mf;
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
      cv = NEW wxComposeView(_("Reply"), this,
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
   Create(folderView);
}

void
wxFolderViewPanel::OnDefaultAction(wxItem *item)
{
   int
      n, *selections;
   n = folderView->GetSelections(&selections);
   folderView->OpenMessages(n, selections);
}

void
wxFolderViewPanel::OnCommand(wxWindow &win, wxCommandEvent &event)
{
}
