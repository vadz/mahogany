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
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"
#endif

#include <XFace.h>

#include <wx/file.h>

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#include "MessageView.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"

#include "gui/wxIconManager.h"
#include "gui/wxMIds.h"
#include "MDialogs.h"

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxListCtrl)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
   EVT_SIZE              (wxFolderListCtrl::OnSize)
END_EVENT_TABLE()



#define   LCFIX ((wxFolderListCtrl *)this)->


static const char *wxFLC_ColumnNames[] =
{
   "Status","Date","Size","From","Subject"
};
   
void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   m_FolderView->PreviewMessage(event.m_itemIndex);
}


wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
   :wxListCtrl(parent,-1,wxDefaultPosition,wxSize(500,300),wxLC_REPORT)
{
   ProfileBase *p = fv->GetProfile();
      
   m_Parent = parent;
   m_FolderView = fv;
   m_Style = wxLC_REPORT;

   m_columns[WXFLC_STATUS] = READ_CONFIG(p,MP_FLC_STATUSCOL);
   m_columnWidths[WXFLC_STATUS] = READ_CONFIG(p,MP_FLC_STATUSWIDTH);
   m_columns[WXFLC_DATE] = READ_CONFIG(p,MP_FLC_DATECOL);
   m_columnWidths[WXFLC_DATE] = READ_CONFIG(p,MP_FLC_DATEWIDTH);
   m_columns[WXFLC_SUBJECT] = READ_CONFIG(p,MP_FLC_SUBJECTCOL);
   m_columnWidths[WXFLC_SUBJECT] = READ_CONFIG(p,MP_FLC_SUBJECTWIDTH);
   m_columns[WXFLC_SIZE] = READ_CONFIG(p,MP_FLC_SIZECOL);
   m_columnWidths[WXFLC_SIZE] = READ_CONFIG(p,MP_FLC_SIZEWIDTH);
   m_columns[WXFLC_FROM] = READ_CONFIG(p,MP_FLC_FROMCOL);
   m_columnWidths[WXFLC_FROM] = READ_CONFIG(p,MP_FLC_FROMWIDTH);

   for(int i = 0; i < WXFLC_NUMENTRIES; i++)
      if(m_columns[i] == 0)
      {
         m_firstColumn = i;
         break;
      }
   
   Clear();
}

int
wxFolderListCtrl::GetSelections(wxArrayInt &selections) const
{
   long item = -1;

   for(item = 0; item < LCFIX GetItemCount(); item++)
      if(LCFIX GetItemState(item,wxLIST_STATE_SELECTED))
         selections.Add(item);
   return selections.Count();
}
void
wxFolderListCtrl::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x,y,i;
   GetClientSize(&x,&y);
   
   if (m_Style & wxLC_REPORT)
      for(i = 0; i < WXFLC_NUMENTRIES; i++)
         SetColumnWidth(m_columns[i],(m_columnWidths[i]*x)/100*9/10);
}

void
wxFolderListCtrl::Clear(void)
{
   int x,y;
   GetClientSize(&x,&y);
   
   DeleteAllItems();
   for (int i = 0; i < WXFLC_NUMENTRIES; i++)
      DeleteColumn( i );
   
   if (m_Style & wxLC_REPORT)
   {
      for(int c = 0; c < WXFLC_NUMENTRIES; c++)
         for (int i = 0; i < WXFLC_NUMENTRIES; i++)
            if(m_columns[i] == c)
               InsertColumn( c, _(wxFLC_ColumnNames[i]),
                             wxLIST_FORMAT_LEFT,
                             (m_columnWidths[i]*x)/100 );
   }
}

void
wxFolderListCtrl::SetEntry(long index,
                           String const &status,
                           String const &sender,
                           String const &subject,
                           String const &date,
                           String const &size)
{
   if(index >= GetItemCount())
      InsertItem(index, status); // column 0
   
   SetItem(index, m_columns[WXFLC_STATUS], status);
   SetItem(index, m_columns[WXFLC_FROM], sender);
   SetItem(index, m_columns[WXFLC_DATE], date);
   SetItem(index, m_columns[WXFLC_SIZE], size);
   SetItem(index, m_columns[WXFLC_SUBJECT], subject);
}


