/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
#include <wx/menuitem.h>

#include "MFolder.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "ASMailFolder.h"
#include "MessageView.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"

#include "gui/wxMIds.h"
#include "MDialogs.h"
#include "MHelp.h"
#include "miscutil.h"            // for UpdateTitleAndStatusBars

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxPListCtrl)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
   EVT_CHAR              (wxFolderListCtrl::OnChar)
   EVT_LIST_ITEM_ACTIVATED(-1, wxFolderListCtrl::OnMouse)

#ifdef __WXGTK__
   EVT_MOTION (wxFolderListCtrl::OnMouseMove)
#endif // wxGTK

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

static const char *wxFLC_DEFAULT_SIZES = "80:80:80:80:80";

void wxFolderListCtrl::OnChar(wxKeyEvent& event)
{
   wxLogDebug("FolderListCtrl::OnChar, this=%p", this);

   if(! m_FolderView || ! m_FolderView->m_MessagePreview)
   {
      event.Skip();
      return; // nothing to do
   }

   if(! event.ControlDown())
   {
      long keyCode = event.KeyCode();
      if(keyCode == WXK_F1) // help
      {
         mApplication->Help(MH_FOLDER_VIEW_KEYBINDINGS, m_FolderView->GetWindow());
         return;
      }
      wxArrayInt selections;
      long nselected = m_FolderView->GetSelections(selections);
      // there is exactly one item with the focus on  it:
      long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
      long nMsgs = m_FolderView->GetFolder()->CountMessages();
      if(focused == -1 || focused >= nMsgs)
            return;
      // in this case we operate on the highlighted  message
      HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
      const HeaderInfo *hi =(*hil)[focused]; 
      UIdType focused_uid = hi->GetUId();
      if(nselected == 0 && hi)
         selections.Add(focused_uid);
      hil->DecRef();

      /** To    allow translations:
          Delete, Undelete, eXpunge, Copytofolder, Savetofile,
          Movetofolder, ReplyTo, Forward, Open, Print, Show Headers,
          View, Group reply (==followup)
      */
      const char keycodes_en[] = gettext_noop("DUXCSMRFOPHVG ");
      const char *keycodes = _(keycodes_en);

      int idx = 0;
      int key = toupper((int)keyCode);
      for(;keycodes[idx] && keycodes[idx] != key;idx++)
         ;

      // extra keys:
      if(key == '#') idx = 2; // # == expunge for VM compatibility
      if(key == WXK_DELETE) idx = 0; // delete
      switch(keycodes_en[idx])
      {
      case 'D':
         m_FolderView->GetTicketList()->Add(
            m_FolderView->GetFolder()->DeleteMessages(&selections, m_FolderView)); 
         break;
      case 'U':
         m_FolderView->GetTicketList()->Add(
            m_FolderView->GetFolder()->UnDeleteMessages(&selections, m_FolderView));
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
         m_FolderView->SaveMessagesToFolder(selections, true);
         break;
      case 'G':
      case 'R':
         m_FolderView->GetFolder()->ReplyMessages(
            &selections,
            GetFrame(this),
            (keycodes_en[idx] == 'G')?MailFolder::REPLY_FOLLOWUP:0,
            m_FolderView);
         break;
      case 'F':
         m_FolderView->GetFolder()->ForwardMessages(
            &selections, GetFrame(this), m_FolderView);
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
      case 'V':
         
         m_FolderView->PreviewMessage(focused_uid);
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
   //SetFocus();  //FIXME ugly wxGTK listctrl bug workaround
}


void wxFolderListCtrl::OnMouse(wxListEvent& event)
{
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   const HeaderInfo *hi = (*hil)[event.m_itemIndex];
   m_FolderView->PreviewMessage(hi->GetUId());
   hil->DecRef();
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   if(m_SelectionCallbacks)
   {
      HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
      const HeaderInfo *hi = (*hil)[event.m_itemIndex];
      m_FolderView->PreviewMessage(hi->GetUId());
      hil->DecRef();
   }
}


wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
{
   ProfileBase *p = fv->GetProfile();

   m_Parent = parent;
   m_FolderView = fv;
   m_Style = wxLC_REPORT;
   m_SelectionCallbacks = true;
   m_Initialised = false;

   if(READ_CONFIG(fv->GetProfile(), MP_PREVIEW_ON_SELECT))
      EnableSelectionCallbacks(true);
   else
      EnableSelectionCallbacks(false);

   int
      w = 500,
      h = 300;

   if(parent)
      parent->GetClientSize(&w,&h);

   wxString name;

   if(fv->GetProfile())
   {
      name = fv->GetProfile()->GetName();
      name << '/' << "FolderListCtrl";
      // If there is no entry for the listctrl, force one by
      // inheriting from NewMail folder.
      String entry = fv->GetProfile()->readEntry("FolderListCtrl","");
      if(! entry.Length() || entry == wxFLC_DEFAULT_SIZES)
      {
         String newMailFolder = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
         ProfileBase   *p = ProfileBase::CreateProfile(newMailFolder);
         p->SetPath("FolderView");
         entry = p->readEntry("FolderListCtrl","");
         if(entry.Length() && entry != wxFLC_DEFAULT_SIZES)
            fv->GetProfile()->writeEntry("FolderListCtrl", entry);
         p->DecRef();
      }
   }
   else
      name = "FolderListCtrl";

   // Without profile, name is relative to /Setting section, nothing we can do about
   wxPListCtrl::Create(name, parent, M_WXID_FOLDERVIEW_LISTCTRL,
                       wxDefaultPosition, wxSize(w,h), wxLC_REPORT | wxNO_BORDER);

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
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   const HeaderInfo *hi;
   while((item = GetNextItem(item,
                             wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED))
         != -1)
   {
      hi = (*hil)[item++];
      if(hi)
         selections.Add(hi->GetUId());
   }
   // If none is selected, use the focused entry
   if(selections.Count() == 0)
   {
      item = -1;
      item = GetNextItem(item, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);
      if(item != -1)
      {
         hi = (*hil)[item++];
         if(hi)
            selections.Add(hi->GetUId());
      }
   }
   hil->DecRef();
   return selections.Count();
}

void
wxFolderListCtrl::Clear(void)
{
   int x,y;
   GetClientSize(&x,&y);

   if(m_Initialised)
      SaveWidths(); // save widths of old columns

   ClearAll();

   if (m_Style & wxLC_REPORT)
   {
      for(int c = 0; c < WXFLC_NUMENTRIES; c++)
         for (int i = 0; i < WXFLC_NUMENTRIES; i++)
            if(m_columns[i] == c)
               InsertColumn( c, _(wxFLC_ColumnNames[i]), wxLIST_FORMAT_LEFT);
   }

   RestoreWidths();
   m_Initialised = true; // now we have proper columns set up
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

//   wxSafeYield();


   if(m_ASMailFolder)  // clean up old folder
   {
      // NB: the test for m_InDeletion is needed because of wxMSW bug which
      //     prevents us from showing a dialog box when called from dtor
      if ( !m_InDeletion )
      {
         wxString msg;
         msg.Printf(_("Mark all articles in\n%s\nas read?"),
                    m_ASMailFolder->GetName().c_str());

         if(m_NumOfMessages > 0 && m_ASMailFolder->GetType() == MF_NNTP
            && MDialog_YesNoDialog(msg,
                                   m_Parent,
                                   MDIALOG_YESNOTITLE,
                                   true,
                                   ProfileBase::FilterProfileName(m_Profile->GetName())))
         {
            // build sequence
            wxString sequence;
            HeaderInfoList *hil = m_ASMailFolder->GetHeaders();
            for(size_t i = 0; i < hil->Count(); i++)
            {
               sequence += strutil_ultoa((*hil)[i]->GetUId());
               sequence += ',';
            }
            hil->DecRef();
            sequence = sequence.substr(0,sequence.Length()-1); //strip off comma
            m_ASMailFolder->SetSequenceFlag(sequence, MailFolder::MSG_STAT_DELETED);
         }
      }

      // This little trick makes sure that we don't react to any final
      // events sent from the MailFolder destructor.
      MailFolder *mf = m_MailFolder;
      m_MailFolder = NULL;
      m_ASMailFolder->DecRef();
      m_ASMailFolder = NULL; // shouldn't be needed
      mf->DecRef();
   }

   SafeDecRef(m_Profile);

   m_NumOfMessages = 0; // At the beginning there was nothing.
   m_UpdateSemaphore = false;
   m_MailFolder = mf;
   m_ASMailFolder = mf ? ASMailFolder::Create(mf) : NULL;
   m_Profile = NULL;

   if(m_ASMailFolder)
   {
      m_Profile = ProfileBase::CreateProfile("FolderView",
                                             m_ASMailFolder ?
                                             m_ASMailFolder->GetProfile() :
                                             NULL);
      m_MessagePreview->SetParentProfile(m_Profile);
      m_MessagePreview->Clear(); // again, to reflect profile changes

      m_MailFolder->IncRef();  // make sure it doesn't go away
      m_folderName = m_ASMailFolder->GetName();

      if ( recreateFolderCtrl )
      {
         wxWindow *oldfolderctrl = m_FolderCtrl;
         m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
         m_SplitterWindow->ReplaceWindow(oldfolderctrl, m_FolderCtrl);
         delete oldfolderctrl;
      }

//      wxSafeYield(); // display the new folderctrl immediately
      Update();

      if(m_NumOfMessages > 0 && READ_CONFIG(m_Profile,MP_AUTOSHOW_FIRSTMESSAGE))
      {
         m_FolderCtrl->SetItemState(0,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
         // the callback will preview the (just) selected message
      }
#ifndef OS_WIN
      m_FolderCtrl->SetFocus(); // so we can react to keyboard events
#endif
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
   m_InDeletion = false;
   m_Parent = parent;
   m_MailFolder = NULL;
   m_ASMailFolder = NULL;
   m_TicketList =  ASTicketList::Create();
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
   m_DeleteSavedMessagesTicket = ASMailFolder::IllegalTicket;
}

wxFolderView::~wxFolderView()
{
   wxCHECK_RET( !m_InDeletion, "being deleted second time??" );

   m_TicketList->DecRef();
   m_InDeletion = true;
   SetFolder(NULL, FALSE);
}

void
wxFolderView::Update(HeaderInfoList *listing)
{
   if ( !m_ASMailFolder )
      return;

   long i;
   String   line;
   UIdType nsize;
   unsigned day, month, year;
   String dateFormat;
   int n;
   String status, sender, subject, date, size;
   bool selected;

   if(m_UpdateSemaphore == true)
      return; // don't call this code recursively
   m_UpdateSemaphore = true;

   if(listing == NULL)
   {
      listing = m_ASMailFolder->GetHeaders();

      if ( !listing )
      {
         m_UpdateSemaphore = false;

         return;
      }
   }
   else
      listing->IncRef();

   if(! listing)
      return;
   
   wxBeginBusyCursor();// wxSafeYield();

   n = listing->Count();

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
                         "contain exacty 3 %%u format specificators. Default "
                         "value '%s' will be used instead."),
                         dateFormat.c_str(), (unsigned int)MP_DATE_FMT_D);
         }

         dateFormat = MP_DATE_FMT_D;
      }
   }

   if(n < m_NumOfMessages)  // messages have been deleted, start over
   {
      m_FolderCtrl->Clear();
      m_NumOfMessages = 0;
   }

