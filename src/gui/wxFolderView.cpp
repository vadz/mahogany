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
#  include <wx/colour.h>
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


class wxFolderListCtrl : public wxPListCtrl
{
public:
   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   ~wxFolderListCtrl();
   void Clear(void);
   void SetEntry(long index,String const &status, String const &sender, String
                 const &subject, String const &date, String const
                 &size);

   void Select(long index, bool on=true)
      { SetItemState(index,on ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED); }

   /// if nofocused == true the focused entry will not be substituted
   /// for an empty list of selections
   int GetSelections(INTARRAY &selections, bool nofocused = false) const;
   UIdType GetFocusedUId(void) const;
   bool IsSelected(long index)
      { return GetItemState(index,wxLIST_STATE_SELECTED) != 0; }

   void OnSelected(wxListEvent& event);
   void OnChar( wxKeyEvent &event);
   void OnMouse(wxMouseEvent& event);
   void OnDoubleClick(wxMouseEvent & /* event */);
   void OnActivated(wxListEvent& event);
   void OnCommandEvent(wxCommandEvent& event)
      { m_FolderView->OnCommandEvent(event); }

   bool EnableSelectionCallbacks(bool enabledisable = true)
      {
         bool rc = m_SelectionCallbacks;
         m_SelectionCallbacks = enabledisable;
         return rc;
      }
   // this is a workaround for focus handling under GTK but it should not be
   // enabled under other platforms
#ifndef OS_WIN
   void OnMouseMove(wxMouseEvent &event)
      {
         if(m_FolderView->GetFocusFollowMode())
            SetFocus();
      }
#endif // wxGTK
   /// goto next unread message
   void SelectNextUnread(void);

   void ApplyOptions(const wxColour &fg, const wxColour &bg,
                     int fontFamily, int fontSize)
      {
         // the foregroundcolour is the one for the title line, we
         // leave it as it is SetForegroundColour( fg );
         // we want to use fg as the default item colour
         SetBackgroundColour( bg );
         SetFont( * new wxFont( fontSize, fontFamily,
                                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );
      }
protected:
   long m_Style;
   long m_NextIndex;
   /// parent window
   wxWindow *m_Parent;
   /// the folder view
   wxFolderView *m_FolderView;
   /// column numbers
   int m_columns[WXFLC_NUMENTRIES];
   /// which entry is in column 0?
   int m_firstColumn;
   /// do we want OnSelect() callbacks?
   bool m_SelectionCallbacks;
   /// do we preview a message on a single mouse click?
   bool m_PreviewOnSingleClick;
   /// have we been used previously?
   bool m_Initialised;
   /// the popup menu
   wxMenu *m_menu;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxPListCtrl)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
   EVT_CHAR              (wxFolderListCtrl::OnChar)
   EVT_LIST_ITEM_ACTIVATED(-1, wxFolderListCtrl::OnActivated)
   EVT_RIGHT_DOWN( wxFolderListCtrl::OnMouse)
   EVT_MENU(-1, wxFolderListCtrl::OnCommandEvent)
   EVT_LEFT_DCLICK(wxFolderListCtrl::OnDoubleClick)
#ifndef OS_WIN
   EVT_MOTION (wxFolderListCtrl::OnMouseMove)
#endif // wxGTK

END_EVENT_TABLE()

void wxFolderListCtrl::OnChar(wxKeyEvent& event)
{
   if(! m_FolderView || ! m_FolderView->m_MessagePreview
      || event.AltDown() || event.ControlDown()
      || ! m_FolderView->GetFolder()
      )
   {
      event.Skip();
      return; // nothing to do
   }
   m_FolderView->UpdateSelectionInfo();

   if(! event.ControlDown())
   {
      long keyCode = event.KeyCode();
      if(keyCode == WXK_F1) // help
      {
         mApplication->Help(MH_FOLDER_VIEW_KEYBINDINGS,
                            m_FolderView->GetWindow());
         event.Skip();
         return;
      }
      wxArrayInt selections;
      long nselected = m_FolderView->GetSelections(selections);
      // there is exactly one item with the focus on  it:
      long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
      long nMsgs = m_FolderView->GetFolder()->CountMessages();
      if(focused == -1 || focused >= nMsgs)
      {
         event.Skip();
         return;
      }
      // in this case we operate on the highlighted  message
      UIdType focused_uid = UID_ILLEGAL;
      HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
      if(hil)
      {
         const HeaderInfo *hi =(*hil)[focused];
         focused_uid = hi->GetUId();
         if(nselected == 0 && hi)
            selections.Add(focused_uid);
         hil->DecRef();
      }
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
               //new wxGTK semantics idx++;
            }
         }
         break;
      default:
         ;
      }
   }
   //SetFocus();  //FIXME ugly wxGTK listctrl bug workaround
   event.Skip();
}

