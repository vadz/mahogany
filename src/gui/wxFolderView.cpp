/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
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
#  include "Profile.h"

#  include <wx/dynarray.h>
#endif
#include <ctype.h>
#include <XFace.h>

#include <wx/file.h>
#include <wx/persctrl.h>

#include "MFolder.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "MessageView.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"

#include "gui/wxIconManager.h"
#include "gui/wxMIds.h"
#include "MDialogs.h"
#include "MHelp.h"

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxPListCtrl)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
   EVT_CHAR              (wxFolderListCtrl::OnKey)
END_EVENT_TABLE()

#define   LCFIX ((wxFolderListCtrl *)this)->

static const char *wxFLC_ColumnNames[] =
{
   gettext_noop("Status"),
   gettext_noop("Date"),
   gettext_noop("Size"),
   gettext_noop("From"),
   gettext_noop("Subject")
};

void wxFolderListCtrl::OnKey(wxKeyEvent& event)
{
   if(! m_FolderView || ! m_FolderView->m_MessagePreview)
      return; // nothing to do
   
   if(! event.ControlDown())
   {
      long keyCode = event.KeyCode();
      wxArrayInt selections;
      long nselected = m_FolderView->GetSelections(selections);
      // there is exactly one item with the focus on  it:
      long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
      wxASSERT(focused != -1); // should never happen

      // in this case we operate on the highlighted  message
      if(nselected == 0)
         selections.Add(focused);

      /** To    allow translations:
          Delete, Undelete, eXpunge, Copytofolder, Savetofile,
          Movetofolder, ReplyTo, Forward, Open, Print, Show Headers
      */
      const char keycodes_en[] = gettext_noop("DUXCSMRFOPH ");
      const char *keycodes = _(keycodes_en);
      
      int idx = 0;
      int key = toupper((int)keyCode);
      for(;keycodes[idx] && keycodes[idx] != key;idx++)
         ;

      // extra keys:
      if(key == '#') idx = 2; // # == expunge for VM compatibility
   
      switch(keycodes_en[idx])
      {
      case 'D':
         m_FolderView->DeleteMessages(selections);
         break;
      case 'U':
         m_FolderView->UnDeleteMessages(selections);
         break;
      case 'X':
         m_FolderView->GetFolder()->ExpungeMessages();
         break;
      case 'C':
         m_FolderView->SaveMessagesToFolder(selections);
         break;
      case 'S':
         m_FolderView->SaveMessagesToFile(selections);
         break;
      case 'M':
         m_FolderView->SaveMessagesToFolder(selections);
         m_FolderView->DeleteMessages(selections);
         break;
      case 'R':
         m_FolderView->ReplyMessages(selections);
         break;
      case 'F':
         m_FolderView->ForwardMessages(selections);
         break;
      case 'O':
         m_FolderView->OpenMessages(selections);
         break;
      case 'P':
         m_FolderView->PrintMessages(selections);
         break;
      case 'H':
         m_FolderView->m_MessagePreview->DoMenuCommand(WXMENU_MSG_TOGGLEHEADERS);
         break;
      case ' ':
         // If shift is not used, deselect all items before having
         // wxListCtrl selects this one.
         if(!event.ShiftDown())
         {
            long idx = -1;
            while((idx = GetNextItem(idx, wxLIST_NEXT_ALL,
                                     wxLIST_STATE_SELECTED)) != -1)
            {
               if(idx != focused)  // allow us to toggle the focused item
                  SetItemState(idx,0,wxLIST_STATE_SELECTED);
               idx++;
            }
         }
         event.Skip();
         break;
      default:
         event.Skip();
      }
   }
   else
      event.Skip();
   SetFocus();  //FIXME ugly wxGTK listctrl bug workaround
}


void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   m_FolderView->PreviewMessage(event.m_itemIndex);
}


wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
{
   ProfileBase *p = fv->GetProfile();

   m_Parent = parent;
   m_FolderView = fv;
   m_Style = wxLC_REPORT;

   int
      w = 500,
      h = 300;

   if(parent)
      parent->GetClientSize(&w,&h);

   wxString name = fv->GetFullName();
   name.Replace("/", "_");
   if ( name.IsEmpty() )
      name = "FolderView";
   wxPListCtrl::Create(name, parent, -1,
                       wxDefaultPosition, wxSize(w,h), wxLC_REPORT);

   m_columns[WXFLC_STATUS] = READ_CONFIG(p,MP_FLC_STATUSCOL);
   m_columns[WXFLC_DATE] = READ_CONFIG(p,MP_FLC_DATECOL);
   m_columns[WXFLC_SUBJECT] = READ_CONFIG(p,MP_FLC_SUBJECTCOL);
   m_columns[WXFLC_SIZE] = READ_CONFIG(p,MP_FLC_SIZECOL);
   m_columns[WXFLC_FROM] = READ_CONFIG(p,MP_FLC_FROMCOL);

   for(int i = 0; i < WXFLC_NUMENTRIES; i++)
   {
      if(m_columns[i] == 0)
      {
         m_firstColumn = i;
         break;
      }
   }

   Clear();
}

int
wxFolderListCtrl::GetSelections(wxArrayInt &selections) const
{
   long item = -1;
   while((item = GetNextItem(item,
                             wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED))
         != -1) 
      selections.Add(item++);
   return selections.Count();
}