/*FIXME! Seems not to cause strange access problems.
  long focused = m_FolderCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   if(focused >= (long) m_ASMailFolder->CountMessages()) //FIXME
      focused = -1;
*/
   HeaderInfo const *hi;
   for(i = 0; i < n; i++)
   {
      hi = (*listing)[i];
      // FIXME vars are not initialised here!
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
/*FIXME!      if(i == focused)
         m_FolderCtrl->SetItemState( i, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED );
*/
   }

   String statusMsg;
   statusMsg.Printf(_("Folder '%s'"), m_folderName.c_str());

   UpdateTitleAndStatusBars(m_folderName, statusMsg, GetFrame(m_Parent),
                            m_MailFolder);

   m_NumOfMessages = n;
   wxEndBusyCursor(); //wxSafeYield();
//   m_FolderCtrl->SetFocus();

   listing->DecRef();
   m_UpdateSemaphore = false;
}


MailFolder *
wxFolderView::OpenFolder(String const &profilename)
{
   wxBeginBusyCursor(); //wxSafeYield(); // make changes visible

   MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE, profilename);
   SetFolder(mf);
   SafeDecRef(mf);
   m_ProfileName = profilename;

   wxEndBusyCursor();

   if ( !mf )
   {
      wxLogError(_("The folder '%s' could not be opened, please check "
                   "its settings."), profilename.c_str());
   }

   return mf;
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
   int n;
   wxArrayInt selections;

   switch(event.GetId())
   {
   case WXMENU_MSG_FIND:
   case WXMENU_MSG_TOGGLEHEADERS:
   case WXMENU_MSG_SHOWRAWTEXT:
   case WXMENU_EDIT_COPY:
      (void)m_MessagePreview->DoMenuCommand(event.GetId());
      break;
   case WXMENU_LAYOUT_LCLICK:
   case WXMENU_LAYOUT_RCLICK:
   case WXMENU_LAYOUT_DBLCLICK:
      m_MessagePreview->OnMouseEvent(event);
      break;

   case  WXMENU_MSG_EXPUNGE:
      m_ASMailFolder->ExpungeMessages();
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
      SaveMessagesToFolder(selections, true);
      break;
   case WXMENU_MSG_SAVE_TO_FILE:
      GetSelections(selections);
      SaveMessagesToFile(selections);
      break;
   case WXMENU_MSG_REPLY:
   case WXMENU_MSG_FOLLOWUP:
      GetSelections(selections);
      m_TicketList->Add(m_ASMailFolder->ReplyMessages(&selections, GetFrame(m_Parent),
                                            (event.GetId() == WXMENU_MSG_FOLLOWUP)
                                            ? MailFolder::REPLY_FOLLOWUP:0));
      break;
   case WXMENU_MSG_FORWARD:
      GetSelections(selections);
      m_TicketList->Add(m_ASMailFolder->ForwardMessages(&selections, GetFrame(m_Parent)));
      break;
   case WXMENU_MSG_UNDELETE:
      GetSelections(selections);
      m_TicketList->Add(m_ASMailFolder->UnDeleteMessages(&selections));
      break;
   case WXMENU_MSG_DELETE:
      GetSelections(selections);
      m_TicketList->Add(m_ASMailFolder->DeleteMessages(&selections));
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
   {
      bool tmp = m_FolderCtrl->EnableSelectionCallbacks(false);
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,TRUE);
      m_FolderCtrl->EnableSelectionCallbacks(tmp);
      break;
   }
   case WXMENU_MSG_DESELECTALL:
   {
      bool tmp = m_FolderCtrl->EnableSelectionCallbacks(false);
      for(n = 0; n < m_NumOfMessages; n++)
         m_FolderCtrl->Select(n,FALSE);
      m_FolderCtrl->EnableSelectionCallbacks(tmp);
      break;
   }
   case WXMENU_HELP_CONTEXT:
      mApplication->Help(MH_FOLDER_VIEW,GetWindow());
      break;

   case WXMENU_FILE_COMPOSE:
   {
      wxComposeView *composeView = wxComposeView::CreateNewMessage
                                   (
                                    GetFrame(m_Parent),
                                    GetProfile()
                                   );
      composeView->InitText();
      composeView->Show();
   }
   break;
   case WXMENU_FILE_POST:
   {
      wxComposeView *composeView = wxComposeView::CreateNewArticle
                                   (
                                    GetFrame(m_Parent),
                                    GetProfile()
                                   );
      composeView->InitText();
      composeView->Show();
   }
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
   // in case there is an event pending:
   if(m_FolderCtrl->GetItemCount() != (int)m_ASMailFolder->CountMessages())
   {
      Update();
      return 0; // ignore current selection, so user has to re-issue command
   }
   return m_FolderCtrl->GetSelections(selections);
}

