/*-*- c++ -*-********************************************************
 * wxFolderView.cc: a window displaying a mail folder               *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

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
#include "XFace.h"

#include <wx/file.h>
#include "wx/persctrl.h"
#include <wx/menuitem.h>
#include <wx/dnd.h>
#include <wx/fontmap.h>
#include <wx/encconv.h>

#include "MFolder.h"
#include "Mdnd.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "ASMailFolder.h"
#include "MessageView.h"
#include "TemplateDialog.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"
#include "gui/wxFolderMenu.h"
#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder()

#include "gui/wxMIds.h"
#include "MDialogs.h"
#include "MHelp.h"
#include "miscutil.h"            // for UpdateTitleAndStatusBars

#define   LCFIX ((wxFolderListCtrl *)this)->

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const char *wxFLC_ColumnNames[] =
{
   gettext_noop("Status"),
   gettext_noop("Date"),
   gettext_noop("Size"),
   gettext_noop("From"),
   gettext_noop("Subject")
};

// the profile key where the columns widths are stored
#define FOLDER_LISTCTRL_WIDTHS "FolderListCtrl"

// the default widths for the columns
static const char *FOLDER_LISTCTRL_WIDTHS_D = "60:300:200:80:80";

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxFolderListCtrl : public wxListCtrl
{
public:
   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   virtual ~wxFolderListCtrl();

   // create the columns using order in m_columns and widths from profile
   void CreateColumns();

   // return the string containing ':' separated columns widths
   String GetWidths() const;

   // deletes all items from the control and recreates columns if necessary
   void Clear(void);

   // sets or adds an entry
   void SetEntry(long index,
                 String const &status,
                 String const &sender,
                 String const &subject,
                 String const &date,
                 String const &size);

   void Select(long index, bool on=true)
      { SetItemState(index,on ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED); }

   /// if nofocused == true the focused entry will not be substituted
   /// for an empty list of selections
   int GetSelections(UIdArray &selections, bool nofocused = false) const;
   UIdType GetFocusedUId(void) const;
   bool IsSelected(long index)
      { return GetItemState(index,wxLIST_STATE_SELECTED) != 0; }

   void OnSelected(wxListEvent& event);
   void OnColumnClick(wxListEvent& event);
   void OnChar( wxKeyEvent &event);
   void OnRightClick(wxMouseEvent& event);
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
   void OnMouseMove(wxMouseEvent &event);

   /// goto next unread message
   void SelectNextUnread(void);

   /// change the options governing our appearance
   void ApplyOptions(const wxColour &fg, const wxColour &bg,
                     int fontFamily, int fontSize,
                     int columns[WXFLC_NUMENTRIES]);

   /// set m_PreviewOnSingleClick flag
   void SetPreviewOnSingleClick(bool flag)
      {
         m_PreviewOnSingleClick = flag;
         if(m_PreviewOnSingleClick)
            EnableSelectionCallbacks(true);
         else
            EnableSelectionCallbacks(false);
      }

   // for wxFolderView
   wxMenu *GetFolderMenu() const { return m_menuFolders; }

   void MoveFocus(int newFocus)
      {
         if(newFocus != -1)
         {
            SetItemState(newFocus, wxLIST_STATE_FOCUSED,
                         wxLIST_STATE_FOCUSED);
            EnsureVisible(newFocus);
            m_FolderView->UpdateSelectionInfo();
         }
      }
protected:
   long m_NextIndex;
   /// parent window
   wxWindow *m_Parent;
   /// the folder view
   wxFolderView *m_FolderView;
   /// the profile used for storing columns widths
   Profile *m_profile;
   /// column numbers
   int m_columns[WXFLC_NUMENTRIES];
   /// do we want OnSelect() callbacks?
   bool m_SelectionCallbacks;
   /// do we preview a message on a single mouse click?
   bool m_PreviewOnSingleClick;
   /// did we create the list ctrl columns?
   bool m_Initialised;
   /// the popup menu
   wxMenu *m_menu;
   /// and the folder submenu for it
   wxMenu *m_menuFolders;

   DECLARE_EVENT_TABLE()
};

// a helper class for dnd
class FolderViewMessagesDropWhere : public MMessagesDropWhere
{
public:
   FolderViewMessagesDropWhere(wxFolderView *view)
   {
      m_folderView = view;
   }

   virtual MFolder *GetFolder(wxCoord x, wxCoord y) const
   {
      // we don't even use the position of the drop
      return MFolder::Get(m_folderView->GetFullName());
   }

   virtual void Refresh()
   {
      m_folderView->Update();
   }

private:
   wxFolderView *m_folderView;
};

// ----------------------------------------------------------------------------
// priate functions
// ----------------------------------------------------------------------------

// return the n-th shown column (WXFLC_NONE if no more columns)
static wxFolderListCtrlFields GetColumnByIndex(const int *columns, size_t n)
{
   size_t col;
   for ( col = 0; col < WXFLC_NUMENTRIES; col++ )
   {
      if ( columns[col] == (int)n )
         break;
   }

   // WXFLC_NONE == WXFLC_NUMENTRIES so the return value is always correct
   return (wxFolderListCtrlFields)col;
}

// read the columns info from profile into provided array
static void ReadColumnsInfo(Profile *profile, int columns[WXFLC_NUMENTRIES])
{
   columns[WXFLC_STATUS]   = READ_CONFIG(profile, MP_FLC_STATUSCOL);
   columns[WXFLC_DATE]     = READ_CONFIG(profile, MP_FLC_DATECOL);
   columns[WXFLC_SUBJECT]  = READ_CONFIG(profile, MP_FLC_SUBJECTCOL);
   columns[WXFLC_SIZE]     = READ_CONFIG(profile, MP_FLC_SIZECOL);
   columns[WXFLC_FROM]     = READ_CONFIG(profile, MP_FLC_FROMCOL);
}

// write the columns to profile
static void WriteColumnsInfo(Profile *profile, const int columns[WXFLC_NUMENTRIES])
{
   profile->writeEntry(MP_FLC_STATUSCOL, columns[WXFLC_STATUS]);
   profile->writeEntry(MP_FLC_DATECOL,   columns[WXFLC_DATE]);
   profile->writeEntry(MP_FLC_SUBJECTCOL,columns[WXFLC_SUBJECT]);
   profile->writeEntry(MP_FLC_SIZECOL,   columns[WXFLC_SIZE]);
   profile->writeEntry(MP_FLC_FROMCOL,   columns[WXFLC_FROM]);
}

// return the name of the n-th columns
static inline wxString GetColumnName(size_t n)
{
   return _(wxFLC_ColumnNames[n]);
}

// return the columns index from name
static wxFolderListCtrlFields GetColumnByName(const wxString& name)
{
   size_t n;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      if ( name == GetColumnName(n) )
         break;
   }

   return (wxFolderListCtrlFields)n;
}

static MessageSortOrder SortOrderFromCol(wxFolderListCtrlFields col)
{
   switch ( col )
   {
      case WXFLC_DATE:
         return MSO_DATE;

      case WXFLC_FROM:
         return MSO_AUTHOR;

      case WXFLC_SUBJECT:
         return MSO_SUBJECT;

      default:
      case WXFLC_NUMENTRIES:
         wxFAIL_MSG( "invalid column" );

      case WXFLC_SIZE:
      case WXFLC_STATUS:
         // we don't support sorting by size or status [yet]
         return MSO_NONE;
   }
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFolderListCtrl, wxListCtrl)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)
   EVT_CHAR              (wxFolderListCtrl::OnChar)
   EVT_LIST_ITEM_ACTIVATED(-1, wxFolderListCtrl::OnActivated)
   EVT_RIGHT_DOWN( wxFolderListCtrl::OnRightClick)
   EVT_MENU(-1, wxFolderListCtrl::OnCommandEvent)
   EVT_LEFT_DCLICK(wxFolderListCtrl::OnDoubleClick)
   EVT_MOTION (wxFolderListCtrl::OnMouseMove)

   EVT_LIST_COL_CLICK(-1, wxFolderListCtrl::OnColumnClick)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderListCtrl
// ----------------------------------------------------------------------------

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
      UIdArray selections;
      long nselected = m_FolderView->GetSelections(selections);
      // there is exactly one item with the focus on  it:
      long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
      long nMsgs = m_FolderView->GetFolder()->CountMessages();
      if(focused == -1 || focused >= nMsgs)
      {
         event.Skip();
         return;
      }
      long newFocus = -1;
      // in this case we operate on the highlighted  message
      UIdType focused_uid = UID_ILLEGAL;
      if(nselected == 0)
      {
         HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
         if(hil)
         {
            if(focused > 0 && focused < (long) hil->Count())
            focused_uid = (*hil)[focused]->GetUId();
            selections.Add(focused_uid);
            hil->DecRef();
         }
         newFocus = -1;
      }
      else
         newFocus = (focused < nMsgs-1) ? focused + 1 : focused;

      /** To    allow translations:
          Delete, Undelete, eXpunge, Copytofolder, Savetofile,
          Movetofolder, ReplyTo, Forward, Open, Print, Show Headers,
          View, Group reply (==followup)
      */
      const char keycodes_en[] = gettext_noop("DUXCSMRFOPHG ");
      const char *keycodes = _(keycodes_en);

      int idx = 0;
      int key = toupper((int)keyCode);
      for(;keycodes[idx] && keycodes[idx] != key;idx++)
         ;

      // extra keys:
      if(key == '#') idx = 2; // # == expunge for VM compatibility
      if(key == WXK_DELETE) idx = 0; // delete
      // scroll up within the message viewer:
      if(key == WXK_BACK)
      {
         if(m_FolderView->GetPreviewUId() == focused_uid)
            m_FolderView->m_MessagePreview->PageUp();
         event.Skip();
         return;
      }
      switch(keycodes_en[idx])
      {
      case 'D':
         m_FolderView->DeleteOrTrashMessages(selections);
         // only move on if we mark as deleted, for trash usage,
         // selection remains the same:
         if(READ_APPCONFIG(MP_USE_TRASH_FOLDER) == FALSE)
            MoveFocus(newFocus);
         else
            m_FolderView->UpdateSelectionInfo();
         break;
      case 'U':
         m_FolderView->GetTicketList()->Add(
            m_FolderView->GetFolder()->UnDeleteMessages(&selections, m_FolderView));
         MoveFocus(newFocus);
         break;
      case 'X':
            m_FolderView->GetFolder()->ExpungeMessages();
         break;
      case 'C':
         m_FolderView->SaveMessagesToFolder(selections);
         MoveFocus(newFocus);
         break;
      case 'S':
         m_FolderView->SaveMessagesToFile(selections);
         MoveFocus(newFocus);
         break;
      case 'M': // move = copy + delete
         m_FolderView->SaveMessagesToFolder(selections, NULL, true);
         if(READ_APPCONFIG(MP_USE_TRASH_FOLDER) == FALSE)
            MoveFocus(newFocus);
         else
            m_FolderView->UpdateSelectionInfo();
         break;
      case 'G':
      case 'R':
         m_FolderView->GetFolder()->ReplyMessages(
            &selections,
            (keycodes_en[idx] == 'G')?MailFolder::REPLY_FOLLOWUP:0,
            GetFrame(this),
            m_FolderView);
         MoveFocus(newFocus);
         break;
      case 'F':
         m_FolderView->GetFolder()->ForwardMessages(
            &selections, MailFolder::Params(), GetFrame(this), m_FolderView);
         MoveFocus(newFocus);
         break;
      case 'O':
         m_FolderView->OpenMessages(selections);
         MoveFocus(newFocus);
         break;
      case 'P':
         m_FolderView->PrintMessages(selections);
         MoveFocus(newFocus);
         break;
      case 'H':
         m_FolderView->m_MessagePreview->DoMenuCommand(WXMENU_MSG_TOGGLEHEADERS);
         break;
      case ' ': // mark:
         // should just be the default behaviour
         /* I would like it to mark the current entry and then
            move to the next one, but it doesn't work.
            I don't know what funny things wxWindows does here,
            but for now I leave it as it is.

          event.Skip();
          MoveFocus(newFocus);
          return;
         */
         ;
      }
   }
   event.Skip();
}