wxFolderView::wxFolderView(String const & folderName, MWindow *iparent)
{
   wxCHECK_RET(iparent, "NULL parent frame in wxFolderView ctor");
   parent = iparent;
   m_NumOfMessages = 0; // At the beginning there was nothing.
   m_UpdateSemaphore = false;
   m_SplitterWindow = 0;
   
   mailFolder = MailFolderCC::OpenFolder(folderName);
   if ( !mailFolder )
   {
      ERRORMESSAGE((_("Can't open folder '%s'."), folderName.c_str()));
      wxASSERT( !initialised );
      return;
   }

   initialised = mailFolder->IsInitialised();
   int x,y;
   parent->GetClientSize(&x, &y);
   
   m_SplitterWindow = new wxSplitterWindow(parent,-1,wxDefaultPosition,wxSize(x,y),wxSP_3D,wxSP_BORDER);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
   m_MessagePreview = new wxMessageView(this,m_SplitterWindow,"MessagePreview");
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl,m_MessagePreview, y/3);
   
   mailFolder->RegisterView(this);
   timer = GLOBAL_NEW wxFVTimer(mailFolder);
   m_SplitterWindow->SetMinimumPaneSize(0);
   m_SplitterWindow->SetFocus();
   Update();
   if(m_NumOfMessages > 0)
   {
      m_FolderCtrl->SetItemState(0,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
      // the callback will preview the (just) selected message
   }
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
   int n;
   String status, sender, subject, date, size;
   bool selected;

   if(m_UpdateSemaphore == true)
      return; // don't call this code recursively
   m_UpdateSemaphore = true;
   
   n = mailFolder->CountMessages();

   // mildly annoying, but have to do it in order to prevent the generation of
   // error messages about failed env var expansion (this string contains '%'
   // which introduce env vars under Windows)
   {
      ProfileEnvVarSuspend suspend(mApplication->GetProfile());
      format = READ_APPCONFIG(MC_DATE_FMT);
   }

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

      delete mptr;
   }
   m_NumOfMessages = n;
   m_UpdateSemaphore = false;
}

wxFolderView::~wxFolderView()
{
   if(initialised)
   {
      if(mailFolder) { // mark messages as seen
         for(int i = 0; i < m_NumOfMessages; i++)
            mailFolder->SetMessageFlag(i, MSG_STAT_UNREAD, false);
      }
               
      timer->Stop();
      GLOBAL_DELETE timer;
      mailFolder->RegisterView(this,false);
      mailFolder->Close();
   }
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
   int n;
   wxArrayInt selections;

   switch(event.GetId())
   {
   case WXMENU_LAYOUT_CLICK:
      m_MessagePreview->OnCommandEvent(event);
      break;
   case  WXMENU_MSG_EXPUNGE:
      mailFolder->ExpungeMessages();
      Update();
      break;
   case WXMENU_MSG_OPEN:
      GetSelections(selections);
      OpenMessages(selections);
      break;
   case WXMENU_MSG_SAVE_TO_FOLDER:
      GetSelections(selections);
      SaveMessagesToFolder(selections);
      break;
   case WXMENU_MSG_SAVE_TO_FILE:
      GetSelections(selections);
      SaveMessagesToFile(selections);
      break;
   case WXMENU_MSG_REPLY:
      GetSelections(selections);
      ReplyMessages(selections);
      break;
   case WXMENU_MSG_FORWARD:
      GetSelections(selections);
      ForwardMessages(selections);
      break;
   case WXMENU_MSG_UNDELETE:
      GetSelections(selections);
      UnDeleteMessages(selections);
      break;
   case WXMENU_MSG_DELETE:
      GetSelections(selections);
      DeleteMessages(selections);
      break;
   case WXMENU_MSG_SELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,TRUE);
      break;
   case WXMENU_MSG_DESELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,FALSE);
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

      delete mptr;
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
wxFolderView::UnDeleteMessages(const wxArrayInt& selections)
{
   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
      mailFolder->UnDeleteMessage(selections[i]+1);
   Update();
}

void
wxFolderView::SaveMessages(const wxArrayInt& selections, String const &folderName)
{
   int i;

   MailFolderCC   *mf;
   
   if(strutil_isempty(folderName))
      return;
   
   int n = selections.Count();
   for(i = 0; i < n; i++)
   {
      mf = MailFolderCC::OpenFolder(Str(folderName));
      mf->AppendMessage(*(mailFolder->GetMessage(selections[i]+1)));
      mf->Close();
   }
}

void
wxFolderView::SaveMessagesToFolder(const wxArrayInt& selections)
{
   wxString folderName;
   if ( MInputBox(&folderName, _("Save Message"),
                  _("Name of the folder to write to?"),
                  parent, "SaveFolderName") ) {
      SaveMessages(selections,folderName);
   }
}