void
wxFolderView::PreviewMessage(long uid)
{
   m_MessagePreview->ShowMessage(m_ASMailFolder, uid);
}

void
wxFolderView::OpenMessages(const wxArrayInt& selections)
{
   String title;
   
   int n = selections.Count();
   int i;
   for(i = 0; i < n; i++)
   {
      new wxMessageViewFrame(m_ASMailFolder, selections[i], this);
   }
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
         PreviewMessage(selections[i]);
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
         PreviewMessage(selections[i]);
         m_MessagePreview->PrintPreview();
      }
   }
}


void
wxFolderView::SaveMessagesToFolder(const wxArrayInt& selections, bool del)
{
   ASMailFolder::Ticket t =
      m_ASMailFolder->SaveMessagesToFolder(&selections,GetFrame(m_Parent), this);
   m_TicketList->Add(t);
   if(del)
      m_DeleteSavedMessagesTicket = t;
}

void
wxFolderView::SaveMessagesToFile(const wxArrayInt& selections)
{
   String msg;
   bool rc;

   rc = m_ASMailFolder->SaveMessagesToFile(&selections,
                                 GetFrame(m_Parent), this) != 0;
   if(rc)
     msg.Printf(_("%d messages saved"), selections.Count());
   else
      msg.Printf(_("Saving messages failed."));
   wxLogStatus(GetFrame(m_Parent), msg);
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
   }
   else // main frame splitter
   {
      SetFolder(NULL, TRUE); 
   }
}