void wxFolderListCtrl::OnMouse(wxMouseEvent& event)
{
// why doesn't this work?   wxASSERT(event.m_rightDown);
   PopupMenu(m_menu, event.GetX(), event.GetY());
}

void wxFolderListCtrl::OnDoubleClick(wxMouseEvent& /*event*/)
{
   // there is exactly one item with the focus on  it:
   long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   wxASSERT(focused != -1); // testing for wxGTK bug
   // in this case we operate on the highlighted  message
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   if(hil)
   {
      const HeaderInfo *hi =(*hil)[focused];
      UIdType focused_uid = hi->GetUId();
      if(m_PreviewOnSingleClick)
      {
         new wxMessageViewFrame(m_FolderView->GetFolder(), focused_uid, m_FolderView);
      }
      else
         m_FolderView->PreviewMessage(focused_uid);
      hil->DecRef();
   }
}

void wxFolderListCtrl::OnActivated(wxListEvent& event)
{

#if 0
   //FIXME: is this needed? I thought OnSelected() should do it?
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   const HeaderInfo *hi = (*hil)[event.m_itemIndex];
   m_FolderView->PreviewMessage(hi->GetUId());
   hil->DecRef();
#endif
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   // check if there is already a selected message, if so, don´t
   // update the message view:
   INTARRAY selections;
   if(GetSelections(selections, true) == 1 // we are the only one
      && m_SelectionCallbacks)
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

   m_PreviewOnSingleClick = READ_CONFIG(fv->GetProfile(),
                                        MP_PREVIEW_ON_SELECT) != 0;

   if(m_PreviewOnSingleClick)
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

   // Create popup menu:
#ifndef wxMENU_TEAROFF
   ///FIXME WXWIN-COMPATIBILITY
   m_menu = new wxMenu();
#else
   int style = 0;
   if(READ_APPCONFIG(MP_TEAROFF_MENUS) != 0)
      style = wxMENU_TEAROFF;
   m_menu = new wxMenu("", style);
#endif
   AppendToMenu(m_menu, WXMENU_MSG_BEGIN+1, WXMENU_MSG_END);

   Clear();
}

wxFolderListCtrl::~wxFolderListCtrl()
{
   delete m_menu;
}

int
wxFolderListCtrl::GetSelections(wxArrayInt &selections, bool nofocused) const
{
   selections.Empty();
   ASMailFolder *asmf = m_FolderView->GetFolder();

   if ( asmf )
   {
      long item = -1;
      HeaderInfoList *hil = asmf->GetHeaders();
      const HeaderInfo *hi = NULL;
      if(hil)
      {
         while((item = GetNextItem(item,
                                   wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED))
               != -1 && (unsigned long)item < hil->Count())
         {
            hi = (*hil)[item]; // no more ++, wxGTK changed
            if(hi)
               selections.Add(hi->GetUId());
         }
         // If none is selected, use the focused entry
         if(selections.Count() == 0 && ! nofocused)
         {
            item = -1;
            item = GetNextItem(item, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);
            if(item != -1 && (unsigned long)item < hil->Count())
            {
               hi = (*hil)[item]; // no more ++ wxGTK semantics changed
               if(hi)
                  selections.Add(hi->GetUId());
            }
         }
         hil->DecRef();
      }
   }

   return selections.Count();
}