void wxFolderListCtrl::OnMouseMove(wxMouseEvent &event)
{
   // this is a workaround for focus handling under GTK but it should not be
   // enabled under other platforms
#ifndef OS_WIN
   if(m_FolderView->GetFocusFollowMode() && (FindFocus() != this))
      SetFocus();
#endif // wxGTK

   // start the drag and drop operation
   if ( event.Dragging() )
   {
      if ( m_FolderView->DragAndDropMessages() )
      {
         // skipping event.Skip() below
         return;
      }
   }

   event.Skip();
}

void wxFolderListCtrl::OnRightClick(wxMouseEvent& event)
{
   // create popup menu if not done yet
   if (m_menu) delete m_menu;

   m_menuFolders = wxFolderMenu::Create();

   m_menu = new wxMenu;
   m_menu->Append(WXMENU_POPUP_FOLDER_MENU, _("&Quick move"), m_menuFolders);
   m_menu->AppendSeparator();

   {
      static const int popupMenuEntries[] =
      {
         WXMENU_MSG_QUICK_FILTER,
         WXMENU_SEPARATOR,
         WXMENU_MSG_OPEN,
         WXMENU_SEPARATOR,
         WXMENU_MSG_REPLY,
         WXMENU_MSG_REPLY_WITH_TEMPLATE,
         WXMENU_MSG_FOLLOWUP,
         WXMENU_MSG_FOLLOWUP_WITH_TEMPLATE,
         WXMENU_MSG_FORWARD,
         WXMENU_MSG_FORWARD_WITH_TEMPLATE,
         WXMENU_SEPARATOR,
         WXMENU_MSG_FILTER,
         WXMENU_MSG_PRINT,
         WXMENU_MSG_SAVE_TO_FILE,
         WXMENU_MSG_SAVE_TO_FOLDER,
         WXMENU_MSG_MOVE_TO_FOLDER,
         WXMENU_MSG_DELETE,
         WXMENU_MSG_UNDELETE,
         WXMENU_SEPARATOR,
         WXMENU_MSG_SHOWRAWTEXT,
      };

      for ( size_t n = 0; n < WXSIZEOF(popupMenuEntries); n++ )
      {
         int id = popupMenuEntries[n];
         AppendToMenu(m_menu, id);
      }
   }

   PopupMenu(m_menu, event.GetX(), event.GetY());

   wxFolderMenu::Remove(m_menuFolders);
   m_menuFolders = NULL;
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
      if ( hi )
      {
         UIdType focused_uid = hi->GetUId();
         if(m_PreviewOnSingleClick)
         {
            new wxMessageViewFrame(m_FolderView->GetFolder(),
                                   focused_uid, m_FolderView);
         }
         else
            m_FolderView->PreviewMessage(focused_uid);
      }
      hil->DecRef();
   }
}