/* This function gets called when the folder contents changed */
void
wxFolderView::OnFolderUpdateEvent(MEventFolderUpdateData &event)
{
   if(event.GetFolder() == m_MailFolder)
   {
      Update(event.GetHeaders());
   }
}

void
wxFolderView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   String msg;
   ASMailFolder::Result *result = event.GetResult();
   if(m_TicketList->Contains(result->GetTicket()))
   {
      ASSERT(result->GetUserData() == this);
      m_TicketList->Remove(result->GetTicket());
      
      switch(result->GetOperation())
      {
      case ASMailFolder::Op_SaveMessagesToFile:
         ASSERT(result->GetSequence());
         if( ((ASMailFolder::ResultInt *)result)->GetValue() )
            msg.Printf(_("Saved %lu messages."), (unsigned long)
                       result->GetSequence()->Count()); 
         else
            msg.Printf(_("Saving messages failed."));
         wxLogStatus(GetFrame(m_Parent), msg);
         ;
      case ASMailFolder::Op_SaveMessagesToFolder:
         ASSERT(result->GetSequence());
         if( ((ASMailFolder::ResultInt *)result)->GetValue() )
            msg.Printf(_("Copied %lu messages."), (unsigned long)
                       result->GetSequence()->Count()); 
         else
            msg.Printf(_("Copying messages failed."));
         wxLogStatus(GetFrame(m_Parent), msg);
         if(result->GetTicket() == m_DeleteSavedMessagesTicket)
         {
            m_TicketList->Add(m_ASMailFolder->DeleteMessages(
               result->GetSequence(),this)); 
         }
         break;
      // these cases don't have return values
      case ASMailFolder::Op_ReplyMessages:
      case ASMailFolder::Op_ForwardMessages:
      case ASMailFolder::Op_DeleteMessages:
      case ASMailFolder::Op_UnDeleteMessages:
         break;
      default:
         wxASSERT_MSG(0,"MEvent handling not implemented yet");
      }
   }
   result->DecRef();
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
   SetTitle(m_FolderView->GetFolder()->GetName());
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
   if(fv->OpenFolder(folderName))
      f->InternalCreate(fv,parent);
   else
   {
      delete fv;
      delete f;
      return NULL;
   }
   return f;
}

