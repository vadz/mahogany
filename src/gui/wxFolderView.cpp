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
#include "HeaderInfo.h"
#include "ASMailFolder.h"
#include "MessageView.h"
#include "TemplateDialog.h"

#include "gui/wxFolderView.h"
#include "gui/wxMessageView.h"
#include "gui/wxComposeView.h"
#include "gui/wxFolderMenu.h"
#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder()
#include "MFolderDialogs.h"      // for ShowFolderPropertiesDialog

#include "gui/wxMIds.h"
#include "MDialogs.h"
#include "MHelp.h"
#include "miscutil.h"            // for UpdateTitleAndStatusBars

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

// the app profile key where the last column widths modified by user are
// stored
#define USER_COLUMNS_WIDTHS "UserColWidths"

// the profile key where the columns widths of this folder are stored
#define FOLDER_LISTCTRL_WIDTHS "FolderListCtrl"

// the separator in column widths string
#define COLUMNS_WIDTHS_SEP ':'

// the default widths for the columns: the number of entries must be equal to
// WXFLC_NUMENTRIES!
static const char *FOLDER_LISTCTRL_WIDTHS_D = "60:300:200:80:80";

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the var expander for folder view frame status bar
//
// TODO: the code should be reused with VarExpander in wxComposeView.cpp!
class HeaderVarExpander : public MessageTemplateVarExpander
{
public:
   HeaderVarExpander(HeaderInfo *hi,
                     const String& dateFormat,
                     bool dateGMT)
      : m_dateFormat(dateFormat)
   {
      m_hi = hi;

      m_dateGMT = dateGMT;
   }

   virtual bool Expand(const String& category,
                       const String& Name,
                       const wxArrayString& arguments,
                       String *value) const
   {
      if ( !m_hi )
         return false;

      // we only understand fields in the unnamed/default category
      if ( !category.empty() )
         return false;

      String name = Name.Lower();
      if ( name == "from" )
         *value = m_hi->GetFrom();
      else if ( name == "subject" )
         *value = m_hi->GetSubject();
      else if ( name == "date" )
         *value = strutil_ftime(m_hi->GetDate(), m_dateFormat, m_dateGMT);
      else if ( name == "size" )
         *value = m_hi->SizeOf();
      else if ( name == "score" )
         *value = m_hi->GetScore();
      else
         return false;

      return true;
   }

private:
    HeaderInfo *m_hi;
    String m_dateFormat;
    bool m_dateGMT;
};

// ----------------------------------------------------------------------------
// wxFolderListCtrl: the list ctrl showing the messages in the folder
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

   void Select(long index, bool on = true)
      { SetItemState(index, on ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED); }

   void Focus(long index)
      { SetItemState(index, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED); }

   /// if nofocused == true the focused entry will not be substituted
   /// for an empty list of selections
   int GetSelections(UIdArray &selections, bool nofocused = false) const;

   /// get the UID and, optionally, the index of the focused item
   UIdType GetFocusedUId(long *idx = NULL) const;

   /// get the currently focused item or -1
   long GetFocusedItem() const;

   /// is the given item selected?
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
   void OnIdle(wxIdleEvent& event);

   bool EnableSelectionCallbacks(bool enabledisable = true)
      {
         bool rc = m_SelectionCallbacks;
         m_SelectionCallbacks = enabledisable;
         return rc;
      }
   void OnMouseMove(wxMouseEvent &event);

   /// goto next unread message, return true if found
   bool SelectNextUnread(void);

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

   /// save the widths of the columns in profile if needed
   void SaveColWidths();

   // for wxFolderView
   wxMenu *GetFolderMenu() const { return m_menuFolders; }

protected:
   /// read the column width string from profile or default one
   wxString GetColWidths() const;

   /// parent window
   wxWindow *m_Parent;

   /// the folder view
   wxFolderView *m_FolderView;

   /// the currently focused item
   long m_itemFocus;

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

   /// did the listctrl focused item change?
   bool m_FocusedItemChanged;

   /// the popup menu
   wxMenu *m_menu;

   /// and the folder submenu for it
   wxMenu *m_menuFolders;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// FolderViewMessagesDropWhere: a helper class for dnd