void
wxFolderView::SaveMessagesToFile(const wxArrayInt& selections)
{
   String
      filename =
      MDialog_FileRequester(NULLstring, parent, NULLstring,
                            NULLstring, NULLstring, NULLstring, true,
                            mailFolder->GetProfile());   

   // truncate the file
   wxFile file(filename, wxFile::write);
   file.Close();

   SaveMessages(selections,filename);
}

void
wxFolderView::ReplyMessages(const wxArrayInt& selections)
{
   int i,np,p;
   String str;
   String str2, prefix;
   const char *cptr;
   wxComposeView *cv;
   Message *msg;

   int n = selections.Count();
   prefix = mailFolder->GetProfile()->readEntry(MP_REPLY_MSGPREFIX,MP_REPLY_MSGPREFIX_D);
   for(i = 0; i < n; i++)
   {
      cv = GLOBAL_NEW wxComposeView(_("Reply"),parent,
                                    mailFolder->GetProfile());
      str = "";
      msg = mailFolder->GetMessage(selections[i]+1);
      np = msg->CountParts();
      for(p = 0; p < np; p++)
      {
         if(msg->GetPartType(p) == TYPETEXT)
         {
            str = msg->GetPartContent(p);
            cptr = str.c_str();
            str2 = "";
            str2 += prefix;
            while(*cptr)
            {
               if(*cptr == '\r')
               {
                  cptr++;
                  continue;
               }
               str2 += *cptr;
               if(*cptr == '\n' && *(cptr+1))
                  str2 += prefix;
               cptr++;
            }
            cv->InsertText(str2);
         }
      }
      cv->Show(TRUE);
      String
         name, email;
      email = msg->Address(name, MAT_REPLYTO);
      if(name.length() > 0)
         email = name + String(" <") + email + String(">");
      cv->SetTo(email);
      cv->SetSubject(mailFolder->GetProfile()
                     ->readEntry(MP_REPLY_PREFIX,String(MP_REPLY_PREFIX_D)) + msg->Subject());
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
                     ->readEntry(MP_FORWARD_PREFIX,String(MP_FORWARD_PREFIX_D)) + msg->Subject());

      mailFolder->GetMessage(selections[i]+1)->WriteToString(str);
      cv->InsertData(strutil_strdup(str),str.Length(),"MESSAGE/RFC822",TYPEMESSAGE);
   }
}

void
wxFolderView::SetSize(const int x, const int y, const int width, int
                      height)
{
//   wxPanel::SetSize(x,y,width,height);
   if(m_SplitterWindow)
      m_SplitterWindow->SetSize( x, y, width, height );
}

BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
   EVT_SIZE(    wxFolderViewFrame::OnSize)
   EVT_MENU(-1,    wxFolderViewFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxFolderViewFrame::OnCommandEvent)
END_EVENT_TABLE()

wxFolderViewFrame::wxFolderViewFrame(const String &folderName, wxFrame *parent)
                 : MFrame(strutil_getfilename(folderName), parent)
{
   wxLogDebug("Creating folder view '%s'.", folderName.c_str());

   wxString strTitle;
   strTitle.Printf(_("Folder '%s'"), folderName.c_str());
   SetTitle(strTitle);

   // menu
   m_FolderView = NULL;
   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   SetMenuBar(m_MenuBar);

   // status bar
   CreateStatusBar();

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
#ifdef USE_WXWINDOWS2
   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_FOLDER);
#endif

   m_FolderView = new wxFolderView(folderName,this);
   if ( m_FolderView->IsInitialised() )
      Show(true);
   else {
      delete m_FolderView;

      Close(true);
   }
}
   
void
wxFolderViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   if(id == WXMENU_EDIT_PREF) // edit folder profile
   {
      MDialog_FolderProfile(this, m_FolderView->GetProfile());
      return;
   }
   if(WXMENU_CONTAINS(MSG,id) || id == WXMENU_LAYOUT_CLICK)
      m_FolderView->OnCommandEvent(event);
   else
      wxMFrame::OnMenuCommand(id);
}

void
wxFolderViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x = 0;
   int y = 0;
   GetClientSize( &x, &y );
//   if(m_ToolBar)
//      m_ToolBar->SetSize( 1, 0, x-2, 30 );
   if(m_FolderView)
      m_FolderView->SetSize(0,0,x,y);
};