UIdType
wxFolderListCtrl::GetFocusedUId(void) const
{
   UIdType uid = UID_ILLEGAL;
   ASMailFolder *asmf = m_FolderView->GetFolder();
   if ( asmf )
   {
      HeaderInfoList *hil = asmf->GetHeaders();
      ASSERT(hil);
      const HeaderInfo *hi = NULL;
      long item = -1;
      item = GetNextItem(item, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);
      if(item != -1 && item < (long) hil->Count() )
      {
         hi = (*hil)[item];
         if(hi)
            uid = hi->GetUId();
      }
      hil->DecRef();
   }

   return uid;
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
wxFolderListCtrl::SelectNextUnread()
{
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   if(! hil || hil->Count() == 0)
   {
      SafeDecRef(hil);
      return; // cannot do anything
   }
   UIdType focusedUId = GetFocusedUId();

   long idx = -1;
   bool foundFocused = false;
   while((idx = GetNextItem(idx)) != -1)
   {
      const HeaderInfo *hi = (*hil)[idx];
      if(foundFocused) // we are looking for the next unread now:
      {
         if((hi->GetStatus() & MailFolder::MSG_STAT_SEEN) == 0)
         {
            SetItemState(idx, wxLIST_STATE_FOCUSED,wxLIST_STATE_FOCUSED);
            SetItemState(idx, wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
            m_FolderView->PreviewMessage(hi->GetUId());
            m_FolderView->UpdateSelectionInfo();
            SafeDecRef(hil);
            return;
         }
      }
      if(hi->GetUId() == focusedUId)
            foundFocused = true;
      // semantics changed idx++;
   }
   SafeDecRef(hil);
}

void
wxFolderView::SetFolder(MailFolder *mf, bool recreateFolderCtrl)
{
   m_FocusedUId = UID_ILLEGAL;
   m_SelectedUIds.Empty();

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
         msg.Printf(_("Mark all articles in\n'%s'\nas read?"),
                    m_ASMailFolder->GetName().c_str());

         if(m_NumOfMessages > 0 && m_ASMailFolder->GetType() == MF_NNTP
            && MDialog_YesNoDialog(msg,
                                   m_Parent,
                                   MDIALOG_YESNOTITLE,
                                   true,
                                   ProfileBase::FilterProfileName(m_Profile->GetName())+"MarkRead"))
         {
            INTARRAY *seq = GetAllMessagesSequence(m_ASMailFolder);
            m_ASMailFolder->SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
            delete seq;
         }


         CheckExpungeDialog(m_ASMailFolder, m_Parent);
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

      // read in our profile settigns
      ReadProfileSettings(&m_settingsCurrent);

      m_MailFolder->IncRef();  // make sure it doesn't go away
      m_folderName = m_ASMailFolder->GetName();

      if ( recreateFolderCtrl )
      {
         wxWindow *oldfolderctrl = m_FolderCtrl;
         m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
         m_FolderCtrl->ApplyOptions( m_settingsCurrent.FgCol,
                                     m_settingsCurrent.BgCol,
                                     m_settingsCurrent.font,
                                     m_settingsCurrent.size);
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
      m_FocusFollowMode = READ_CONFIG(m_Profile, MP_FOCUS_FOLLOWSMOUSE);
      if(m_FocusFollowMode)
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
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);
   m_TicketList =  ASTicketList::Create();
   m_NumOfMessages = 0;
   m_Parent->GetClientSize(&x, &y);
   m_Profile = ProfileBase::CreateProfile("FolderView",NULL);
   m_SplitterWindow = new wxPSplitterWindow("FolderSplit", m_Parent, -1,
                                            wxDefaultPosition, wxSize(x,y),
                                            wxSP_3D|wxSP_BORDER);
   m_MessagePreview = new wxMessageView(this,m_SplitterWindow);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
   ReadProfileSettings(&m_settingsCurrent);
   m_FolderCtrl->ApplyOptions( m_settingsCurrent.FgCol,
                               m_settingsCurrent.BgCol,
                               m_settingsCurrent.font,
                               m_settingsCurrent.size);
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl, m_MessagePreview, y/3);
   m_SplitterWindow->SetMinimumPaneSize(0);
   m_SplitterWindow->SetFocus();
   m_DeleteSavedMessagesTicket = ILLEGAL_TICKET;
   m_FocusedUId = UID_ILLEGAL;
}

wxFolderView::~wxFolderView()
{
   wxCHECK_RET( !m_InDeletion, "being deleted second time??" );

   MEventManager::Deregister(m_regOptionsChange);
   DeregisterEvents();

   m_TicketList->DecRef();
   m_InDeletion = true;
   SetFolder(NULL, FALSE);
}

void
wxFolderView::ReadProfileSettings(AllProfileSettings *settings)
{
#   define NUM_FONTS 7
   static int wxFonts[NUM_FONTS] =
   {
      wxDEFAULT,
      wxDECORATIVE,
      wxROMAN,
      wxSCRIPT,
      wxSWISS,
      wxMODERN,
      wxTELETYPE
   };

#ifdef OS_WIN
   // MP_DATE_FMT contains '%' which are being (mis)interpreted as env var
   // expansion characters under Windows - prevent this from happening
   ProfileEnvVarSave noEnvVars(m_Profile);
#endif // OS_WIN

   settings->dateFormat = READ_CONFIG(m_Profile, MP_DATE_FMT);
   settings->dateGMT = READ_CONFIG(m_Profile, MP_DATE_GMT) != 0;
   GetColourByName(&settings->FgCol, READ_CONFIG(m_Profile,
                                                 MP_FVIEW_FGCOLOUR),
                   MP_FVIEW_FGCOLOUR_D);
   GetColourByName(&settings->BgCol, READ_CONFIG(m_Profile,
                                                 MP_FVIEW_BGCOLOUR),
                   MP_FVIEW_BGCOLOUR_D);
   settings->font = READ_CONFIG(m_Profile,MP_MVIEW_FONT);
   ASSERT(settings->font >= 0 && settings->font <= NUM_FONTS);
   settings->font = wxFonts[settings->font];
   settings->size = READ_CONFIG(m_Profile,MP_MVIEW_FONT_SIZE);


}

void
wxFolderView::OnOptionsChange(MEventOptionsChangeData& event)
{
   if ( !m_Profile )
   {
      // can't do anything useful (but can (and will) crash)
      return;
   }

   AllProfileSettings settingsNew;
   ReadProfileSettings(&settingsNew);

   if ( settingsNew == m_settingsCurrent )
   {
      // we don't care
      return;
   }

   m_settingsCurrent = settingsNew;

   switch ( event.GetChangeKind() )
   {
      case MEventOptionsChangeData::Apply:
      case MEventOptionsChangeData::Ok:
      case MEventOptionsChangeData::Cancel:
         // need to repopulate the list ctrl because the date format changed
         m_FolderCtrl->ApplyOptions( m_settingsCurrent.FgCol,
                                     m_settingsCurrent.BgCol,
                                     m_settingsCurrent.font,
                                     m_settingsCurrent.size);
         m_FolderCtrl->Clear();
         m_NumOfMessages = 0;
         Update();
         break;

      default:
         FAIL_MSG("unknown options change event");
   }
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
   int n;
   String status, sender, subject, size;
   bool selected;

   if(m_UpdateSemaphore == true)
      return; // don't call this code recursively
   m_UpdateSemaphore = true;

   if(! listing )
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

   wxBeginBusyCursor();

   n = listing->Count();

   if(n < m_NumOfMessages)  // messages have been deleted, start over
   {
      m_FolderCtrl->Clear();
      m_NumOfMessages = 0;
   }

   long focusedIndex, tmp = -1;
   focusedIndex = m_FolderCtrl->GetNextItem(tmp, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);
   bool foundFocus = false;
   HeaderInfo const *hi;
   for(i = 0; i < n; i++)
   {
      hi = (*listing)[i];
      subject = wxString("   ", hi->GetIndentation());
      subject << hi->GetSubject();
      nsize = day = month = year = 0;
      size = strutil_ultoa(nsize);
      selected = (m_SelectedUIds.Index(hi->GetUId()) != wxNOT_FOUND);
      m_FolderCtrl->SetEntry(i,
                             MailFolder::ConvertMessageStatusToString(hi->GetStatus()),
                             hi->GetFrom(),
                             subject,
                             strutil_ftime(hi->GetDate(),
                                           m_settingsCurrent.dateFormat,
                                           m_settingsCurrent.dateGMT),
                             strutil_ultoa(hi->GetSize()));
      m_FolderCtrl->Select(i,selected);
      m_FolderCtrl->SetItemState(i, wxLIST_STATE_FOCUSED,
                                 (hi->GetUId() == m_FocusedUId)?
                                 wxLIST_STATE_FOCUSED : 0);
#if 0
      // this only affects the first column, why?
      wxListItem info;
      info.m_itemId = i;
      m_FolderCtrl->GetItem(info);
      info.m_colour = & m_settingsCurrent.FgCol;
      m_FolderCtrl->SetItem(info);
#endif
      if(hi->GetUId() == m_FocusedUId)
         foundFocus = true;
   }
   if(! foundFocus) // old focused UId is gone, so we use the list
      // index instead
      if(focusedIndex != -1)
         m_FolderCtrl->SetItemState(focusedIndex,
                                    wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);
   String statusMsg;
   statusMsg.Printf(_("Folder '%s'"), m_folderName.c_str());

   UpdateTitleAndStatusBars(m_folderName, statusMsg, GetFrame(m_Parent),
                            m_MailFolder);

   m_NumOfMessages = n;
   wxEndBusyCursor();
   listing->DecRef();
   // the previously focused uid might be gone now:
   //PreviewMessage(m_FolderCtrl->GetFocusedUId());
   m_UpdateSemaphore = false;
}


MailFolder *
wxFolderView::OpenFolder(String const &profilename)
{
   wxBeginBusyCursor();

   MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE, profilename);
   SetFolder(mf);
   SafeDecRef(mf);
   m_ProfileName = profilename;

   wxEndBusyCursor();

   if ( !mf && (mApplication->GetLastError() != M_ERROR_CANCEL) )
   {
      // FIXME propose to show the folder properties dialog right here
      wxLogError(_("The folder '%s' could not be opened, please check "
                   "its settings."), profilename.c_str());
   }

   return mf;
}