void wxFolderListCtrl::OnActivated(wxListEvent& event)
{
   // called by RETURN press
   HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
   ASSERT_RET(hil);
   const HeaderInfo *hi = (*hil)[event.m_itemIndex];
   ASSERT_RET(hi);
   if(m_FolderView->GetPreviewUId() == hi->GetUId())
      m_FolderView->m_MessagePreview->PageDown();
   else
      m_FolderView->PreviewMessage(hi->GetUId());
   hil->DecRef();
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   // check if there is already a selected message, if so, don´t
   // update the message view:
   UIdArray selections;
   if(GetSelections(selections, true) == 1 // we are the only one
      && m_SelectionCallbacks)
   {
      HeaderInfoList *hil = m_FolderView->GetFolder()->GetHeaders();
      const HeaderInfo *hi = (*hil)[event.m_itemIndex];
      m_FolderView->UpdateSelectionInfo();
      if(m_PreviewOnSingleClick)
        m_FolderView->PreviewMessage(hi->GetUId());
      hil->DecRef();
   }
}

void wxFolderListCtrl::OnColumnClick(wxListEvent& event)
{
   // get the column which was clicked
   wxFolderListCtrlFields col = GetColumnByIndex(m_columns, event.GetColumn());
   wxCHECK_RET( col != WXFLC_NONE, "should have a valid column" );

   // we're going to change the sort order either for this profile in the
   // advanced mode but the global sort order in the novice mode: otherwise,
   // it would be surprizing for the novice user as he'd see one thing in the
   // options dialog and another thing on the screen (the sorting page is not
   // shown in the folder dialog in this mode)
   Profile *profile = READ_APPCONFIG(MP_USERLEVEL) == M_USERLEVEL_NOVICE
                        ? mApplication->GetProfile()
                        : m_FolderView->GetProfile();

   // sort by this column: if we already do this, then toggle the sort
   // direction
   long sortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   if ( profile != m_FolderView->GetProfile() )
   {
      if ( READ_CONFIG(m_FolderView->GetProfile(), MP_MSGS_SORTBY) != sortOrder )
      {
         // as we're going to change the global one but this profiles settings
         // will override it!
         wxLogDebug("Changing the sort order won't take effect.");
      }
   }

   wxArrayInt sortOrders = SplitSortOrder(sortOrder);

   MessageSortOrder orderCol = SortOrderFromCol(col);
   if ( orderCol == MSO_NONE )
   {
      // we can't sort by this column
      return;
   }

   size_t count = sortOrders.GetCount();
   if ( count == 0 )
   {
      // no sort rules at all, just add this one
      sortOrders.Add(orderCol);
   }
   else // we already have some sort rules
   {
      // if we are already sorting by this column (directly or not), reverse
      // the sort order
      if ( sortOrders[0u] == orderCol )
      {
         sortOrders[0u] = orderCol + 1;
      }
      else if ( sortOrders[0u] == orderCol + 1 )
      {
         sortOrders[0u] = orderCol;
      }
      else
      {
         // now sort by this column and push back the old sort orders, also
         // remove the duplicates: we don't need to sort twice on the same
         // column
         wxArrayInt sortOrders2;
         sortOrders2.Add(orderCol);
         for ( size_t n = 0; n < count; n++ )
         {
            if ( sortOrders[n] != orderCol && sortOrders[n] != orderCol + 1 )
               sortOrders2.Add(sortOrders[n]);
         }

         sortOrders = sortOrders2;
      }
   }

   // save the new sort order and update everything
   wxLogStatus(GetFrame(this), _("Now sorting by %s%s"),
               GetColumnName(col).c_str(),
               sortOrders[0u] == orderCol ? "" :  _(" (reverse)"));

   sortOrder = BuildSortOrder(sortOrders);
   profile->writeEntry(MP_MSGS_SORTBY, sortOrder);

   MEventManager::Send(new MEventOptionsChangeData
                           (
                            profile,
                            MEventOptionsChangeData::Ok
                           ));
}

wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
{
   m_Parent = parent;
   m_profile = fv->GetProfile();

   m_PreviewOnSingleClick = false;

   m_profile->IncRef(); // we wish to keep it until dtor
   m_FolderView = fv;
   m_SelectionCallbacks = true;
   m_Initialised = false;
   m_menu =
   m_menuFolders = NULL;

   int
      w = 500,
      h = 300;

   if(parent)
      parent->GetClientSize(&w,&h);

   Create(parent, M_WXID_FOLDERVIEW_LISTCTRL,
          wxDefaultPosition, wxSize(w,h),
          wxLC_REPORT | wxNO_BORDER);

   ReadColumnsInfo(m_profile, m_columns);

   CreateColumns();

   // create a drop target for dropping messages on us
   new MMessagesDropTarget(new FolderViewMessagesDropWhere(m_FolderView), this);
}

wxFolderListCtrl::~wxFolderListCtrl()
{
   // save the widths
   String str = GetWidths();
   if ( !str.empty() )
      m_profile->writeEntry(FOLDER_LISTCTRL_WIDTHS, str);

   m_profile->DecRef();

   delete m_menu;
}

int
wxFolderListCtrl::GetSelections(UIdArray &selections, bool nofocused) const
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
      if(! hil) // if there is no listing
         return uid;
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

void wxFolderListCtrl::CreateColumns()
{
   // delete existing columns
   ClearAll();

   // read the widths array and complete it with -1 if necessary
   String widthsString = READ_CONFIG(m_profile, FOLDER_LISTCTRL_WIDTHS);
   wxArrayString widths = strutil_restore_array(':', widthsString);
   size_t n;
   for ( n = widths.GetCount(); n < WXFLC_NUMENTRIES; n++ )
   {
      widths.Add("-1");
   }

   // add the new ones
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      wxFolderListCtrlFields col = GetColumnByIndex(m_columns, n);
      if ( col == WXFLC_NONE )
         break;

      long width;
      if ( !widths[n].ToLong(&width) )
         width = -1;

      InsertColumn(n, GetColumnName(col), wxLIST_FORMAT_LEFT, width);
   }
}

String wxFolderListCtrl::GetWidths() const
{
   String str;

   int count = GetColumnCount();
   for ( int col = 0; col < count; col++ )
   {
      if ( !str.IsEmpty() )
         str << ':';

      str << GetColumnWidth(col);
   }

   return str;
}