// ----------------------------------------------------------------------------

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
// wxFolderSplitterWindow: splitter containing folder and msg view
// ----------------------------------------------------------------------------

class wxFolderSplitterWindow : public wxPSplitterWindow
{
public:
   wxFolderSplitterWindow(wxWindow *parent)
      : wxPSplitterWindow("FolderSplit", parent, -1,
                          wxDefaultPosition, parent->GetClientSize(),
                          wxSP_3D | wxSP_BORDER)
   {
   }
};

// ----------------------------------------------------------------------------
// private functions
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

      case WXFLC_SIZE:
         return MSO_SIZE;

      case WXFLC_STATUS:
         return MSO_STATUS;

      case WXFLC_NUMENTRIES: // suppress gcc warning
      default:
         wxFAIL_MSG( "invalid column" );
         return MSO_NONE;
   }
}

// return columns widths from the given string
static wxArrayString UnpackWidths(const wxString& s)
{
   return strutil_restore_array(COLUMNS_WIDTHS_SEP, s);
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

   EVT_IDLE(wxFolderListCtrl::OnIdle)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderListCtrl
// ----------------------------------------------------------------------------

void wxFolderListCtrl::OnChar(wxKeyEvent& event)
{
   // don't process events until we're fully initialized and also only process
   // simple characters here (without Ctrl/Alt)
   if( !m_FolderView ||
       !m_FolderView->m_MessagePreview ||
       !m_FolderView->GetFolder() ||
       event.HasModifiers() )
   {
      event.Skip();
      return; // nothing to do
   }

   long keyCode = event.KeyCode();
   if ( keyCode == WXK_F1 ) // help
   {
      mApplication->Help(MH_FOLDER_VIEW_KEYBINDINGS,
                         m_FolderView->GetWindow());
      event.Skip();
      return;
   }

   // we can operate either on all selected items
   UIdArray selections;
   long nselected = m_FolderView->GetSelections(selections);

   // or only on the one which is focused
   long focused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);

   UIdType focused_uid = UID_ILLEGAL;

   long newFocus = -1;
   if ( nselected == 0 )
   {
      if ( focused == -1 )
      {
         // no selection and no focus
         event.Skip();
         return;
      }

      // in this case we operate on the highlighted message
      HeaderInfoList_obj hil = m_FolderView->GetFolder()->GetHeaders();
      if ( hil )
      {
         focused_uid = hil[focused]->GetUId();
         selections.Add(focused_uid);
      }

      // don't move focus
      newFocus = -1;
   }
   else // operate on the selected messages
   {
      // advance focus
      newFocus = focused;
      if ( newFocus == -1 )
         newFocus = 0;
      else if ( focused < GetItemCount() - 1 )
         newFocus++;
   }

   /** To allow translations:
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
   if(key == '#')
   {
      // # == expunge for VM compatibility
      idx = 2;
   }

   if(key == WXK_DELETE)
   {
      // delete
      idx = 0;
   }

   // scroll up within the message viewer:
   if(key == WXK_BACK)
   {
      if(m_FolderView->GetPreviewUId() == focused_uid)
         m_FolderView->m_MessagePreview->PageUp();
      event.Skip();
      return;
   }

   if ( key == '*' )
   {
      // toggle the flagged status
      m_FolderView->ToggleMessages(selections);
      return;
   }

   switch(keycodes_en[idx])
   {
      case 'M': // move = copy + delete
         m_FolderView->SaveMessagesToFolder(selections, NULL, true);

         // FIXME: no, it isn't worth it... there are plenty of other places
         //        where messages are moved, so we should check for this at
         //        much lower level - but there we don't have the profile any
         //        more...
#if 0
         // careful here: DeleteOrTrashMessages() will expunge the folder if
         // we're using trash possibly removing other messages from it as well
         // if they had been marked as deleted before
         //
         // note that normally this won't happen as, if we use trash, there
         // should be no deleted messages, but in the case it does (the
         // messages could have been marked as deleted by external program or
         // the user could have switched "Use Trash" option on only recently),
         // warn about it
         if ( (m_FolderView->m_nDeleted > 0) &&
              READ_CONFIG(m_FolderView->m_Profile, MP_USE_TRASH_FOLDER) &&
              MDialog_YesNoDialog
              (
               _("This folder contains some messages marked as deleted.\n"
                 "If you choose to really move this message, they will be\n"
                 "expunged together with the message(s) you move.\n"
                 "\n"
                 "Please select [Yes] to confirm this or [No] to just mark\n"
                 "the moved message(s) as deleted and not expunge it neither."
                 "\n"
                 "Do you want to expunge all messages marked as deleted?"),
               m_Parent,
               MDIALOG_YESNOTITLE,
               false, // [No] default
               m_FolderView->GetFullPersistentKey(M_MSGBOX_MOVE_EXPUNGE_CONFIRM)
              ) )
         {
            // mark the messages as deleted only, don't expunge them
            m_FolderView->GetFolder()->DeleteMessages(&selections, false);

            // don't move focus but do update the selection info
            m_FolderView->UpdateSelectionInfo();
            newFocus = -1;

            break;
         }
#endif // 0
         // fall through

      case 'D': // delete
         m_FolderView->DeleteOrTrashMessages(selections);

         // only move on if we mark as deleted, for trash usage, selection
         // remains the same:
         if ( READ_CONFIG(m_FolderView->m_Profile, MP_USE_TRASH_FOLDER) )
         {
            m_FolderView->UpdateSelectionInfo();

            // don't move focus
            newFocus = -1;
         }
         break;

      case 'U': // undelete
         m_FolderView->GetTicketList()->Add(
            m_FolderView->GetFolder()->UnDeleteMessages(&selections, m_FolderView));
         break;

      case 'X': // expunge
         m_FolderView->ExpungeMessages();
         newFocus = -1;
         break;

      case 'C': // copy
         m_FolderView->SaveMessagesToFolder(selections);
         break;

      case 'S': // save
         m_FolderView->SaveMessagesToFile(selections);
         break;

      case 'G': // group reply
      case 'R': // reply
         m_FolderView->GetFolder()->ReplyMessages
            (
               &selections,
               keycodes_en[idx] == 'G' ? MailFolder::REPLY_FOLLOWUP : 0,
               GetFrame(this),
               m_FolderView
            );
         break;

      case 'F': // forward
         m_FolderView->GetFolder()->ForwardMessages
         (
            &selections,
            MailFolder::Params(),
            GetFrame(this),
            m_FolderView
         );
         break;

      case 'O': // open
         m_FolderView->OpenMessages(selections);
         break;

      case 'P': // print
         m_FolderView->PrintMessages(selections);
         break;

      case 'H': // headers
         m_FolderView->m_MessagePreview->DoMenuCommand(WXMENU_MSG_TOGGLEHEADERS);

         // don't move focus
         newFocus = -1;
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

      default:
         // don't move focus
         newFocus = -1;

         event.Skip();
   }

   if ( newFocus != -1 )
   {
      // move focus
      Focus(newFocus);
      EnsureVisible(newFocus);
      m_FolderView->UpdateSelectionInfo();
   }
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
   // PopupMenu() is very dangerous: it can send more messages before returning
   // and so before we reset m_menuFolders - so we have to protect against it
   // here
   if ( m_menuFolders )
   {
      return;
   }

   // create popup menu if not done yet
   if ( m_menu )
      delete m_menu;

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
   CHECK_RET(hil, "no header listing in wxFolderListCtrl");

   const HeaderInfo *hi = (*hil)[event.m_itemIndex];
   CHECK_RET(hi, "no header entry in wxFolderListCtrl");

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

void wxFolderListCtrl::OnIdle(wxIdleEvent& event)
{
   long itemFocus = GetFocusedItem();

   if ( itemFocus != m_itemFocus )
   {
      m_itemFocus = itemFocus;

      m_FolderView->UpdateSelectionInfo();
   }

   event.Skip();
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

   // no item focused yet
   m_itemFocus = -1;

   Create(parent, M_WXID_FOLDERVIEW_LISTCTRL,
          wxDefaultPosition, parent->GetClientSize(),
          wxLC_REPORT | wxNO_BORDER);

   ReadColumnsInfo(m_profile, m_columns);

   CreateColumns();

   // create a drop target for dropping messages on us
   new MMessagesDropTarget(new FolderViewMessagesDropWhere(m_FolderView), this);
}

wxFolderListCtrl::~wxFolderListCtrl()
{
   m_profile->DecRef();

   delete m_menu;
}

void wxFolderListCtrl::SaveColWidths()
{
   // save the widths if they changed
   String str = GetWidths();
   if ( str != FOLDER_LISTCTRL_WIDTHS_D && str != GetColWidths() )
   {
      // the widths did change, so save them for this folder
      m_profile->writeEntry(FOLDER_LISTCTRL_WIDTHS, str);

      // and also in global location so the next folder the user will open
      // will use these widths unless the user had fixed its width before:
      // this allows to only set the widths once and have them apply to all
      // folders
      mApplication->GetProfile()->writeEntry(USER_COLUMNS_WIDTHS, str);
   }
}

wxString wxFolderListCtrl::GetColWidths() const
{
   // first look in our profile
   String widthsString = READ_CONFIG(m_profile, FOLDER_LISTCTRL_WIDTHS);
   if ( widthsString == FOLDER_LISTCTRL_WIDTHS_D )
   {
      // try the global setting
      widthsString =
         mApplication->GetProfile()->readEntry(USER_COLUMNS_WIDTHS,
                                               FOLDER_LISTCTRL_WIDTHS_D);
   }

   return widthsString;
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

long wxFolderListCtrl::GetFocusedItem() const
{
   return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
}

UIdType
wxFolderListCtrl::GetFocusedUId(long *idx) const
{
   UIdType uid = UID_ILLEGAL;
   ASMailFolder *asmf = m_FolderView->GetFolder();
   if ( asmf )
   {
      HeaderInfoList_obj hil = asmf->GetHeaders();

      // do we have the listing?
      if ( hil )
      {
         size_t nMessages = hil->Count();

         long item = GetFocusedItem();

         // we need to compare with nMessages because the listing might have
         // been already updated and messages could have been deleted from it
         if ( item != -1 && (unsigned long)item < nMessages )
         {
            uid = hil[item]->GetUId();
            if ( idx )
               *idx = item;
         }
      }
   }

   return uid;
}

void wxFolderListCtrl::CreateColumns()
{
   // delete existing columns
   ClearAll();

   // read the widths array
   String widthsString = GetColWidths();

   // complete the array if necessary - this can happen if the previous folder
   // had less columns than this one
   wxArrayString widths = UnpackWidths(widthsString);
   wxArrayString widthsStd = UnpackWidths(FOLDER_LISTCTRL_WIDTHS_D);

   size_t n,
          count = widthsStd.GetCount();

   wxASSERT_MSG( widthsStd.GetCount() == WXFLC_NUMENTRIES,
                 "mismatch in max column number" );

   for ( n = widths.GetCount(); n < count; n++ )
   {
      widths.Add(widths[n]);
   }

   // add the new columns
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
         str << COLUMNS_WIDTHS_SEP;

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

bool
wxFolderListCtrl::SelectNextUnread()
{
   HeaderInfoList_obj hil = m_FolderView->GetFolder()->GetHeaders();
   if( !hil || hil->Count() == 0 )
   {
      // cannot do anything without listing
      return false;
   }

   long idxFocused = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   long idx = idxFocused;
   for ( ;; )
   {
      idx = GetNextItem(idx);
      if ( idx == idxFocused )
      {
         // we have looped
         break;
      }

      if ( idx == -1 )
      {
         // continue from the beginning
         continue;
      }

      // is this one unread?
      const HeaderInfo *hi = hil[idx];
      if( !(hi->GetStatus() & MailFolder::MSG_STAT_SEEN) )
      {
         Focus(idx);

         // always preview the selected message, if we did "Show next unread"
         // we really want to see it regardless of m_PreviewOnSingleClick
         // setting
         m_FolderView->PreviewMessage(hi->GetUId());
         m_FolderView->UpdateSelectionInfo();

         return true;
      }
   }

   String msg;
   if ( (idxFocused != -1) &&
            !(hil[idx]->GetStatus() & MailFolder::MSG_STAT_SEEN) )
   {
      // "more" because the one selected previously was unread
      msg = _("No more unread messages in this folder.");
   }
   else
   {
      // no unread messages at all
      msg = _("No unread messages in this folder.");
   }

   wxLogStatus(GetFrame(this), msg);

   return false;
}

// ----------------------------------------------------------------------------
// wxFolderView
// ----------------------------------------------------------------------------

void
wxFolderView::SetFolder(MailFolder *mf, bool recreateFolderCtrl)
{
   // this shouldn't happen
   CHECK_RET(m_SetFolderSemaphore == false,
             "wxFolderView::SetFolder() called recursively.");

   m_SetFolderSemaphore = true;

   m_FocusedUId = UID_ILLEGAL;
   m_SelectedUIds.Empty();

   // this shows what's happening:
   m_MessagePreview->Clear();
   m_previewUId = UID_ILLEGAL;
   if ( recreateFolderCtrl )
      m_FolderCtrl->Clear();

   SafeIncRef(mf);

   if ( m_ASMailFolder )
   {
      // clean up old folder

      // NB: the test for m_InDeletion is needed because of wxMSW bug which
      //     prevents us from showing a dialog box when called from dtor
      if ( !m_InDeletion )
      {
         if( m_NumOfMessages > 0 &&
             (m_ASMailFolder->GetType() == MF_NNTP ||
              m_ASMailFolder->GetType() == MF_NEWS) )
         {
            wxString msg;
            msg.Printf(_("Mark all articles in\n'%s'\nas read?"),
                       m_ASMailFolder->GetName().c_str());

            if ( MDialog_YesNoDialog
                 (
                  msg,
                  m_Parent,
                  MDIALOG_YESNOTITLE,
                  true,
                  GetFullPersistentKey(M_MSGBOX_MARK_READ)
                 ) )
            {
               UIdArray *seq = GetAllMessagesSequence(m_ASMailFolder);
               m_ASMailFolder->SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
               delete seq;
            }
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
   //else: no old folder

   SafeDecRef(m_Profile);

   m_NumOfMessages = 0; // At the beginning there was nothing.
   m_UpdateSemaphore = false;
   m_MailFolder = mf;
   m_ASMailFolder = mf ? ASMailFolder::Create(mf) : NULL;
   m_Profile = NULL;
   SafeDecRef(mf); // now held by m_ASMailFfolder

   if ( m_ASMailFolder )
   {
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
         // save the columns widths of the old folder view before creating the
         // new one as the latter might use the former
         m_FolderCtrl->SaveColWidths();

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

      // dispatch any update folder events in the queue
      MEventManager::DispatchPending();

      if ( m_NumOfMessages > 0 &&
           READ_CONFIG(m_Profile,MP_AUTOSHOW_FIRSTMESSAGE) )
      {
         m_FolderCtrl->Focus(0);

         HeaderInfoList_obj hil = m_ASMailFolder->GetHeaders();
         if ( hil && hil->Count() > 0 )
            PreviewMessage(hil[0]->GetUId());
      }

#ifndef OS_WIN
      m_FocusFollowMode = READ_CONFIG(m_Profile, MP_FOCUS_FOLLOWSMOUSE);
      if(m_FocusFollowMode && wxWindow::FindFocus() != m_FolderCtrl)
         m_FolderCtrl->SetFocus(); // so we can react to keyboard events
#endif // OS_WIN
   }
   //else: no new folder

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

   m_NumOfMessages =
   m_nDeleted = 0;
   m_previewUId = UID_ILLEGAL;

   m_Profile = Profile::CreateEmptyProfile(mApplication->GetProfile());
   m_SplitterWindow = new wxFolderSplitterWindow(m_Parent);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow, this);
   m_MessagePreview = new wxMessageView(this, m_SplitterWindow);

   ReadProfileSettings(&m_settings);
   bool   previewOnSingleClick = READ_CONFIG(GetProfile(),
                                             MP_PREVIEW_ON_SELECT) != 0;

   m_FolderCtrl->SetPreviewOnSingleClick(previewOnSingleClick);
   m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                               m_settings.BgCol,
                               m_settings.font,
                               m_settings.size,
                               m_settings.columns);
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl,
                                       m_MessagePreview,
                                       m_Parent->GetClientSize().y/3);
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

String wxFolderView::GetFullPersistentKey(MPersMsgBox key)
{
   String s;
   s << Profile::FilterProfileName(m_ProfileName)
     << '/'
     << GetPersMsgBoxName(key);
   return s;
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

   GetColourByName(&settings->FgCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_FGCOLOUR),
                   MP_FVIEW_FGCOLOUR_D);
   GetColourByName(&settings->BgCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_BGCOLOUR),
                   MP_FVIEW_BGCOLOUR_D);
   GetColourByName(&settings->FlaggedCol,
                   READ_CONFIG(m_Profile, MP_FVIEW_FLAGGEDCOLOUR),
                   MP_FVIEW_FLAGGEDCOLOUR_D);
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
   if ( settings->font < 0 || settings->font > NUM_FONTS )
   {
      wxFAIL_MSG( "bad font setting in config" );

      settings->font = 0;
   }

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
      m_NumOfMessages =
      m_nDeleted = 0;
      Update();
   }
}


void
wxFolderView::SetEntry(const HeaderInfo *hi, size_t index)
{
   String   line;
   UIdType nsize;
   unsigned day, month, year;
   String sender, subject, size;
   bool selected;

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

   int status = hi->GetStatus();
   String strStatus = MailFolder::ConvertMessageStatusToString(hi->GetStatus());
   m_FolderCtrl->SetEntry(index,
                          strStatus,
                          sender,
                          subject,
                          strutil_ftime(hi->GetDate(),
                                        m_settings.dateFormat,
                                        m_settings.dateGMT),
                          strutil_ultoa(hi->GetSize()));

   if ( status & MailFolder::MSG_STAT_DELETED )
   {
      // remember the number of deleted messages we have
      m_nDeleted++;
   }

   m_FolderCtrl->Select(index,selected);
   m_FolderCtrl->SetItemState
                 (
                  index,
                  wxLIST_STATE_FOCUSED,
                  hi->GetUId() == m_FocusedUId ? wxLIST_STATE_FOCUSED : 0
                 );
   wxListItem info;
   info.m_itemId = index;
   m_FolderCtrl->GetItem(info);

   wxColour col;
   if ( !hi->GetColour().empty() ) // entry has its own colour setting
   {
      GetColourByName(&col, hi->GetColour(), MP_FVIEW_FGCOLOUR_D);
   }
   else // set the colour depending on the status of the message
   {
      if ( status & MailFolder::MSG_STAT_FLAGGED )
         col = m_settings.FlaggedCol;
      else if ( status & MailFolder::MSG_STAT_DELETED )
         col = m_settings.DeletedCol;
      else if ( status & MailFolder::MSG_STAT_RECENT )
         col = status & MailFolder::MSG_STAT_SEEN ? m_settings.RecentCol
                                                  : m_settings.NewCol;
      else if ( !(status & MailFolder::MSG_STAT_SEEN) )
         col = m_settings.UnreadCol;
      else // normal message
         col = m_settings.FgCol;
   }

   if ( col.Ok() )
      info.SetTextColour(col);

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

   // this shouldn't happen
   CHECK_RET( !m_UpdateSemaphore, "wxFolderView::Update() recursion" );

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
   {
      listing->IncRef();
   }

   size_t n = listing->Count();

   static const size_t THRESHOLD = 100;
   if ( n > THRESHOLD )
   {
      wxBeginBusyCursor();
#ifdef __WXMSW__
      m_FolderCtrl->Hide(); // avoid flicker under MSW
#endif
   }

   long focusedIndex, tmp = -1;
   focusedIndex = m_FolderCtrl->GetNextItem(tmp, wxLIST_NEXT_ALL,wxLIST_STATE_FOCUSED);

   if(n < (size_t) m_NumOfMessages)  // messages have been deleted, start over
   {
      m_FolderCtrl->Clear();
      m_NumOfMessages =
      m_nDeleted = 0;
   }

   bool foundFocus = false;
   HeaderInfo const *hi;
   for(size_t i = 0; i < n; i++)
   {
      hi = (*listing)[i];
      SetEntry(hi, i);
      if(hi->GetUId() == m_FocusedUId)
      {
         foundFocus = true;
         focusedIndex = i;
      }
   }
   if(! foundFocus)
   {
      // old focused UId is gone, so we use the list index instead
      if ( focusedIndex != -1 && focusedIndex < (long) n )
      {
         m_FolderCtrl->Focus(focusedIndex);
      }
   }

   UpdateTitleAndStatusBars("", "", GetFrame(m_Parent), m_MailFolder);

   if(focusedIndex != -1 && focusedIndex < (long) n)
      m_FolderCtrl->EnsureVisible(focusedIndex);

   if ( n > THRESHOLD )
   {
#ifdef __WXMSW__
      m_FolderCtrl->Show();
#endif

      wxEndBusyCursor();
   }

   m_NumOfMessages = n;
   listing->DecRef();

   // the previously focused uid might be gone now:
   if ( !foundFocus )
   {
      m_MessagePreview->Clear();
      m_previewUId = UID_ILLEGAL;
   }

   m_UpdateSemaphore = false;
}


MailFolder *
wxFolderView::OpenFolder(String const &profilename)
{
   MFolder_obj folder(profilename);
   if ( !folder.IsOk() )
   {
      wxLogError(_("The folder '%s' doesn't exist and can't be opened."),
                 profilename.c_str());

      return NULL;
   }

   int flags = folder->GetFlags();
   if ( (flags & MF_FLAGS_UNACCESSIBLE) && !(flags & MF_FLAGS_MODIFIED) )
   {
      wxFrame *frame = GetFrame(m_Parent);
      if ( MDialog_YesNoDialog(_("This folder couldn't be opened last time, "
                                 "do you still want to try to open it (it "
                                 "will probably fail again)?"),
                               frame,
                               MDIALOG_YESNOTITLE,
                               FALSE,
                               "OpenUnaccessibleFolder") )
      {
         if ( MDialog_YesNoDialog(_("Would you like to change folder "
                                    "settings before trying to open it?"),
                                  frame,
                                  MDIALOG_YESNOTITLE,
                                  FALSE,
                                  "ChangeUnaccessibleFolderSettings") )
         {
            // invoke the folder properties dialog
            if ( !ShowFolderPropertiesDialog(folder, frame) )
            {
               // the dialog was cancelled
               wxLogStatus(frame, _("Opening the folder '%s' cancelled."),
                           profilename.c_str());

               return NULL;
            }
         }
      }
   }

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
            // set the flag saying that this folder couldn't be opened which
            // will be used when the user tries to open it the next time

            // it's not modified any more, even if it had been
            folder->ResetFlags(MF_FLAGS_MODIFIED);

            // ... and it is unacessible because we couldn't open it
            folder->AddFlags(MF_FLAGS_UNACCESSIBLE);

            // FIXME propose to show the folder properties dialog right here
            wxLogError(_("The folder '%s' could not be opened, please check "
                         "its settings."), profilename.c_str());
      }
   }
   else
   {
      // reset the unaccessible and modified flags this folder might have had
      // before as now we could open it
      folder->ResetFlags(MF_FLAGS_MODIFIED | MF_FLAGS_UNACCESSIBLE);
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

   long idx;
   UIdType uid = m_FolderCtrl->GetFocusedUId(&idx);
   if ( uid != m_FocusedUId )
   {
      m_FocusedUId = uid;

      String msg;
      if ( m_FocusedUId != UID_ILLEGAL )
      {
         HeaderInfoList_obj hil = GetFolder()->GetHeaders();
         if ( hil )
         {
            wxString fmt = READ_CONFIG(m_Profile, MP_FVIEW_STATUS_FMT);
            HeaderVarExpander expander(hil[idx],
                                       m_settings.dateFormat,
                                       m_settings.dateGMT);
            msg = ParseMessageTemplate(fmt, expander);
         }
      }

      wxLogStatus(GetFrame(m_Parent), msg);
   }
}

void wxFolderView::ExpungeMessages()
{
   if ( m_nDeleted )
   {
      unsigned long nDeletedOld = m_nDeleted;

      m_ASMailFolder->ExpungeMessages();

      wxLogStatus(GetFrame(m_Parent),
                  _("%u deleted messages were expunged"),
                  m_nDeleted - nDeletedOld);
   }
   else
   {
      wxLogWarning(_("No deleted messages - nothing to expunge"));
   }
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
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
         ExpungeMessages();
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
         for ( size_t n = 0; n < m_NumOfMessages; n++ )
            m_FolderCtrl->Select(n,TRUE);
         m_FolderCtrl->EnableSelectionCallbacks(tmp);
         break;
      }

      case WXMENU_MSG_DESELECTALL:
         {
            bool tmp = m_FolderCtrl->EnableSelectionCallbacks(false);
            for ( size_t n = 0; n < m_NumOfMessages; n++ )
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

void
wxFolderView::ToggleMessages(const UIdArray& messages)
{
   HeaderInfoList *hil = GetFolder()->GetHeaders();
   CHECK_RET( hil, "can't toggle messages without folder listing" );

   size_t count = messages.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      UIdType uid = messages[n];
      UIdType idx = hil->GetIdxFromUId(uid);
      if ( idx == UID_ILLEGAL )
      {
         wxLogDebug("Inexistent message (uid = %lu) selected?", uid);

         continue;
      }

      // find the corresponding entry in the listing
      // is the message currently flagged?
      bool flagged = (hil->GetItem(idx)->GetStatus() &
                       MailFolder::MSG_STAT_FLAGGED) != 0;

      // invert the flag
      m_ASMailFolder->SetMessageFlag(uid, MailFolder::MSG_STAT_FLAGGED,
                                     !flagged);
   }

   hil->DecRef();
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
wxFolderView::SetSize(const int x, const int y,
                      const int width, int height)
{
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

// This function gets called when the folder contents changed
void
wxFolderView::OnFolderUpdateEvent(MEventFolderUpdateData &event)
{
   if ( event.GetFolder() == m_MailFolder )
   {
      Update();
   }
}

/// update only one entry in listing:
void
wxFolderView::OnMsgStatusEvent(MEventMsgStatusData &event)
{
   if ( event.GetFolder() == m_MailFolder )
   {
      size_t index = event.GetIndex();
      CHECK_RET( index < (size_t)m_FolderCtrl->GetItemCount(),
                 "invalid index in wxFolderView::OnMsgStatusEvent" );

      SetEntry(event.GetHeaderInfo(), index);
      UpdateTitleAndStatusBars("", "", GetFrame(m_Parent), m_MailFolder);
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
         //    m_TicketsToDeleteList), we have to delete it and give a
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
   EVT_UPDATE_UI_RANGE(WXMENU_MSG_OPEN, WXMENU_MSG_UNDELETE,
                       wxFolderViewFrame::OnUpdateUI)
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

// TODO: this code probably should be updated to avoid writing columns width
//       to the profile if the widths didn't change

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
   strWidths = UnpackWidths(READ_CONFIG(profile, FOLDER_LISTCTRL_WIDTHS));
   strWidthsStandard = UnpackWidths(FOLDER_LISTCTRL_WIDTHS_D);

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
                       strutil_flatten_array(strWidthsNew, COLUMNS_WIDTHS_SEP));

   return true;
}

// ----------------------------------------------------------------------------
// AllProfileSettings
// ----------------------------------------------------------------------------

bool wxFolderView::AllProfileSettings::
operator==(const wxFolderView::AllProfileSettings& other) const
{
   bool equal = dateFormat == other.dateFormat &&
                dateGMT == other.dateGMT &&
                senderOnlyNames == other.senderOnlyNames &&
                replaceFromWithTo == other.replaceFromWithTo &&
                memcmp(columns, other.columns, sizeof(columns)) == 0;

   if ( equal )
   {
      size_t count = returnAddresses.GetCount();
      equal = count == other.returnAddresses.GetCount();
      for ( size_t n = 0; equal && (n < count); n++ )
      {
         equal = returnAddresses[n] == other.returnAddresses[n];
      }
   }

   return equal;
}