void
wxFolderView::SearchMessages(void)
{
   SearchCriterium criterium;

   ConfigureSearchMessages(&criterium,GetProfile(),NULL);
   Ticket t = m_ASMailFolder->SearchMessages(&criterium, this);
   m_TicketList->Add(t);
}

void
wxFolderView::UpdateSelectionInfo(void)
{
   // record this for the later Update:
   m_FolderCtrl->GetSelections(m_SelectedUIds, true);
   m_FocusedUId = m_FolderCtrl->GetFocusedUId();
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
   int n;
   INTARRAY selections;

   UpdateSelectionInfo();

   switch(event.GetId())
   {
   case WXMENU_MSG_SEARCH:
      SearchMessages();
      break;
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
   case WXMENU_MSG_NEXT_UNREAD:
      m_FolderCtrl->SelectNextUnread();
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
   if(! m_ASMailFolder)
      return 0;

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
   Ticket t =
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
         break;

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

      case ASMailFolder::Op_SearchMessages:
         ASSERT(result->GetSequence());
         if( ((ASMailFolder::ResultInt *)result)->GetValue() )
         {
            INTARRAY *ia = result->GetSequence();
            msg.Printf(_("Found %lu messages."), (unsigned long)
                       ia->Count());
            bool tmp = m_FolderCtrl->EnableSelectionCallbacks(false);
            for(unsigned long n = 0; n < ia->Count(); n++)
               m_FolderCtrl->Select((*ia)[n]);
            m_FolderCtrl->EnableSelectionCallbacks(tmp);

         }
         else
            msg.Printf(_("No matching messages found."));
         wxLogStatus(GetFrame(m_Parent), msg);
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
   AddToolbarButtons(CreateToolBar(), WXFRAME_FOLDER);
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

   // no cut/paste for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);
   menuBar->Enable(WXMENU_EDIT_CUT, FALSE);

   AddMessageMenu();

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