void wxFolderListCtrl::ApplyOptions(const wxColour &fg, const wxColour &bg,
                                    int fontFamily, int fontSize,
                                    int columns[WXFLC_NUMENTRIES])
{
   // foreground colour is the colour of the items text, and so we use
   // SetTextColour() and not SetForegroundColour() which would be wrong
   SetTextColour( fg );
   SetBackgroundColour( bg );

   SetFont(wxFont(fontSize, fontFamily, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

   if ( memcmp(m_columns, columns, sizeof(m_columns)) != 0 )
   {
      // the control must be recreated if the order of columns changed
      memcpy(m_columns, columns, sizeof(m_columns));

      CreateColumns();
   }
}

void
wxFolderListCtrl::Clear(void)
{
   DeleteAllItems();
}

void
wxFolderListCtrl::SetEntry(long index,
                           String const &status,
                           String const &sender,
                           String const &subject,
                           String const &date,
                           String const &size)
{
   if ( index >= GetItemCount() )
   {
      // the item label is the value of the first column
      String label;
      switch ( GetColumnByIndex(m_columns, 0) )
      {
         case WXFLC_STATUS:   label = status;  break;
         case WXFLC_FROM:     label = sender;  break;
         case WXFLC_DATE:     label = date;    break;
         case WXFLC_SIZE:     label = size;    break;
         case WXFLC_SUBJECT:  label = subject; break;

         default:
            wxFAIL_MSG( "unknown column" );
      }

      InsertItem(index, label);
   }

   if ( m_columns[WXFLC_STATUS] != -1 )
      SetItem(index, m_columns[WXFLC_STATUS], status);
   if ( m_columns[WXFLC_FROM] != -1 )
      SetItem(index, m_columns[WXFLC_FROM], sender);
   if ( m_columns[WXFLC_DATE] != -1 )
      SetItem(index, m_columns[WXFLC_DATE], date);
   if ( m_columns[WXFLC_SIZE] != -1 )
      SetItem(index, m_columns[WXFLC_SIZE], size);
   if ( m_columns[WXFLC_SUBJECT] != -1 )
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

   if(focusedUId == UID_ILLEGAL)
      return;

   long idx = -1;
   bool foundFocused = false;
   bool failedOnce = false;
   while(1)
   {
      idx = GetNextItem(idx);
      if(idx == -1)
      {
         if(failedOnce)
            break; // we really haven't found one
         failedOnce = true;
         // else: we didn't find an unread *below*, so try above:
         foundFocused = false;
         idx = -1;
         idx = GetNextItem(idx);
      }

      const HeaderInfo *hi = (*hil)[idx];
      if(hi->GetUId() == focusedUId)
      {
         foundFocused = true;
         continue; // we don't want the current one
      }

      if(
         ((! failedOnce) && foundFocused)// we are looking for the next unread now:
         || (failedOnce && ! foundFocused) // now we look for the once
         // before the focused one
         )
      {
         if((hi->GetStatus() & MailFolder::MSG_STAT_SEEN) == 0)
         {
            SetItemState(idx, wxLIST_STATE_FOCUSED,wxLIST_STATE_FOCUSED);
            if(m_PreviewOnSingleClick)
               m_FolderView->PreviewMessage(hi->GetUId());
            m_FolderView->UpdateSelectionInfo();
            SafeDecRef(hil);
            return;
         }
      }
   }
   SafeDecRef(hil);
}


// ----------------------------------------------------------------------------
// wxFolderView
// ----------------------------------------------------------------------------

void
wxFolderView::SetFolder(MailFolder *mf, bool recreateFolderCtrl)
{
   // If we don't check, we could get called recursively from within a
   // wxYield()...
   ASSERT_MSG(m_SetFolderSemaphore == false, "DEBUG: SetFolder() called recursively, shouldn't happen.");
//   if(m_SetFolderSemaphore)
//      return;
   m_SetFolderSemaphore = true;


   m_FocusedUId = UID_ILLEGAL;
   m_SelectedUIds.Empty();

   // this shows what's happening:
   m_MessagePreview->Clear();
   m_previewUId = UID_ILLEGAL;
   if ( recreateFolderCtrl )
      m_FolderCtrl->Clear();


   SafeIncRef(mf);

   if(m_ASMailFolder)  // clean up old folder
   {
      // NB: the test for m_InDeletion is needed because of wxMSW bug which
      //     prevents us from showing a dialog box when called from dtor
      if ( !m_InDeletion )
      {
         wxString msg;
         msg.Printf(_("Mark all articles in\n'%s'\nas read?"),
                    m_ASMailFolder->GetName().c_str());

         if(m_NumOfMessages > 0
            && (m_ASMailFolder->GetType() == MF_NNTP ||
                m_ASMailFolder->GetType() == MF_NEWS)
            && MDialog_YesNoDialog(msg,
                                   m_Parent,
                                   MDIALOG_YESNOTITLE,
                                   true,
                                   Profile::FilterProfileName(m_Profile->GetName())+"MarkRead"))
         {
            UIdArray *seq = GetAllMessagesSequence(m_ASMailFolder);
            m_ASMailFolder->SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
            delete seq;
         }
         CheckExpungeDialog(m_ASMailFolder, m_Parent);
      }

      // This little trick makes sure that we don't react to any final
      // events sent from the MailFolder destructor.
      MailFolder *mf2 = m_MailFolder;
      m_MailFolder = NULL;
      m_ASMailFolder->DecRef();
      m_ASMailFolder = NULL; // shouldn't be needed
      mf2->DecRef();
   }

   SafeDecRef(m_Profile);

   m_NumOfMessages = 0; // At the beginning there was nothing.
   m_UpdateSemaphore = false;
   m_MailFolder = mf;
   m_ASMailFolder = mf ? ASMailFolder::Create(mf) : NULL;
   m_Profile = NULL;
   SafeDecRef(mf); // now held by m_ASMailFfolder

   if(m_ASMailFolder)
   {
/*      m_Profile = Profile::CreateProfile("FolderView",
                                             m_ASMailFolder ?
                                             m_ASMailFolder->GetProfile() :
                                             NULL);
*/
      m_Profile = m_ASMailFolder->GetProfile();
      if(m_Profile)
         m_Profile->IncRef();
      else
         m_Profile = Profile::CreateEmptyProfile(mApplication->GetProfile());

      m_MessagePreview->SetParentProfile(m_Profile);
      m_MessagePreview->Clear(); // again, to reflect profile changes
      m_previewUId = UID_ILLEGAL;

      // read in our profile settigns
      ReadProfileSettings(&m_settings);

      m_MailFolder->IncRef();  // make sure it doesn't go away
      m_folderName = m_ASMailFolder->GetName();

      if ( recreateFolderCtrl )
      {
         wxWindow *oldfolderctrl = m_FolderCtrl;
         m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow, this);
         m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                                     m_settings.BgCol,
                                     m_settings.font,
                                     m_settings.size,
                                     m_settings.columns);
         bool previewOnSingleClick = READ_CONFIG(GetProfile(),
                                             MP_PREVIEW_ON_SELECT) != 0;
         m_FolderCtrl->SetPreviewOnSingleClick(previewOnSingleClick);

         m_SplitterWindow->ReplaceWindow(oldfolderctrl, m_FolderCtrl);
         delete oldfolderctrl;
      }

      Update();

      if(m_NumOfMessages > 0 && READ_CONFIG(m_Profile,MP_AUTOSHOW_FIRSTMESSAGE))
      {
         m_FolderCtrl->SetItemState(0,wxLIST_STATE_FOCUSED,wxLIST_STATE_FOCUSED);
         HeaderInfoList *hil = m_ASMailFolder->GetHeaders();
         if(hil && hil->Count() > 0)
            PreviewMessage ((*hil)[0]->GetUId());
         SafeDecRef(hil);
      }
#ifndef OS_WIN
      m_FocusFollowMode = READ_CONFIG(m_Profile, MP_FOCUS_FOLLOWSMOUSE);
      if(m_FocusFollowMode && wxWindow::FindFocus() != m_FolderCtrl)
         m_FolderCtrl->SetFocus(); // so we can react to keyboard events
#endif
   }
   EnableMMenu(MMenu_Message, m_FolderCtrl, (m_ASMailFolder != NULL) );
   m_SetFolderSemaphore = false;
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
   m_UpdateSemaphore = false;
   m_SetFolderSemaphore = false;
   m_Parent = parent;
   m_MailFolder = NULL;
   m_ASMailFolder = NULL;
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);

   m_TicketList =  ASTicketList::Create();
   m_TicketsToDeleteList = ASTicketList::Create();
   m_TicketsDroppedList = NULL;

   m_NumOfMessages = 0;
   m_previewUId = UID_ILLEGAL;
   m_Parent->GetClientSize(&x, &y);
   m_Profile = Profile::CreateEmptyProfile(mApplication->GetProfile());
   m_SplitterWindow = new wxPSplitterWindow("FolderSplit", m_Parent, -1,
                                            wxDefaultPosition, wxSize(x,y),
                                            wxSP_3D|wxSP_BORDER);
   m_MessagePreview = new wxMessageView(this,m_SplitterWindow);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow,this);
   ReadProfileSettings(&m_settings);
   bool   previewOnSingleClick = READ_CONFIG(GetProfile(),
                                             MP_PREVIEW_ON_SELECT) != 0;

   m_FolderCtrl->SetPreviewOnSingleClick(previewOnSingleClick);
   m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                               m_settings.BgCol,
                               m_settings.font,
                               m_settings.size,
                               m_settings.columns);
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl, m_MessagePreview, y/3);
   m_SplitterWindow->SetMinimumPaneSize(0);
   m_SplitterWindow->SetFocus();

   m_FocusedUId = UID_ILLEGAL;
}