void
wxFolderListCtrl::Clear(void)
{
   int x,y;
   GetClientSize(&x,&y);

   SaveWidths();

   DeleteAllItems();
   DeleteAllColumns();

   if (m_Style & wxLC_REPORT)
   {
      for(int c = 0; c < WXFLC_NUMENTRIES; c++)
         for (int i = 0; i < WXFLC_NUMENTRIES; i++)
            if(m_columns[i] == c)
               InsertColumn( c, _(wxFLC_ColumnNames[i]), wxLIST_FORMAT_LEFT);
   }

   RestoreWidths();
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

void
wxFolderView::SetFolder(MailFolder *mf, bool recreateFolderCtrl)
{
   // this shows what's happening:
   m_MessagePreview->Clear();
   if ( recreateFolderCtrl )
      m_FolderCtrl->Clear();
   wxYield();
   
   if(m_MailFolder)  // clean up old folder
   {
      m_timer->Stop();
      delete m_timer;
//      m_MailFolder->RegisterView(this,false);
      
      // These messages are no longer recent as we've seen their headers.
      if(m_NumOfMessages > 0)
         m_MailFolder->SetSequenceFlag("1:*", MailFolder::MSG_STAT_RECENT, false);
      m_MailFolder->DecRef();
   }

   SafeDecRef(m_Profile); 

   m_NumOfMessages = 0; // At the beginning there was nothing.
   m_UpdateSemaphore = false;
   m_MailFolder = mf;
   m_Profile = NULL;
   m_timer = NULL;

   if(m_MailFolder)
   {
      m_Profile = ProfileBase::CreateProfile("FolderView",
                                             m_MailFolder ?
                                             m_MailFolder->GetProfile() :
                                             NULL);
      m_MessagePreview->SetParentProfile(m_Profile);
      m_MessagePreview->Clear(); // again, to reflect profile changes

      m_MailFolder->IncRef();  // make sure it doesn't go away
      m_folderName = m_MailFolder->GetName();
      m_timer = new wxFVTimer(m_MailFolder);
//      m_MailFolder->RegisterView(this);

      if ( recreateFolderCtrl )
      {
         wxWindow *oldfolderctrl = m_FolderCtrl;
         m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
         m_SplitterWindow->ReplaceWindow(oldfolderctrl, m_FolderCtrl);
         delete oldfolderctrl;
      }

      wxYield(); // display the new folderctrl immediately
      Update();

      if(m_NumOfMessages > 0 && READ_CONFIG(m_Profile,MP_AUTOSHOW_FIRSTMESSAGE))
      {
         m_FolderCtrl->SetItemState(0,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
         // the callback will preview the (just) selected message
      }
      m_FolderCtrl->SetFocus(); // so we can react to keyboard events
   }
}

wxFolderView *
wxFolderView::Create(MWindow *parent)
{
   wxCHECK_MSG(parent, NULL, "NULL parent frame in wxFolderView ctor");
   wxFolderView *fv = new wxFolderView(parent);
   return fv;
}

wxFolderView::wxFolderView(wxWindow *parent)
{
   int x,y;
   m_Parent = parent;
   m_MailFolder = NULL;
   m_NumOfMessages = 0;
   m_Parent->GetClientSize(&x, &y);
   m_Profile = ProfileBase::CreateProfile("FolderView",NULL);
   m_SplitterWindow = new wxPSplitterWindow("FolderSplit", m_Parent, -1,
                                            wxDefaultPosition, wxSize(x,y),
                                            wxSP_3D|wxSP_BORDER);
   m_MessagePreview = new wxMessageView(this,m_SplitterWindow);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl, m_MessagePreview, y/3);
   m_SplitterWindow->SetMinimumPaneSize(0);
   m_SplitterWindow->SetFocus();

}

void
wxFolderView::Update(void)
{
   if(! m_MailFolder)
      return;

   long i;
   String   line;
   unsigned long nsize;
   unsigned day, month, year;
   String dateFormat;
   int n;
   String status, sender, subject, date, size;
   bool selected;

   if(m_UpdateSemaphore == true)
      return; // don't call this code recursively
   m_UpdateSemaphore = true;

   wxBeginBusyCursor(); wxYield();
   n = m_MailFolder->CountMessages();

   // mildly annoying, but have to do it in order to prevent the generation of
   // error messages about failed env var expansion (this string contains '%'
   // which introduce env vars under Windows)
   {
      ProfileEnvVarSave suspend(mApplication->GetProfile(),false);
      dateFormat = READ_APPCONFIG(MP_DATE_FMT);

      // should have _exactly_ 3 format specificators, otherwise can't call
      // Printf()!
      String specs = strutil_extract_formatspec(dateFormat);
      if ( specs != "ddd" )
      {
         static bool s_bErrorMessageGiven = false;
         if ( s_bErrorMessageGiven )
         {
            // don't give it each time - annoying...
            s_bErrorMessageGiven = true;

            wxLogError(_("Invalid value '%s' for the date format: it should "
                         "contain exactyly 3 %%u format specificators. Default "
                         "value '%s' will be used instead."),
                         dateFormat.c_str(), MP_DATE_FMT_D);
         }

         dateFormat = MP_DATE_FMT_D;
      }
   }

   if(n < m_NumOfMessages)  // messages have been deleted, start over
   {
      m_FolderCtrl->Clear();
      m_NumOfMessages = 0;
   }

   HeaderInfo const *hi;
   i = 0;
   for(hi = m_MailFolder->GetFirstHeaderInfo(); hi != NULL;
       hi = m_MailFolder->GetNextHeaderInfo(hi))
   {
      // FIXME vars are not inited here!
      nsize = day = month = year = 0;

      date.Printf(dateFormat, day, month, year);
      size = strutil_ultoa(nsize);

      selected = (i < m_NumOfMessages) ? m_FolderCtrl->IsSelected(i) : false;

      m_FolderCtrl->SetEntry(i,
                             MailFolder::ConvertMessageStatusToString(hi->GetStatus()),
                             hi->GetFrom(),
                             hi->GetSubject(),
                             hi->GetDate(),
                             strutil_ultoa(hi->GetSize())
         );
      m_FolderCtrl->Select(i,selected);
      i++;
   }
   m_NumOfMessages = n;
   wxEndBusyCursor(); wxYield();
   m_UpdateSemaphore = false;
}


void
wxFolderView::OpenFolder(String const &profilename)
{
   wxBeginBusyCursor();
   wxYield(); // make changes visible

   MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE, profilename);
   SetFolder(mf);
   SafeDecRef(mf);
   m_ProfileName = profilename;
   wxEndBusyCursor();
}