wxFolderViewFrame::wxFolderViewFrame(String const &name, wxMFrame *parent)
   : wxMFrame(name,parent)
{
   m_FolderView = NULL;

   // menu
   AddFileMenu();
   AddEditMenu();
   wxMenuItem *item = m_MenuBar->FindItem(WXMENU_EDIT_CUT);
   wxASSERT(item);
   item->Enable(FALSE); // no cut for viewer
   item = m_MenuBar->FindItem(WXMENU_EDIT_CUT);
   wxASSERT(item);
   item->Enable(FALSE); // no cut for viewer

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
   switch(id)
   {
      case WXMENU_EDIT_PREF: // edit folder profile
         MDialog_FolderProfile(this, m_FolderView->GetFullName());
         break;

      default:
         if( WXMENU_CONTAINS(MSG, id) || WXMENU_CONTAINS(LAYOUT, id)
             || id == WXMENU_HELP_CONTEXT
             || id == WXMENU_FILE_COMPOSE || id == WXMENU_FILE_POST
             || id == WXMENU_EDIT_CUT
             || id == WXMENU_EDIT_COPY
             || id == WXMENU_EDIT_PASTE)
            m_FolderView->OnCommandEvent(event);
         else
            wxMFrame::OnMenuCommand(id);
   }
}

void
wxFolderViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x, y;
   GetClientSize( &x, &y );

   if(m_FolderView)
      m_FolderView->SetSize(0,0,x,y);
}

IMPLEMENT_DYNAMIC_CLASS(wxFolderViewFrame, wxMFrame)