wxFolderView::~wxFolderView()
{
   wxCHECK_RET( !m_InDeletion, "being deleted second time??" );

   MEventManager::Deregister(m_regOptionsChange);
   DeregisterEvents();

   m_TicketList->DecRef();
   m_TicketsToDeleteList->DecRef();

   SafeDecRef(m_TicketsDroppedList);

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
   GetColourByName(&settings->BgCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_BGCOLOUR),
                   MP_FVIEW_BGCOLOUR_D);
   GetColourByName(&settings->NewCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_NEWCOLOUR),
                   MP_FVIEW_NEWCOLOUR_D);
   GetColourByName(&settings->RecentCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_RECENTCOLOUR),
                   MP_FVIEW_RECENTCOLOUR_D);
   GetColourByName(&settings->DeletedCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_DELETEDCOLOUR),
                   MP_FVIEW_DELETEDCOLOUR_D);
   GetColourByName(&settings->UnreadCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_UNREADCOLOUR),
                   MP_FVIEW_UNREADCOLOUR_D);
   settings->font = READ_CONFIG(m_Profile,MP_FVIEW_FONT);
   ASSERT(settings->font >= 0 && settings->font <= NUM_FONTS);
   settings->font = wxFonts[settings->font];
   settings->size = READ_CONFIG(m_Profile, MP_FVIEW_FONT_SIZE);
   settings->senderOnlyNames = READ_CONFIG(m_Profile, MP_FVIEW_NAMES_ONLY) != 0;

   settings->replaceFromWithTo = READ_CONFIG(m_Profile, MP_FVIEW_FROM_REPLACE) != 0;
   if ( settings->replaceFromWithTo )
   {
      String returnAddrs = READ_CONFIG(m_Profile, MP_FROM_REPLACE_ADDRESSES);
      if ( returnAddrs == MP_FROM_REPLACE_ADDRESSES_D )
      {
         // the default for this option is just the return address
         returnAddrs = READ_CONFIG(m_Profile, MP_FROM_ADDRESS);
      }

      settings->returnAddresses = strutil_restore_array(':', returnAddrs);
   }

   ReadColumnsInfo(m_Profile, settings->columns);
}

void
wxFolderView::OnOptionsChange(MEventOptionsChangeData& event)
{
   Profile *profile = GetProfile();
   if ( !profile )
   {
      // we're empty
      return;
   }

   bool previewOnSingleClick = READ_CONFIG(profile, MP_PREVIEW_ON_SELECT) != 0;
   m_FolderCtrl->SetPreviewOnSingleClick(previewOnSingleClick);

   AllProfileSettings settings;
   ReadProfileSettings(&settings);
   if ( settings != m_settings )
   {
      // need to repopulate the list ctrl because its appearance changed
      m_settings = settings;

      m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                                  m_settings.BgCol,
                                  m_settings.font,
                                  m_settings.size,
                                  m_settings.columns);
      m_FolderCtrl->Clear();
      m_NumOfMessages = 0;
      Update();
   }
}


void
wxFolderView::SetEntry(HeaderInfoList *listing, size_t index)
{
   ASSERT_RET(listing);
   ASSERT_RET(index < listing->Count());

   String   line;
   UIdType nsize;
   unsigned day, month, year;
   String sender, subject, size;
   bool selected;

   HeaderInfo const *hi = (*listing)[index];

   subject = wxString(' ', 3*hi->GetIndentation()) + hi->GetSubject();
   nsize = day = month = year = 0;
   size = strutil_ultoa(nsize);
   selected = (m_SelectedUIds.Index(hi->GetUId()) != wxNOT_FOUND);

   sender = hi->GetFrom();

   // optionally replace the "From" with "To: someone" for messages sent by
   // the user himself
   bool replaceFromWithTo = false;
   if ( m_settings.replaceFromWithTo )
   {
      const wxArrayString& adrs = m_settings.returnAddresses;
      size_t nAdrCount = adrs.GetCount();
      for ( size_t nAdr = 0; !replaceFromWithTo && (nAdr < nAdrCount); nAdr++ )
      {
         replaceFromWithTo = Message::CompareAddresses(sender, adrs[nAdr]);
      }
   }

   if ( replaceFromWithTo )
   {
      sender.clear();
      sender << _("To: ") << hi->GetTo();
   }

   // optionally leave only the name part of the address
   if ( m_settings.senderOnlyNames )
   {
      sender = Message::GetNameFromAddress(sender);
   }

   // we optionally need to convert sender string
   wxFontEncoding encoding = hi->GetEncoding();
   if ( encoding != wxFONTENCODING_SYSTEM )
   {
      wxFontEncoding encoding = hi->GetEncoding();
      if ( !wxTheFontMapper->IsEncodingAvailable(encoding) )
      {
         // try to find another encoding
         wxFontEncoding encAlt;
         if ( wxTheFontMapper->GetAltForEncoding(encoding, &encAlt) )
         {
            wxEncodingConverter conv;
            if ( conv.Init(encoding, encAlt) )
            {
               encoding = encAlt;

               sender = conv.Convert(sender);
               subject = conv.Convert(subject);
            }
            else
            {
               // TODO give an error message here

               // don't attempt to do anything with this encoding
               encoding = wxFONTENCODING_SYSTEM;
            }
         }
      }
   }

   MailFolder *mf = m_ASMailFolder->GetMailFolder();
   m_FolderCtrl->SetEntry(index,
                          MailFolder::ConvertMessageStatusToString(hi->GetStatus(),
                                                                   mf),
                          sender,
                          subject,
                          strutil_ftime(hi->GetDate(),
                                        m_settings.dateFormat,
                                        m_settings.dateGMT),
                          strutil_ultoa(hi->GetSize()));
   mf->DecRef();
   m_FolderCtrl->Select(index,selected);
   m_FolderCtrl->SetItemState(index, wxLIST_STATE_FOCUSED,
                              (hi->GetUId() == m_FocusedUId)?
                              wxLIST_STATE_FOCUSED : 0);
   wxListItem info;
   info.m_itemId = index;
   m_FolderCtrl->GetItem(info);
   int status = hi->GetStatus();

   if(hi->GetColour()[0]) // entry has a colour setting
   {
      wxColour col;
      GetColourByName(&col, hi->GetColour(), MP_FVIEW_FGCOLOUR_D);
      info.SetTextColour(col);
   }
   else
      info.SetTextColour(
         ((status & MailFolder::MSG_STAT_DELETED) != 0 ) ? m_settings.DeletedCol
         : ( ( (status & MailFolder::MSG_STAT_RECENT) != 0 ) ?
             ( ( (status & MailFolder::MSG_STAT_SEEN) != 0 ) ? m_settings.RecentCol
               : m_settings.NewCol )
             : ( ((status & MailFolder::MSG_STAT_SEEN) != 0 ) ?
                 m_settings.FgCol :
                 m_settings.UnreadCol)
            )
         );

   if ( encoding != wxFONTENCODING_SYSTEM )
   {
      wxFont font = m_FolderCtrl->GetFont();
      font.SetEncoding(encoding);
      info.SetFont(font);
   }

   m_FolderCtrl->SetItem(info);
}