wxFolderView::~wxFolderView()
{
   SetFolder(NULL, FALSE);
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
   int n;
   wxArrayInt selections;

   switch(event.GetId())
   {
   case WXMENU_LAYOUT_LCLICK:
   case WXMENU_LAYOUT_RCLICK:
   case WXMENU_LAYOUT_DBLCLICK:
      m_MessagePreview->OnMouseEvent(event);
      break;

   case  WXMENU_MSG_EXPUNGE:
      m_MailFolder->ExpungeMessages();
      Update();
      wxLogStatus(GetFrame(m_Parent), _("Deleted messages were expunged"));
      break;

   case WXMENU_MSG_OPEN:
      GetSelections(selections);
      OpenMessages(selections);
      break;
   case WXMENU_MSG_SAVE_TO_FOLDER:
      GetSelections(selections);
      SaveMessagesToFolder(selections);
      break;
   case WXMENU_MSG_MOVE_TO_FOLDER:
      GetSelections(selections);
      SaveMessagesToFolder(selections);
      DeleteMessages(selections);  //FIXME: we should test for save being successful
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
   case WXMENU_MSG_PRINT:
      GetSelections(selections);
      PrintMessages(selections);
      break;
#ifdef USE_PS_PRINTING
   case WXMENU_MSG_PRINT_PS:
      //FIXME GetSelections(selections);
      //FIXME PrintMessages(selections);
      break;
#endif
   case WXMENU_MSG_PRINT_PREVIEW:
      GetSelections(selections);
      PrintPreviewMessages(selections);
      break;
   case WXMENU_MSG_SELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,TRUE);
      break;
   case WXMENU_MSG_DESELECTALL:
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,FALSE);
      break;
   case WXMENU_HELP_CONTEXT:
      mApplication->Help(MH_FOLDER_VIEW,GetWindow());
      break;
   case WXMENU_MSG_TOGGLEHEADERS:
   case WXMENU_MSG_SHOWRAWTEXT:
      (void)m_MessagePreview->DoMenuCommand(event.GetId());
      break;

   default:
      wxFAIL_MSG("wxFolderView::OnMenuCommand() called with illegal id.");
   }
}

bool
wxFolderView::HasSelection() const
{
   return m_FolderCtrl->GetSelectedItemCount() != 0;
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
      mptr = m_MailFolder->GetMessage(selections[i]+1);
      title = mptr->Subject() + " - " + mptr->From();
      mv = GLOBAL_NEW wxMessageViewFrame(m_MailFolder,selections[i]+1,
                                         this);
      mv->SetTitle(title);
      SafeDecRef(mptr);
   }
}

void
wxFolderView::DeleteMessages(const wxArrayInt& selections)
{
   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
      m_MailFolder->DeleteMessage(selections[i]+1);
   Update();

   wxLogStatus(GetFrame(m_Parent), _("%d messages deleted"), n);
}

void
wxFolderView::UnDeleteMessages(const wxArrayInt& selections)
{
   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
      m_MailFolder->UnDeleteMessage(selections[i]+1);
   Update();

   wxLogStatus(GetFrame(m_Parent), _("%d messages undeleted"), n);
}

void
wxFolderView::PrintMessages(const wxArrayInt& selections)
{
   int n = selections.Count();

   if(n == 1)
      m_MessagePreview->Print();
   else
   {
      int i;
      for(i = 0; i < n; i++)
      {
         PreviewMessage(i);
         m_MessagePreview->Print();
      }
   }
}