void
wxFolderView::Update(HeaderInfoList *listing)
{
   if ( !m_ASMailFolder )
      return;

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

   size_t n = listing->Count();

#ifdef __WXMSW__
   m_FolderCtrl->Hide(); // optimise for speed under MSW
#endif
   long focusedIndex, tmp = -1;
   focusedIndex = m_FolderCtrl->GetNextItem(tmp, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);

   if(n < (size_t) m_NumOfMessages)  // messages have been deleted, start over
   {
      m_FolderCtrl->Clear();
      m_NumOfMessages = 0;
   }

   bool foundFocus = false;
   HeaderInfo const *hi;
   for(size_t i = 0; i < n; i++)
   {
      hi = (*listing)[i];
      SetEntry(listing, i);
      if(hi->GetUId() == m_FocusedUId)
      {
         foundFocus = true;
         focusedIndex = i;
      }
   }
   if(! foundFocus) // old focused UId is gone, so we use the list
      // index instead
      if(focusedIndex != -1 && focusedIndex < (long) n)
         m_FolderCtrl->SetItemState(focusedIndex,
                                    wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

#ifdef DEBUG_greg
printf("Update ==> UpdateTitleAndStatusBars\n");
#endif
   UpdateTitleAndStatusBars("", "", GetFrame(m_Parent), m_MailFolder);

   if(focusedIndex != -1 && focusedIndex < (long) n)
      m_FolderCtrl->EnsureVisible(focusedIndex);

#ifdef __WXMSW__
   m_FolderCtrl->Show();
#endif
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
   MailFolder *mf = MailFolder::OpenFolder(profilename);
   SetFolder(mf);
   SafeDecRef(mf);
   m_ProfileName = profilename;

   wxEndBusyCursor();

   if ( !mf )
   {
      switch ( mApplication->GetLastError() )
      {
         case M_ERROR_CANCEL:
            // don't say anything
            break;

         case M_ERROR_HALFOPENED_ONLY:
            // FIXME propose to show the subfolders dialog from here
            wxLogWarning(_("The folder '%s' cannot be opened, it only contains "
                           "the other folders and not the messages.\n"
                           "Please select \"Browse\" from the menu instead "
                           "to add its subfolders to the folder tree."),
                         profilename.c_str());
            break;

         default:
            // FIXME propose to show the folder properties dialog right here
            wxLogError(_("The folder '%s' could not be opened, please check "
                         "its settings."), profilename.c_str());
      }
   }

   return mf;
}

void
wxFolderView::SearchMessages(void)
{
   SearchCriterium criterium;

   if( ConfigureSearchMessages(&criterium,GetProfile(),NULL) )
   {
      Ticket t = m_ASMailFolder->SearchMessages(&criterium, this);
      m_TicketList->Add(t);
   }
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
   UIdArray selections;

   UpdateSelectionInfo();

   int cmd = event.GetId();

   // first process commands which can be reduced to other commands
   MessageTemplateKind templKind;
   switch ( cmd )
   {
      case WXMENU_MSG_REPLY_WITH_TEMPLATE:
         cmd = WXMENU_MSG_REPLY;
         templKind = MessageTemplate_Reply;
         break;

      case WXMENU_MSG_FORWARD_WITH_TEMPLATE:
         cmd = WXMENU_MSG_FORWARD;
         templKind = MessageTemplate_Forward;
         break;

      case WXMENU_MSG_FOLLOWUP_WITH_TEMPLATE:
         cmd = WXMENU_MSG_FOLLOWUP;
         templKind = MessageTemplate_Followup;
         break;

      default:
         templKind = MessageTemplate_None;
   }

   String templ;
   if ( templKind != MessageTemplate_None )
   {
      templ = ChooseTemplateFor(templKind, GetFrame(m_Parent));
      if ( !templ )
      {
         // cancelled by user
         return;
      }
   }

   switch ( cmd )
   {
      case WXMENU_MSG_SEARCH:
         SearchMessages();
         break;

      case WXMENU_MSG_FIND:
      case WXMENU_MSG_TOGGLEHEADERS:
      case WXMENU_MSG_SHOWRAWTEXT:
      case WXMENU_EDIT_COPY:
         (void)m_MessagePreview->DoMenuCommand(cmd);
         break;

      case WXMENU_LAYOUT_LCLICK:
      case WXMENU_LAYOUT_RCLICK:
      case WXMENU_LAYOUT_DBLCLICK:
         //FIXME m_MessagePreview->OnMouseEvent(event);
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
         SaveMessagesToFolder(selections, NULL, true);
         break;

      case WXMENU_MSG_SAVE_TO_FILE:
         GetSelections(selections);
         SaveMessagesToFile(selections);
         break;

      case WXMENU_MSG_REPLY:
      case WXMENU_MSG_FOLLOWUP:
         {
            int flags = cmd == WXMENU_MSG_FOLLOWUP ? MailFolder::REPLY_FOLLOWUP
                                                   : 0;

            GetSelections(selections);
            m_TicketList->Add(
               m_ASMailFolder->ReplyMessages(
                                             &selections,
                                             MailFolder::Params(templ, flags),
                                             GetFrame(m_Parent),
                                             this
                                            )
            );
         }
         break;

      case WXMENU_MSG_FORWARD:
         GetSelections(selections);
         m_TicketList->Add(
               m_ASMailFolder->ForwardMessages(
                                               &selections,
                                               MailFolder::Params(templ),
                                               GetFrame(m_Parent),
                                               this
                                              )
            );
         break;

      case WXMENU_MSG_QUICK_FILTER:
         GetSelections(selections);
         if ( selections.Count() > 0 )
         {
            // create a filter for the (first of) currently selected message(s)
            m_TicketList->Add(m_ASMailFolder->GetMessage(selections[0], this));
         }
         break;

      case WXMENU_MSG_FILTER:
         GetSelections(selections);
         m_TicketList->Add(m_ASMailFolder->ApplyFilterRules(&selections, this));
         break;

      case WXMENU_MSG_UNDELETE:
         GetSelections(selections);
         m_TicketList->Add(m_ASMailFolder->UnDeleteMessages(&selections, this));
         break;

      case WXMENU_MSG_DELETE:
         GetSelections(selections);
         DeleteOrTrashMessages(selections);
         break;

      case WXMENU_MSG_DELDUPLICATES:
         m_TicketList->Add(m_ASMailFolder->DeleteDuplicates(this));
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
#endif // USE_PS_PRINTING

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
         }
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(MH_FOLDER_VIEW,GetWindow());
         break;

      case WXMENU_FILE_COMPOSE_WITH_TEMPLATE:
      case WXMENU_FILE_COMPOSE:
         {
            wxFrame *frame = GetFrame(m_Parent);

            wxString templ;
            if ( cmd == WXMENU_FILE_COMPOSE_WITH_TEMPLATE )
            {
               templ = ChooseTemplateFor(MessageTemplate_NewMessage, frame);
            }

            wxComposeView *composeView = wxComposeView::CreateNewMessage
                                         (
                                          templ,
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
                                          GetProfile()
                                         );
            composeView->InitText();
            composeView->Show();
         }
         break;

      case WXMENU_MSG_SAVEADDRESSES:
         GetSelections(selections);

         // this is probably not the way it should be done, but I don't know
         // how to get the folder to do all this synchronously otherwise -
         // and I don't think this operation gains anything from being async
         if ( m_MessagePreview )
         {
            CHECK_RET( m_ASMailFolder, "message preview without folder?" );

            MailFolder *mf = m_ASMailFolder->GetMailFolder();
            CHECK_RET( mf, "message preview without folder?" );

            // extract all addresses from the selected messages to this array
            wxSortedArrayString addressesSorted;
            size_t count = selections.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
               Message *msg = mf->GetMessage(selections[n]);
               if ( !msg )
               {
                  FAIL_MSG( "selected message disappeared?" );

                  continue;
               }

               if ( !msg->ExtractAddressesFromHeader(addressesSorted) )
               {
                  // very strange
                  wxLogWarning(_("Selected message doesn't contain any valid addresses."));
               }

               msg->DecRef();
            }

            mf->DecRef();

            wxArrayString addresses = strutil_uniq_array(addressesSorted);
            if ( !addresses.IsEmpty() )
            {
               InteractivelyCollectAddresses(addresses,
                                             READ_APPCONFIG(MP_AUTOCOLLECT_ADB),
                                             m_ProfileName,
                                             (MFrame *)GetFrame(m_Parent));
            }
         }
         break;

#ifdef EXPERIMENTAL_show_uid
      case WXMENU_MSG_SHOWUID:
         GetSelections(selections);
         if ( selections.Count() > 0 )
         {
            MailFolder *mf = m_ASMailFolder->GetMailFolder();

            String uidString = mf->GetMessageUID(selections[0u]);
            if ( uidString.empty() )
               wxLogWarning("This message doesn't have a valid UID.");
            else
               wxLogMessage("The UID of this message is '%s'.",
                            uidString.c_str());

            mf->DecRef();
         }
         break;
#endif // EXPERIMENTAL_show_uid

      default:
         if ( WXMENU_CONTAINS(LANG, cmd) )
         {
            if ( m_MessagePreview )
               m_MessagePreview->SetLanguage(cmd);
         }
         else if ( cmd >= WXMENU_POPUP_FOLDER_MENU )
         {
            // it might be a folder from popup menu
            MFolder *folder = wxFolderMenu::GetFolder
                              (
                                 m_FolderCtrl->GetFolderMenu(),
                                 cmd
                              );
            if ( folder )
            {
               GetSelections(selections);
               SaveMessagesToFolder(selections, folder, true /* move */);

               folder->DecRef();
            }
         }
         break;
      }
}

bool
wxFolderView::HasSelection() const
{
   return m_FolderCtrl->GetSelectedItemCount() != 0;
}

int
wxFolderView::GetSelections(UIdArray& selections)
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
   m_previewUId = uid;
}


void
wxFolderView::OpenMessages(const UIdArray& selections)
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
wxFolderView::PrintMessages(const UIdArray& selections)
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
wxFolderView::PrintPreviewMessages(const UIdArray& selections)
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
wxFolderView::DeleteOrTrashMessages(const UIdArray& messages)
{
   Ticket t = m_ASMailFolder->DeleteOrTrashMessages(&messages, this);
   m_TicketList->Add(t);
}

Ticket
wxFolderView::SaveMessagesToFolder(const UIdArray& selections,
                                   MFolder *folder,
                                   bool del)
{
   Ticket t =
      m_ASMailFolder->SaveMessagesToFolder(&selections,
                                           GetFrame(m_Parent),
                                           folder,
                                           this);
   m_TicketList->Add(t);
   if ( del )
   {
      // also don't forget to delete messages once they're successfulyl saved
      m_TicketsToDeleteList->Add(t);
   }

   return t;
}

void
wxFolderView::DropMessagesToFolder(const UIdArray& selections, MFolder *folder)
{
   Ticket t = SaveMessagesToFolder(selections, folder);

   if ( !m_TicketsDroppedList )
      m_TicketsDroppedList = ASTicketList::Create();

   m_TicketsDroppedList->Add(t);
}

void
wxFolderView::SaveMessagesToFile(const UIdArray& selections)
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
#ifdef DEBUG_greg
printf("OnFolderUpdateEvent ==> Update()\n");
#endif
      Update();
#ifdef DEBUG_greg
printf("OnFolderUpdateEvent ==> UpdateTitleAndStatusBars()\n");
#endif
      UpdateTitleAndStatusBars("", "", GetFrame(m_Parent), m_MailFolder);
   }
}

/// update only one entry in listing:
void
wxFolderView::OnMsgStatusEvent(MEventMsgStatusData &event)
{
   if(event.GetFolder() == m_MailFolder)
   {
#ifdef __WXMSW__
      m_FolderCtrl->Hide(); // optimise for speed under MSW
#endif
      SetEntry(event.GetHeaders(), event.GetIndex());
#ifdef __WXMSW__
      m_FolderCtrl->Show();
#endif
   }
}

bool
wxFolderView::DragAndDropMessages()
{
   UIdArray selections;
   size_t countSel = m_FolderCtrl->GetSelections(selections);
   if ( countSel > 0 )
   {
      MMessagesDataObject dropData(this,
                                   GetFolder()->GetMailFolder(),
                                   selections);
#ifdef __WXMSW__
      wxDropSource dropSource(dropData, m_FolderCtrl,
                              wxCursor("msg_copy"),
                              wxCursor("msg_move"));
#else // Unix
      wxIconManager *iconManager = mApplication->GetIconManager();
      wxIcon icon = iconManager->GetIcon(countSel > 1 ? "dnd_msgs"
                                                      : "dnd_msg");
      wxDropSource dropSource(dropData, m_FolderCtrl, icon);
#endif // OS

      switch ( dropSource.DoDragDrop(TRUE /* allow move */) )
      {
         default:
         case wxDragError:
            wxLogDebug("An error occured during drag and drop operation");
            break;

         case wxDragNone:
         case wxDragCancel:
            wxLogDebug("Drag and drop aborted by user.");
            break;

         case wxDragMove:
            if ( m_TicketsDroppedList )
            {
               // we have to delete the messages as they were moved
               while ( !m_TicketsDroppedList->IsEmpty() )
               {
                  Ticket t = m_TicketsDroppedList->Pop();

                  // the message hasn't been saved yet, wait with deletion
                  // until it is copied successfully
                  m_TicketsToDeleteList->Add(t);
               }

               // also delete the messages which have been already saved
               if ( !m_UIdsCopiedOk.IsEmpty() )
               {
                  DeleteOrTrashMessages(m_UIdsCopiedOk);
                  m_UIdsCopiedOk.Empty();
               }
            }
            //else: nothing was dropped at all

            // fall through

         case wxDragCopy:
            SafeDecRef(m_TicketsDroppedList);
            m_TicketsDroppedList = NULL;

            // we did something
            return TRUE;
      }
   }

   // we didn't do anything
   return FALSE;
}