void
wxFolderView::PrintPreviewMessages(const wxArrayInt& selections)
{
   int n = selections.Count();

   if(n == 1)
      m_MessagePreview->PrintPreview();
   else
   {
      int i;
      for(i = 0; i < n; i++)
      {
         PreviewMessage(i);
         m_MessagePreview->PrintPreview();
      }
   }
}

void
wxFolderView::SaveMessages(const wxArrayInt& selections, String const &folderName)
{
   int i;

   MailFolder   *mf;

   if(strutil_isempty(folderName))
      return;
   Message *msg;
   
   int n = selections.Count();
   mf = MailFolder::OpenFolder(MF_PROFILE,folderName);
   if(! mf)
   {
      wxString msg;
      msg << _("Cannot open folder '") << folderName << "'.";
      wxLogError(msg);
      return;
   }
   for(i = 0; i < n; i++)
   {
      msg = m_MailFolder->GetMessage(selections[i]+1);
      mf->AppendMessage(*msg);
      SafeDecRef(msg);
   }
   mf->Ping(); // update any views
   mf->DecRef();
   wxLogStatus(GetFrame(m_Parent), _("%d messages saved"), n);
}

void
wxFolderView::SaveMessagesToFolder(const wxArrayInt& selections)
{
   MFolder *folder = MDialog_FolderChoose(m_Parent);
   if ( folder )
   {
      // +1 is apparently needed to skip the '/'
      SaveMessages(selections, folder->GetFullName().c_str() + 1);

      folder->DecRef();
   }
   else
   {
      wxLogStatus(GetFrame(m_Parent), _("Cancelled"));
   }
}