void
wxFolderView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   String msg;
   ASMailFolder::Result *result = event.GetResult();
   const Ticket& t = result->GetTicket();
   int value = ((ASMailFolder::ResultInt *)result)->GetValue();

   if ( m_TicketList->Contains(t) )
   {
      ASSERT(result->GetUserData() == this);
      m_TicketList->Remove(t);

      switch(result->GetOperation())
      {
      case ASMailFolder::Op_SaveMessagesToFile:
         ASSERT(result->GetSequence());
         if( value )
            msg.Printf(_("Saved %lu messages."), (unsigned long)
                       result->GetSequence()->Count());
         else
            msg.Printf(_("Saving messages failed."));
         wxLogStatus(GetFrame(m_Parent), msg);
         break;

      case ASMailFolder::Op_SaveMessagesToFolder:
         ASSERT(result->GetSequence());

         // we may have to do a few extra things here:
         //  - if the message was marked for deletion (its ticket is in
         //    m_TicketsToDeleteList_, we have to delete it and give a
         //    message about a successful move and not copy operation
         //
         //  - if we're inside DoDragDrop(), m_TicketsDroppedList is !NULL
         //    but we don't know yet if we will have to delete dropped
         //    messages or not, so just remember those of them which were
         //    copied successfully
         {
            bool toDelete = m_TicketsToDeleteList->Contains(t);
            bool wasDropped = m_TicketsDroppedList &&
               m_TicketsDroppedList->Contains(t);

            if ( toDelete )
            {
               m_TicketsToDeleteList->Remove(t);
            }

            if ( !value )
            {
               // something failed - what?
               if ( toDelete )
                  msg = _("Moving messages failed.");
               else if ( wasDropped )
                  msg = _("Dragging messages failed.");
               else
                  msg = _("Copying messages failed.");
            }
            else
            {
               // message was copied ok, what else to do with it?
               if ( toDelete )
               {
                  // delete right now
                  Profile *p = m_ASMailFolder->GetProfile();
                  m_TicketList->Add(m_ASMailFolder->DeleteMessages(
                     result->GetSequence(),
                     (p && READ_CONFIG(p,  MP_USE_TRASH_FOLDER)),
                     this));
                  msg.Printf(_("Moved %lu messages."), (unsigned long)
                             result->GetSequence()->Count());
               }
               else if ( wasDropped )
               {
                  // remember all UIDs as we may have to delete them later
                  m_TicketsDroppedList->Remove(t);

                  UIdArray& seq = *result->GetSequence();
                  WX_APPEND_ARRAY(m_UIdsCopiedOk, seq);

                  msg.Printf(_("Dropped %lu messages."), (unsigned long)
                             result->GetSequence()->Count());
               }
               else
               {
                  msg.Printf(_("Copied %lu messages."), (unsigned long)
                             result->GetSequence()->Count());
               }
            }

            wxLogStatus(GetFrame(m_Parent), msg);
         }
         break;

      case ASMailFolder::Op_SearchMessages:
         ASSERT(result->GetSequence());
         if( value )
         {
            UIdArray *ia = result->GetSequence();
            msg.Printf(_("Found %lu messages."), (unsigned long)
                       ia->Count());
            bool tmp = m_FolderCtrl->EnableSelectionCallbacks(false);
            /* The returned message numbers are UIds which we must map
               to our listctrl indices via the current HeaderInfo
               structure. */
            HeaderInfoList *hil = GetFolder()->GetHeaders();
            for(unsigned long n = 0; n < ia->Count(); n++)
            {
               UIdType idx = hil->GetIdxFromUId((*ia)[n]);
               if(idx != UID_ILLEGAL)
                  m_FolderCtrl->Select(idx);
            }
            hil->DecRef();
            m_FolderCtrl->EnableSelectionCallbacks(tmp);

         }
         else
            msg.Printf(_("No matching messages found."));
         wxLogStatus(GetFrame(m_Parent), msg);
         break;

      case ASMailFolder::Op_ApplyFilterRules:
      {
         ASSERT(result->GetSequence());
         if(value == -1)
            msg.Printf(_("Filtering messages failed."));
         else
            msg.Printf(_("Applied filters to %lu messages, return code %d."),
                       (unsigned long)
                       result->GetSequence()->Count(),
                       value);
         wxLogStatus(GetFrame(m_Parent), msg);
         break;
      }

      case ASMailFolder::Op_DeleteDuplicates:
      {
         UIdType count = ((ASMailFolder::ResultUIdType
                           *)result)->GetValue();
         if(count == UID_ILLEGAL)
            msg = _("An error occured while deleting duplicate messages.");
         else if (count != 0)
            msg.Printf(_("%lu duplicate messages removed."),
                       (unsigned long) count);
         wxLogStatus(GetFrame(m_Parent), msg);
         break;
      }
      // these cases don't have return values
      case ASMailFolder::Op_ReplyMessages:
      case ASMailFolder::Op_ForwardMessages:
      case ASMailFolder::Op_DeleteMessages:
      case ASMailFolder::Op_UnDeleteMessages:
         break;

      case ASMailFolder::Op_GetMessage:
         // so far we only use GetMessage() when processing
         // WXMENU_MSG_QUICK_FILTER
         {
            Message *msg = ((ASMailFolder::ResultMessage *)result)->GetMessage();
            if ( msg )
            {
               MFolder_obj folder(m_folderName);

               CreateQuickFilter(folder, msg->From(), msg->Subject(), m_FolderCtrl);
               msg->DecRef();
            }
         }
         break;

      default:
         wxASSERT_MSG(0,"MEvent handling not implemented yet");
      }
   }
   result->DecRef();
}

// ----------------------------------------------------------------------------
// wxFolderViewFrame
// ----------------------------------------------------------------------------

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
   AddLanguageMenu();

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

      case WXMENU_EDIT_FILTERS:
         {
            MFolder *folder = MFolder::Get(m_FolderView->GetFullName());
            (void) ConfigureFiltersForFolder(folder, this);
            folder->DecRef();
         }
         break;

      default:
         if ( WXMENU_CONTAINS(MSG, id) ||
              WXMENU_CONTAINS(LAYOUT, id) ||
              (WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT)) ||
              id == WXMENU_HELP_CONTEXT ||
              id == WXMENU_FILE_COMPOSE ||
              id == WXMENU_FILE_POST ||
              id == WXMENU_EDIT_CUT ||
              id == WXMENU_EDIT_COPY ||
              id == WXMENU_EDIT_PASTE )
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

// ----------------------------------------------------------------------------
// other public functions
// ----------------------------------------------------------------------------

bool ConfigureFolderViewHeaders(Profile *profile, wxWindow *parent)
{
   // prepare the data for the dialog: we need the array of columns in order
   // and we also need to remember the widths of the columns to shuffle them
   // as well if the columns order is changed

   // this array contains the column positions: columns[WXFLC_XXX] is the
   // index (from 0 to WXFLC_NUMENTRIES) of the column WXFLC_XXX if it is
   // shown and -1 otherwise (we also have a copy of it used below)
   int columns[WXFLC_NUMENTRIES], columnsOld[WXFLC_NUMENTRIES];
   ReadColumnsInfo(profile, columnsOld);
   memcpy(columns, columnsOld, sizeof(columns));

   // these arrays contain the current/default columns widths:
   // strWidths[n] is the width of n-th column where n is the column index
   wxArrayString strWidths, strWidthsStandard;
   strWidths = strutil_restore_array(':', READ_CONFIG(profile, FOLDER_LISTCTRL_WIDTHS));
   strWidthsStandard = strutil_restore_array(':', FOLDER_LISTCTRL_WIDTHS_D);

   wxArrayString choices;
   wxArrayInt status;
   size_t n;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      // find the n-th column shown currently
      wxFolderListCtrlFields col = GetColumnByIndex(columns, n);
      if ( col == WXFLC_NONE )
      {
         break;
      }
      else
      {
         choices.Add(GetColumnName(col));
         status.Add(true);
      }
   }

   // all columns which are shown are already in choices array, add all the
   // ones which are not shown to the end of it too
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      wxString colName = GetColumnName(n);
      if ( choices.Index(colName) == wxNOT_FOUND )
      {
         choices.Add(colName);
         status.Add(false);
      }
   }

   // do show the dialog
   if ( !MDialog_GetSelectionsInOrder(_("All &columns:"),
                                      _("Choose the columns to be shown"),
                                      &choices,
                                      &status,
                                      "FolderViewCol",
                                      parent) )
   {
      // nothing changed
      return false;
   }

   // now write the arrays back to the profile
   int index = 0;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      wxFolderListCtrlFields col = GetColumnByName(choices[n]);
      columns[col] = status[n] ? index++ : -1;
   }

   WriteColumnsInfo(profile, columns);

   // update the widths
   wxArrayString strWidthsNew;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      // find the n-th column shown currently
      wxFolderListCtrlFields col = GetColumnByIndex(columns, n);
      if ( col == WXFLC_NONE )
         break;

      String s;
      int pos = columnsOld[col];
      if ( pos == -1 || (size_t)pos >= strWidths.GetCount() )
         s = strWidthsStandard[col];
      else
         s = strWidths[pos];

      strWidthsNew.Add(s);
   }

   profile->writeEntry(FOLDER_LISTCTRL_WIDTHS,
                       strutil_flatten_array(strWidthsNew, ':'));

   return true;
}