void
wxFolderView::SaveMessagesToFile(const wxArrayInt& selections)
{
   String filename = wxPFileSelector("MsgSave",
                                     _("Choose file to save message to"),
                                     NULL, NULL, NULL,
                                     _("All files (*.*)|*.*"),
                                     wxSAVE | wxOVERWRITE_PROMPT,
                                     m_Parent);

   if ( !filename )
   {
      wxLogStatus(GetFrame(m_Parent), _("Cancelled"));
   }
   else
   {
      // truncate the file
      wxFile file(filename, wxFile::write);
      file.Close();

      SaveMessages(selections,filename);
   }
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
   prefix = READ_CONFIG(m_Profile, MP_REPLY_MSGPREFIX);
   for(i = 0; i < n; i++)
   {
      cv = GLOBAL_NEW wxComposeView(_("Reply"),m_Parent, m_Profile);
      str = "";
      msg = m_MailFolder->GetMessage(selections[i]+1);
      np = msg->CountParts();
      for(p = 0; p < np; p++)
      {
         if(msg->GetPartType(p) == Message::MSG_TYPETEXT)
         {
            str = msg->GetPartContent(p);
            cptr = str.c_str();
            str2 = prefix;
            while(*cptr)
            {
               if(*cptr == '\r')
               {
                  cptr++;
                  continue;
               }
               str2 += *cptr;
               if(*cptr == '\n' && *(cptr+1))
               {
                  str2 += prefix;
               }
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
      cv->SetSubject(READ_CONFIG(GetProfile(), MP_REPLY_PREFIX)
                     + msg->Subject());
      SafeDecRef(msg);
      m_MailFolder->SetMessageFlag(i, MailFolder::MSG_STAT_ANSWERED, true);
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
   prefix = READ_CONFIG(GetProfile(), MP_REPLY_MSGPREFIX);
   for(i = 0; i < n; i++)
   {
      str = "";
      msg = m_MailFolder->GetMessage(selections[i]+1);
      cv = GLOBAL_NEW wxComposeView(_("Forward"),m_Parent,
                                    GetProfile());
      cv->SetSubject(READ_CONFIG(GetProfile(), MP_FORWARD_PREFIX)
                                 + msg->Subject());

      msg->WriteToString(str);
      cv->InsertData(strutil_strdup(str), str.Length(),
                     "MESSAGE/RFC822");
      SafeDecRef(msg);
   }

   wxLogStatus(GetFrame(m_Parent), _("%d messages forwarded"), n);
}

void
wxFolderView::SetSize(const int x, const int y, const int width, int
                      height)
{
//   wxPanel::SetSize(x,y,width,height);
   if(m_SplitterWindow)
      m_SplitterWindow->SetSize( x, y, width, height );
}

void wxFolderView::OnFolderDeleteEvent(const String& folderName)
{
   if ( folderName == m_folderName )
   {
      if ( m_Parent->IsKindOf(CLASSINFO(wxFrame)) )
      {
         wxLogMessage(_("Closing folder '%s' because the underlying mail "
                        "folder was deleted."), m_folderName.c_str());

         m_Parent->Close();
      }
      else
      {
         FAIL_MSG("I thought folder view parent must be a frame.");
      }
   }
}

/* This function gets called when the folder contents changed */
void
wxFolderView::OnFolderUpdateEvent(MEventFolderUpdateData &event)
{
   if(event.GetFolder() == m_MailFolder)
   {
      Update();
   }
}


/*------------------------------------------------------------------------
 * wxFolderViewFrame
 *-----------------------------------------------------------------------*/
BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
   EVT_SIZE(    wxFolderViewFrame::OnSize)
   EVT_MENU(-1,    wxFolderViewFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxFolderViewFrame::OnCommandEvent)

   // only enable message operations if the selection is not empty
   // (the range should contain _only_ these operations!)
   { wxEVT_UPDATE_UI, WXMENU_MSG_OPEN, WXMENU_MSG_UNDELETE,
     (wxObjectEventFunction)(wxEventFunction)(wxUpdateUIEventFunction)
     &(wxFolderViewFrame::OnUpdateUI), NULL },
END_EVENT_TABLE()

void
wxFolderViewFrame::InternalCreate(wxFolderView *fv, wxMFrame * /* parent */)
{
   m_FolderView = fv;
   SetTitle(String("M: " + m_FolderView->GetFolder()->GetName()));
   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_FOLDER);
   Show(true);
}

wxFolderViewFrame *
wxFolderViewFrame::Create(MailFolder *mf, wxMFrame *parent)

{
   CHECK( mf, NULL, "NULL folder passed to wxFolderViewFrame::Create" );

   wxFolderViewFrame *f = new wxFolderViewFrame(mf->GetName(),parent);
   wxFolderView *fv = wxFolderView::Create(f);
   fv->SetFolder(mf);
   f->InternalCreate(fv,parent);
   return f;
}

wxFolderViewFrame *
wxFolderViewFrame::Create(const String &folderName, wxMFrame *parent)
{
   wxFolderViewFrame *f = new wxFolderViewFrame(folderName, parent);
   wxFolderView *fv = wxFolderView::Create(f);
   fv->OpenFolder(folderName);
   f->InternalCreate(fv,parent);
   return f;
}

wxFolderViewFrame::wxFolderViewFrame(String const &name, wxMFrame *parent)
   : wxMFrame(name,parent)
{
   m_FolderView = NULL;

   SetTitle(String("M: " + name));
   // menu
   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   SetMenuBar(m_MenuBar);
   // status bar
   CreateStatusBar();
}

wxFolderViewFrame::~wxFolderViewFrame()
{
   if(m_FolderView) delete m_FolderView;
}

void
wxFolderViewFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
   if(m_FolderView)
      event.Enable(m_FolderView->HasSelection());
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
   if( WXMENU_CONTAINS(MSG, id) || WXMENU_CONTAINS(LAYOUT, id)
       || id == WXMENU_HELP_CONTEXT)
      m_FolderView->OnCommandEvent(event);
   else
      wxMFrame::OnMenuCommand(id);
}

void
wxFolderViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x, y;
   GetClientSize( &x, &y );

   if(m_FolderView)
      m_FolderView->SetSize(0,0,x,y);
}
