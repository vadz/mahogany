///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxFolderView.cpp - window with folder view inside
// Purpose:     wxFolderView is used to show to the user folder contents
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

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

#ifdef __WXGTK__
   #define BROKEN_LISTCTRL
#endif

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

// the trace mask for dnd messages
#define M_TRACE_DND "msgdnd"

// the trace mask for selection/focus handling
#define M_TRACE_SELECTION "msgsel"

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
         *value = m_hi->GetSize();
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

   // adds a new entry to the control, returns its index
   long AddEntry(UIdType uid,
                 const String &status,
                 const String &sender,
                 const String &subject,
                 const String &date,
                 const String &size);

   // modifies the existing entry
   void SetEntry(long index,
                 const String &status,
                 const String &sender,
                 const String &subject,
                 const String &date,
                 const String &size);

   // changes the status of the existing entry
   void SetEntryStatus(long index, const String& status);

   /// [de]select the given item
   void Select(long index, bool on = true)
   {
      SetItemState(index,
                   on ? wxLIST_STATE_SELECTED : 0,
                   wxLIST_STATE_SELECTED);
   }

   /// select the item currently focused
   void SelectFocused() { Select(GetFocusedItem(), true); }

   /// goto next unread message, return true if found
   bool SelectNextUnread(void);

   /// select next unread message after the given one
   UIdType SelectNextUnreadAfter(const HeaderInfoList_obj& hil,
                                 long indexStart = -1);

   /// focus the given item and ensure it is visible
   void Focus(long index);

   /// get the next selected item after the given one (use -1 to start)
   long GetNextSelected(long item) const
      { return GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }

   /// return true if we preview this item
   bool IsPreviewed(long item) const
      { return item == m_itemPreviewed; }

   /// return true if we preview the item with this UID
   bool IsUIdPreviewed(UIdType uid) const
      { return m_FolderView->GetPreviewUId() == uid; }

   /// get the UID of the given item
   UIdType GetUIdFromIndex(long item) const
   {
      CHECK( item < GetItemCount(), UID_ILLEGAL, "invalid listctrl index" );

      return (UIdType)GetItemData(item);
   }

   /// get the UID and, optionally, the index of the focused item
   UIdType GetFocusedUId(long *idx = NULL) const;

   /// get the currently focused item or -1
   long GetFocusedItem() const;

   /// is the given item selected?
   bool IsSelected(long index)
      { return GetItemState(index,wxLIST_STATE_SELECTED) != 0; }

   /// get the only selected item, return -1 if 0 or >= 2 items are selected
   long GetUniqueSelection() const;

   /// get the item being previewed
   long GetPreviewedItem() const { return m_itemPreviewed; }

   /// get the selected items (use the focused one if no selection)
   UIdArray GetSelectionsOrFocus() const;

   /// return true if we have either selection or valid focus
   bool HasSelection() const;

   /// @name the event handlers
   //@{
   void OnSelected(wxListEvent& event);
   void OnColumnClick(wxListEvent& event);
   void OnListKeyDown(wxListEvent& event);
   void OnChar( wxKeyEvent &event);
   void OnRightClick(wxMouseEvent& event);
   void OnDoubleClick(wxMouseEvent &event);
   void OnActivated(wxListEvent& event);
   void OnCommandEvent(wxCommandEvent& event)
      { m_FolderView->OnCommandEvent(event); }
   void OnIdle(wxIdleEvent& event);
   void OnMouseMove(wxMouseEvent &event);

   /// called by wxFolderView before previewing the focused message
   void OnPreview()
   {
      m_itemPreviewed = GetFocusedItem();
      Select(m_itemPreviewed, true);
   }
   //@}

   /// change the options governing our appearance
   void ApplyOptions(const wxColour &fg, const wxColour &bg,
                     int fontFamily, int fontSize,
                     int columns[WXFLC_NUMENTRIES]);

   /// set m_PreviewOnSingleClick flag
   void SetPreviewOnSingleClick(bool flag)
      {
         m_PreviewOnSingleClick = flag;
      }

   /// save the widths of the columns in profile if needed
   void SaveColWidths();

   /// update the info about focused item if it changed
   void UpdateFocus();

   // for wxFolderView
   wxFolderMenu *GetFolderMenu() const { return m_menuFolders; }

protected:
   /// read the column width string from profile or default one
   wxString GetColWidths() const;

   /// enable/disable handling of select events (returns old value)
   bool EnableOnSelect(bool enable)
   {
      bool enableOld = m_enableOnSelect;
      m_enableOnSelect = enable;
      return enableOld;
   }

   // update our "unique selection" flag
   void UpdateUniqueSelFlag()
   {
      m_selIsUnique = GetUniqueSelection() != -1;
   }

   /// parent window
   wxWindow *m_Parent;

   /// the folder view
   wxFolderView *m_FolderView;

   /// the currently focused item
   long m_itemFocus;

   /// the item currently previewed in the folder view
   long m_itemPreviewed;

   /// this is true as long as we have exactly one selection
   bool m_selIsUnique;

   /// HACK: this is set to true to force m_selIsUnique update if needed
   bool m_selMaybeChanged;

   /// the profile used for storing columns widths
   Profile *m_profile;

   /// column numbers
   int m_columns[WXFLC_NUMENTRIES];

   /// do we preview a message on a single mouse click?
   bool m_PreviewOnSingleClick;

   /// did we create the list ctrl columns?
   bool m_Initialised;

   /// do we handle OnSelected()?
   bool m_enableOnSelect;

   /// the popup menu
   wxMenu *m_menu;

   /// and the folder submenu for it
   wxFolderMenu *m_menuFolders;

   /// flag to prevent reentrance in OnRightClick()
   bool m_isInPopupMenu;

#ifdef BROKEN_LISTCTRL
   /// flag used in UpdateFocus() to work around wxListCtrl bug in wxGTK
   bool m_suppressFocusTracking;
#endif // BROKEN_LISTCTRL

private:
   // allow it to use EnableOnSelect()
   friend class wxFolderListCtrlBlockOnSelect;

   DECLARE_EVENT_TABLE()
};

/// an object of this class blocks OnSelected() handling during its lifetime
class wxFolderListCtrlBlockOnSelect
{
public:
   wxFolderListCtrlBlockOnSelect(wxFolderListCtrl *ctrl)
   {
      m_ctrl = ctrl;
      m_enableOld = m_ctrl->EnableOnSelect(false);
   }

   ~wxFolderListCtrlBlockOnSelect()
   {
      m_ctrl->EnableOnSelect(m_enableOld);
   }

private:
   wxFolderListCtrl *m_ctrl;
   bool m_enableOld;
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
// wxStatusProgress: displays a message in the status bar of the given frame
// in its ctor and the same message with "done" appended in its dtor
// ----------------------------------------------------------------------------

// TODO: move this class in a header, it will be useful elsewhere too

class wxStatusProgress
{
public:
   wxStatusProgress(wxFrame *frame, const char *fmt, ...)
   {
      m_frame = frame;

      va_list argptr;
      va_start(argptr, fmt);
      m_msg.PrintfV(fmt, argptr);
      va_end(argptr);

      wxLogStatus(m_frame, m_msg);
      wxBeginBusyCursor();
   }

   ~wxStatusProgress()
   {
      wxLogStatus(m_frame, m_msg + _("done."));
      wxEndBusyCursor();
   }

private:
   wxFrame *m_frame;
   wxString m_msg;
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
   EVT_CHAR(wxFolderListCtrl::OnChar)
   EVT_LIST_ITEM_ACTIVATED(-1, wxFolderListCtrl::OnActivated)
   EVT_LIST_ITEM_SELECTED(-1, wxFolderListCtrl::OnSelected)

   EVT_MENU(-1, wxFolderListCtrl::OnCommandEvent)

   EVT_RIGHT_DOWN( wxFolderListCtrl::OnRightClick)
   EVT_LEFT_DCLICK(wxFolderListCtrl::OnDoubleClick)
   EVT_MOTION(wxFolderListCtrl::OnMouseMove)

   EVT_LIST_COL_CLICK(-1, wxFolderListCtrl::OnColumnClick)
   EVT_LIST_KEY_DOWN(-1, wxFolderListCtrl::OnListKeyDown)

   EVT_IDLE(wxFolderListCtrl::OnIdle)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderListCtrl char handling
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

   long key = event.KeyCode();
   if ( key == WXK_F1 ) // help
   {
      mApplication->Help(MH_FOLDER_VIEW_KEYBINDINGS, m_FolderView->GetWindow());
      event.Skip();
      return;
   }

   // we can operate either on all selected items
   const UIdArray& selections = m_FolderView->GetSelections();

   // get the focused item
   long focused = GetFocusedItem();

   // find the next item
   long newFocus = focused;
   if ( newFocus == -1 )
      newFocus = 0;
   else if ( focused < GetItemCount() - 1 )
      newFocus++;

   switch ( key )
   {
      case WXK_PRIOR:
      case WXK_NEXT:
         // Shift-PageUp/Down have a predefined meaning in the wxListCtrl, so
         // let it have it
         if ( event.ShiftDown() )
         {
            event.Skip();
            return;
         }
         // fall through

      case WXK_BACK:
      case '*':
         // leave as is
         break;

      case '#':
         // # == expunge for VM compatibility
         key = 'X';
         break;

      case WXK_DELETE:
         // <Del> should delete
         key = 'D';
         break;

      default: // translatable key?
         {
            /** To allow translations:

                Delete, Undelete, eXpunge, Copytofolder, Savetofile,
                Movetofolder, ReplyTo, Forward, Open, Print, Show Headers,
                View, Group reply (== followup)
            */
            static const char keycodes_en[] = gettext_noop("DUXCSMRFOPHG");
            static const char *keycodes = _(keycodes_en);

            key = toupper(key);

            int idx = 0;
            for ( ; keycodes[idx] && keycodes[idx] != key; idx++ )
               ;

            key = keycodes[idx] ? keycodes_en[idx] : 0;
         }
   }

   switch ( key )
   {
      case 'M': // move = copy + delete
         if ( !m_FolderView->SaveMessagesToFolder(selections, NULL, true) )
         {
            // don't delete the messages if we couldn't save them!
            newFocus = -1;
         }
         break;

      case 'D': // delete
         m_FolderView->DeleteOrTrashMessages(selections);

         // only move on if we mark as deleted, for trash usage, selection
         // remains the same:
         if ( READ_CONFIG(m_FolderView->m_Profile, MP_USE_TRASH_FOLDER) )
         {
            // don't move focus
            newFocus = -1;
         }
         break;

      case 'U': // undelete
         m_FolderView->GetTicketList()->
            Add(m_FolderView->GetFolder()->UnDeleteMessages(&selections,
                                                            m_FolderView));
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
               key == 'G' ? MailFolder::REPLY_FOLLOWUP : 0,
               GetFrame(this),
               m_FolderView
            );
         newFocus = -1;
         break;

      case 'F': // forward
         m_FolderView->GetFolder()->ForwardMessages
         (
            &selections,
            MailFolder::Params(),
            GetFrame(this),
            m_FolderView
         );
         newFocus = -1;
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

      case '*':
         // toggle the flagged status
         m_FolderView->ToggleMessages(selections);

         // don't move focus
         newFocus = -1;
         break;

      case WXK_NEXT:
         // scroll down the preview window
         if ( IsPreviewed(focused) )
         {
            m_FolderView->m_MessagePreview->PageDown();
         }
         else
         {
            // let it work as usual
            event.Skip();
         }

         // don't move focus
         newFocus = -1;
         break;

      case WXK_PRIOR:
      case WXK_BACK:
         // scroll up within the message viewer:
         if ( IsPreviewed(focused) )
         {
            m_FolderView->m_MessagePreview->PageUp();
         }
         else
         {
            // let it work as usual
            event.Skip();
         }


         // don't move focus
         newFocus = -1;
         break;

      default:
         // all others should have been mapped to 0 in the code above
         FAIL_MSG("unexpected key");

         // fall through

      case 0:
         // don't move focus
         newFocus = -1;

         event.Skip();
   }

   if ( newFocus != -1 )
   {
      // move focus
      Focus(newFocus);
      UpdateFocus();
   }
}

void wxFolderListCtrl::OnMouseMove(wxMouseEvent &event)
{
   // a workaround for focus handling - otherwise, the listctrl keeps losing
   // focus
#ifdef OS_WIN
   // workaround for workaround: we have to test this to avoid the frame
   // containing the list ctrl being raised to the top from behind another top
   // level frame
   HWND hwndTop = ::GetForegroundWindow();
   wxFrame *frame = m_FolderView->m_Frame;
   if ( frame && frame->GetHWND() == (WXHWND)hwndTop )
#endif // OS_WIN
   if ( m_FolderView->GetFocusFollowMode() && (FindFocus() != this) )
   {
      SetFocus();
   }

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
   if ( m_isInPopupMenu )
   {
      return;
   }

#ifdef __WXGTK__
   // I've tried to cache the folder menu to avoid recreating it every time
   // (which is quite time consuming) but this can't work because of wxGTK bug!
   // Argh...
   if ( m_menu )
   {
      delete m_menu;
      m_menu = NULL;
   }

   if ( m_menuFolders )
   {
      delete m_menuFolders;
      m_menuFolders = NULL;
   }
#endif // __WXGTK__

   // create popup menu if not done yet
   if ( !m_menu )
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

      m_menu = new wxMenu;

      // the first item is going to be the quick move menu added below
      m_menu->AppendSeparator();

      for ( size_t n = 0; n < WXSIZEOF(popupMenuEntries); n++ )
      {
         int id = popupMenuEntries[n];
         AppendToMenu(m_menu, id);
      }
   }

   if ( !m_menuFolders )
   {
      m_menuFolders = new wxFolderMenu();
   }

   m_menu->Insert(0, WXMENU_POPUP_FOLDER_MENU, _("&Quick move"),
                  m_menuFolders->GetMenu());

   m_isInPopupMenu = true;

   PopupMenu(m_menu, event.GetX(), event.GetY());

   m_isInPopupMenu = false;

#ifdef __WXGTK__
   m_menuFolders->Detach();
#else // !__WXGTK__
   m_menu->Remove(WXMENU_POPUP_FOLDER_MENU);
#endif // __WXGTK__/!__WXGTK__
}

void wxFolderListCtrl::OnDoubleClick(wxMouseEvent& /*event*/)
{
   // there is exactly one item with the focus on it
   long focused = GetFocusedItem();

   // this would be a wxWin bug
   CHECK_RET( focused != -1, "activated unfocused item?" );

   // show the focused message
   UIdType uid = GetUIdFromIndex(focused);
   if ( m_PreviewOnSingleClick )
   {
      // view on double click then
      new wxMessageViewFrame(m_FolderView->GetFolder(), uid, m_FolderView);
   }
   else // preview on double click
   {
      m_FolderView->PreviewMessage(uid);
   }
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   mApplication->UpdateAwayMode();

   long item = event.m_itemIndex;

   wxLogTrace(M_TRACE_SELECTION, "%ld was selected", item);

   UpdateUniqueSelFlag();

   if ( m_enableOnSelect )
   {
      // update it as it is normally only updated in OnIdle() which wasn't
      // called yet
      m_itemFocus = GetFocusedItem();

      // only preview the message when it is the first one we select,
      // selecting subsequent messages just extends the selection but doesn't
      // show them
      if ( m_selIsUnique && (item == m_itemFocus) )
      {
         // the current message is selected - view it if we don't yet
         m_FolderView->PreviewMessage(GetFocusedUId());
      }
      //else: don't react to selecting another message
   }
   //else: processing this is temporarily blocked
}

// called by RETURN press
void wxFolderListCtrl::OnActivated(wxListEvent& event)
{
   UIdType uid = GetUIdFromIndex(event.m_itemIndex);

   if ( IsPreviewed(event.m_itemIndex) )
   {
      // scroll down one line
      m_FolderView->m_MessagePreview->LineDown();
   }
   else // do preview
   {
      m_FolderView->PreviewMessage(uid);
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

void wxFolderListCtrl::UpdateFocus()
{
   long itemFocus = GetFocusedItem();

   if ( itemFocus == m_itemFocus )
      return;

   // What we do here is to automatically preview the currently focused message
   // if and only if there is exactly one item currently selected.
   //
   // Rationale: if there are no items selected, the user is just moving
   // through the headers list and doesn't want to preview anything at all, so
   // don't do anything. If there are 2 or more items selected, we shouldn't
   // deselect them as it would cancel the users work which he did to select
   // the messages in the first place. But if he just moved to the next
   // message after previewing the previous one, he does want to preview it
   // (unless "preview on select" option is off) but to do this he has to
   // manually unselect the previously selected message and only then select
   // this one (as selecting this one now won't preview it because it is not
   // the first selected message, see OnSelected()!)

   // we have to use tmp var as Select() will reset m_selIsUnique
   bool selIsUnique = m_selIsUnique;
   if ( selIsUnique && (m_itemFocus != -1) )
   {
      Select(m_itemFocus, false);
   }

   m_itemFocus = itemFocus;

   m_FolderView->OnFocusChange();

   if ( selIsUnique )
   {
      // will set m_selIsUnique to true back again
      Select(m_itemFocus, true);
   }
}

void wxFolderListCtrl::OnListKeyDown(wxListEvent& event)
{
   mApplication->UpdateAwayMode();

   switch ( event.GetCode() )
   {
      case WXK_UP:
      case WXK_DOWN:
      case WXK_PRIOR:
      case WXK_NEXT:
      case WXK_HOME:
      case WXK_END:
         // update the unique selection flag as we can lose it now (the keys
         // above can change the focus/selection)
         UpdateUniqueSelFlag();
         break;

      case WXK_SPACE:
         // selection may change if this is Ctrl-Space, for example, so check
         // it a bit later
         m_selMaybeChanged = true;
         break;
   }

   event.Skip();
}

void wxFolderListCtrl::OnIdle(wxIdleEvent& event)
{
   // if we suspect that the state of the selection may change without notifying
   // us about it, we set this flag
   if ( m_selMaybeChanged )
   {
      UpdateUniqueSelFlag();

      m_selMaybeChanged = false;
   }

   // there is no "focus change" event in wxListCtrl so we have to track it
   // ourselves
#ifdef BROKEN_LISTCTRL
   if ( !m_suppressFocusTracking )
#endif // BROKEN_LISTCTRL
   {
      UpdateFocus();
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
   m_Initialised = false;
   m_enableOnSelect = true;
   m_menu = NULL;
   m_menuFolders = NULL;

   m_isInPopupMenu = false;

#ifdef BROKEN_LISTCTRL
   m_suppressFocusTracking = false;
#endif // BROKEN_LISTCTRL

   // no item focused yet
   m_itemFocus = -1;

   // nor previewed
   m_itemPreviewed = -1;

   // no selection at all
   m_selIsUnique = false;

   // and it didn't change yet
   m_selMaybeChanged = false;

   // do create the control
   Create(parent, M_WXID_FOLDERVIEW_LISTCTRL,
          wxDefaultPosition, parent->GetClientSize(),
          wxLC_REPORT | wxNO_BORDER);

   // set the initial column widths
   ReadColumnsInfo(m_profile, m_columns);

   // and create columns after this
   CreateColumns();

   // create a drop target for dropping messages on us
   new MMessagesDropTarget(new FolderViewMessagesDropWhere(m_FolderView), this);
}

wxFolderListCtrl::~wxFolderListCtrl()
{
   SaveColWidths();

   m_profile->DecRef();

   delete m_menuFolders;
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

void wxFolderListCtrl::Focus(long index)
{
   SetItemState(index, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

   // this doesn't work well with wxWin <= 2.2.5 in debug mode as calling
   // EnsureVisible() results in an assert failure which is harmless but
   // _very_ annoying as it happens all the time
#if !defined(__WXDEBUG__) || wxCHECK_VERSION(2,2,6)
   if ( index != -1 )
   {
      // we don't want any events come here while we're inside EnsureVisible()
      MEventManagerSuspender noEvents;

#ifdef BROKEN_LISTCTRL
      // EnsureVisible() shouldn't call our OnIdle!
      m_suppressFocusTracking = true;
#endif // BROKEN_LISTCTRL

      EnsureVisible(index);

#ifdef BROKEN_LISTCTRL
      m_suppressFocusTracking = false;
#endif // BROKEN_LISTCTRL
   }
#endif // wxWin >= 2.2.6
}

long wxFolderListCtrl::GetFocusedItem() const
{
   return GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
}

long wxFolderListCtrl::GetUniqueSelection() const
{
   long item = GetNextSelected(-1);
   if ( item != -1 )
   {
      if ( GetNextSelected(item) != -1 )
      {
         // > 1 items selected
         item = -1;
      }
      //else: exactly 1 item selected
   }
   //else: no items selected

   return item;
}

UIdArray
wxFolderListCtrl::GetSelectionsOrFocus() const
{
   UIdArray uids;

   long item = GetNextSelected(-1);
   while ( item != -1 )
   {
      uids.Add(GetItemData(item));

      item = GetNextSelected(item);
   }

   // if no selection, use the focused item
   if ( uids.IsEmpty() )
   {
      uids.Add(GetFocusedUId());
   }

   return uids;
}

bool
wxFolderListCtrl::HasSelection() const
{
   return GetNextSelected(-1) != -1 || GetFocusedItem() != -1;
}

UIdType
wxFolderListCtrl::GetFocusedUId(long *idx) const
{
   long item = GetFocusedItem();
   if ( idx )
      *idx = item;

   return item == -1 ? UID_ILLEGAL : GetUIdFromIndex(item);
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
      widths.Add(widthsStd[n]);
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

long
wxFolderListCtrl::AddEntry(UIdType uid,
                           const String &status,
                           const String &sender,
                           const String &subject,
                           const String &date,
                           const String &size)
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

   // add the item
   long index = GetItemCount();
   InsertItem(index, label);

   // store the UID with the item
   SetItemData(index, (long)uid);

   // set the other columns
   SetEntry(index, status, sender, subject, date, size);

   // return the index of the new item
   return index;
}

void
wxFolderListCtrl::SetEntry(long index,
                           const String &status,
                           const String &sender,
                           const String &subject,
                           const String &date,
                           const String &size)
{
   ASSERT_MSG( index < GetItemCount(), "invalid index in wxFolderListCtrl" );

   SetEntryStatus(index, status);

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
wxFolderListCtrl::SetEntryStatus(long index, const String& status)
{
   if ( m_columns[WXFLC_STATUS] != -1 )
      SetItem(index, m_columns[WXFLC_STATUS], status);
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

   long idxFocused = GetFocusedItem();
   UIdType uid = SelectNextUnreadAfter(hil, idxFocused);
   if ( uid != UID_ILLEGAL )
   {
      // always preview the selected message, if we did "Show next unread"
      // we really want to see it regardless of m_PreviewOnSingleClick
      // setting
      m_FolderView->PreviewMessage(uid);
      m_FolderView->OnFocusChange();

      return true;
   }
   //else: no next unread msg found

   String msg;
   if ( (idxFocused != -1) &&
            !(hil[idxFocused]->GetStatus() & MailFolder::MSG_STAT_SEEN) )
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


UIdType wxFolderListCtrl::SelectNextUnreadAfter(const HeaderInfoList_obj& hil,
                                                long idxFocused)
{
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

         return hi->GetUId();
      }
   }

   return UID_ILLEGAL;
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

   // this shows what's happening:
   m_MessagePreview->Clear();

   // reset them as they don't make sense for the new folder
   InvalidatePreviewUID();

   if ( recreateFolderCtrl )
      m_FolderCtrl->DeleteAllItems();

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
                  m_Frame,
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

         CheckExpungeDialog(m_ASMailFolder, m_Frame);
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
      InvalidatePreviewUID();

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
         m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);

         m_SplitterWindow->ReplaceWindow(oldfolderctrl, m_FolderCtrl);
         delete oldfolderctrl;
      }

      // TODO: sometimes we already have an UpdateFolder event in the event
      //       queue and then we could avoid updating the folder view twice,
      //       but it can also happen that we open a folder which had been
      //       opened before and then MailFolderCC::Open() we had called
      //       doesn't produce any update events - currently we have no way to
      //       test for this, but we should either add a method to MailFolder
      //       to allow us to detect if the folder is already opened or not,
      //       or add a method to MEventManager to detect if the event is
      //       already in the queue
      //
      // For now, let it process the event if there is any and if not, update
      // manually
      MEventManager::DispatchPending();
      if ( m_NumOfMessages == 0 )
      {
         Update();
      }

      // select some "interesting" message initially: the logic here is a bit
      // hairy, but, hopefully, this does what expected.
      //
      // explanations: if MP_AUTOSHOW_FIRSTUNREADMESSAGE is off, then we
      // just select either the first message (if MP_AUTOSHOW_FIRSTMESSAGE) or
      // the last one (otherwise). If it is on and we have an unread message,
      // we always select first unread message, but if there are no unread
      // messages, we revert to the previous behaviour, i.e. select the first
      // or the last one
      if ( m_NumOfMessages > 0 )
      {
         HeaderInfoList_obj hil = m_ASMailFolder->GetHeaders();
         if ( hil )
         {
            UIdType uid;
            if ( READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTUNREADMESSAGE) )
            {
               uid = m_FolderCtrl->SelectNextUnreadAfter(hil);
            }
            else
            {
               uid = UID_ILLEGAL;
            }

            if ( uid == UID_ILLEGAL )
            {
               // select first unread is off or no unread message
               unsigned long idx
                  = READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTMESSAGE)
                     ? 0
                     : m_NumOfMessages - 1;

               m_FolderCtrl->Focus(idx);

               HeaderInfo *hi = hil[idx];
               if ( hi )
               {
                  uid = hi->GetUId();
               }
               else
               {
                  wxFAIL_MSG("Failed to get the uid of preselected message");
               }
            }

            // the item is already focused, now preview it automatically too
            // if we're configured to do this automatically
            if ( (uid != UID_ILLEGAL) && m_settings.previewOnSingleClick )
            {
               PreviewMessage(uid);
            }
         }
      }

      m_FocusFollowMode = READ_CONFIG(m_Profile, MP_FOCUS_FOLLOWSMOUSE) != 0;
      if(m_FocusFollowMode && wxWindow::FindFocus() != m_FolderCtrl)
         m_FolderCtrl->SetFocus(); // so we can react to keyboard events
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
   // cast is harmless as we can only have MFrames in this application
   m_Frame = (MFrame *)GetFrame(m_Parent);

   m_MailFolder = NULL;
   m_ASMailFolder = NULL;
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);

   m_TicketList =  ASTicketList::Create();
   m_TicketsToDeleteList = ASTicketList::Create();
   m_TicketsDroppedList = NULL;

   m_NumOfMessages =
   m_nDeleted = 0;
   InvalidatePreviewUID();

   m_Profile = Profile::CreateEmptyProfile(mApplication->GetProfile());
   m_SplitterWindow = new wxFolderSplitterWindow(m_Parent);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow, this);
   m_MessagePreview = new wxMessageView(this, m_SplitterWindow);

   ReadProfileSettings(&m_settings);
   m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);
   m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                               m_settings.BgCol,
                               m_settings.font,
                               m_settings.size,
                               m_settings.columns);
   m_SplitterWindow->SplitHorizontally((wxWindow *)m_FolderCtrl,
                                       m_MessagePreview,
                                       m_Parent->GetClientSize().y/3);
   m_SplitterWindow->SetMinimumPaneSize(10);
   m_SplitterWindow->SetFocus();
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
   SetFolder(NULL, false);
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

   settings->showSize =
      (MessageSizeShow)READ_CONFIG(m_Profile, MP_FVIEW_SIZE_FORMAT);

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

   settings->previewOnSingleClick =
      READ_CONFIG(GetProfile(), MP_PREVIEW_ON_SELECT) != 0;

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

   AllProfileSettings settings;
   ReadProfileSettings(&settings);

   // handle this setting specially as we don't have to recreate the folder
   // list ctrl if it changes, so just pretend it didn't change
   m_settings.previewOnSingleClick = settings.previewOnSingleClick;

   // did any other, important, setting change?
   if ( settings != m_settings )
   {
      // yes: need to repopulate the list ctrl because its appearance changed
      m_settings = settings;

      m_FolderCtrl->ApplyOptions( m_settings.FgCol,
                                  m_settings.BgCol,
                                  m_settings.font,
                                  m_settings.size,
                                  m_settings.columns);
      m_FolderCtrl->DeleteAllItems();
      m_NumOfMessages =
      m_nDeleted = 0;
      Update();
   }

   m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);
}


void
wxFolderView::AddEntry(const HeaderInfo *hi)
{
   String subject = wxString(' ', 3*hi->GetIndentation()) + hi->GetSubject();
   String sender = hi->GetFrom();

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

      // if there is no "To" address, this must be a newsgroup posting (not
      // 100% true, but for 99% it is)
      String to = hi->GetTo(),
             newsgroups = hi->GetNewsgroups();
      if ( to.empty() && !newsgroups.empty() )
         sender << _("Posted to: ") << newsgroups;
      else
         sender << _("To: ") << to;
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
   int index = m_FolderCtrl->AddEntry
                             (
                              hi->GetUId(),
                              strStatus,
                              sender,
                              subject,
                              strutil_ftime(hi->GetDate(),
                                            m_settings.dateFormat,
                                            m_settings.dateGMT),
                              MailFolder::SizeToString(hi->GetSize(),
                                                       hi->GetLines(),
                                                       m_settings.showSize)
                             );

   if ( status & MailFolder::MSG_STAT_DELETED )
   {
      // remember the number of deleted messages we have
      m_nDeleted++;
   }

   wxListItem info;
   info.m_itemId = index;
   m_FolderCtrl->GetItem(info);

   if ( encoding != wxFONTENCODING_SYSTEM )
   {
      wxFont font = m_FolderCtrl->GetFont();
      font.SetEncoding(encoding);
      info.SetFont(font);
   }

   m_FolderCtrl->SetItem(info);

   SetEntryColour(index, hi);
}

void
wxFolderView::SetEntryColour(size_t index, const HeaderInfo *hi)
{
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
      int status = hi->GetStatus();

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

   if ( col.Ok() && col != info.GetTextColour() )
   {
      info.SetTextColour(col);

      m_FolderCtrl->SetItem(info);
   }
}


void
wxFolderView::Update()
{
   if ( !m_ASMailFolder )
      return;

   // this shouldn't happen but unfortunately it did before because
   // wxGTK::wxListCtrl::EnsureVisible() which we call from here calls
   // wxYield() - now I've added a workaround for this, but keep this check in
   // case it starts happen again
   //
   // notice that it's a real error if it happens because the update folder
   // events can just be lost like this
   CHECK_RET( !m_UpdateSemaphore, "reentrancy in wxFolderView::Update" );

   HeaderInfoList_obj listing = m_ASMailFolder->GetHeaders();
   if ( !listing )
   {
      return;
   }

   m_UpdateSemaphore = true;

   MFrame *frameOld = m_MailFolder->SetInteractiveFrame(m_Frame);

   size_t numMessages = listing->Count();

   static const size_t THRESHOLD = 100;
   if ( numMessages > THRESHOLD )
   {
      wxBeginBusyCursor();
#ifdef __WXMSW__
      m_FolderCtrl->Hide(); // avoid flicker under MSW
#endif
   }

   wxLogTrace(M_TRACE_SELECTION, "recreating the listctrl from Update()");

   // completely repopulate the control
   m_FolderCtrl->DeleteAllItems();

   // this will reset m_uidFocused
   OnFocusChange();

   // which message should be focused after update? notice that we can *not*
   // keep the old index because the position of the message might have changed
   // because of sorting/threading
   bool searchForFocus = m_uidPreviewed != UID_ILLEGAL;

   // fill the list control
   const HeaderInfo *hi;
   for ( size_t i = 0; i < numMessages; i++ )
   {
      hi = listing[i];

      // this adds it to the list control
      AddEntry(hi);

      // if we haven't found it yet ...
      if ( searchForFocus )
      {
         // ... try to find the previously focused item
         if ( hi->GetUId() == m_uidPreviewed )
         {
            searchForFocus = false;

            m_FolderCtrl->Focus(i);
            m_FolderCtrl->Select(i);
         }
      }
   }

   UpdateTitleAndStatusBars("", "", m_Frame, m_MailFolder);

   if ( numMessages > THRESHOLD )
   {
#ifdef __WXMSW__
      m_FolderCtrl->Show();
#endif

      wxEndBusyCursor();
   }

   m_NumOfMessages = numMessages;

   // clear the preview window if the focused message disappeared: this happens
   // only if we didn't find it above (notice that if there was no preview
   // message to start with, we have nothing to do here)
   if ( searchForFocus )
   {
      m_MessagePreview->Clear();
      InvalidatePreviewUID();
   }
   //else: it is still there

   m_UpdateSemaphore = false;

   m_MailFolder->SetInteractiveFrame(frameOld);
}


MailFolder *
wxFolderView::OpenFolder(const String &profilename)
{
   MFolder_obj folder(profilename);
   if ( !folder.IsOk() )
   {
      wxLogError(_("The folder '%s' doesn't exist and can't be opened."),
                 profilename.c_str());

      return NULL;
   }

   // just a cast
   wxFrame *frame = m_Frame;

   int flags = folder->GetFlags();

   // special check for IMAP servers (i.e. IMAP folders withotu path): we
   // create by default just the entry for the server but the user needs to
   // work with the mailboxes on the IMAP server, at least with INBOX, so ask
   // him if he wants to do it first
   //
   // if the IMAP server already has some subfolders, we don't need to do this,
   // of course, as we either already did or the user was smart enough to
   // create the subfolders himself anyhow
   if ( (folder->GetType() == MF_IMAP) &&
        folder->GetPath().empty() &&
        (folder->GetSubfolderCount() == 0) )
   {
      if ( MDialog_YesNoDialog
           (
             _("You are trying to open an IMAP server: this will usually\n"
               "open the special mailbox called INBOX, however there may\n"
               "exist other mailboxes on this server which don't appear\n"
               "in Mahogany folder tree yet.\n"
               "\n"
               "Would you like to retrieve all existing mailboxes from\n"
               "the IMAP server right now? If you reply [No], you can\n"
               "also choose \"Browse subfolders...\" from the folder\n"
               "popup menu later and selectively choose the mailboxes\n"
               "to create in the tree from there. If you reply [Yes],\n"
               "all existing mailboxes on the server will be created in\n"
               "the folder tree unconditionally."),
             frame,
             MDIALOG_YESNOTITLE,
             true,
             GetFullPersistentKey(M_MSGBOX_BROWSE_IMAP_SERVERS)
           ) )
      {
         // create all folders under the IMAP server
         ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folder, NULL);
         if ( !asmf )
         {
            wxLogError(_("Impossible to add IMAP folders to the folder tree."));
         }
         else
         {
            if ( AddAllSubfoldersToTree(folder, asmf) > 0 )
            {
               wxLogStatus(_("You can now open any of folders on the "
                             "IMAP server '%s'"), folder->GetName().c_str());
            }

            asmf->DecRef();
         }

         return NULL;
      }
      //else: continue opening the server, this will just open INBOX on it
   }

   // check if we didn't mark this folder as unopenable the last time
   if ( (flags & MF_FLAGS_UNACCESSIBLE) && !(flags & MF_FLAGS_MODIFIED) )
   {
      // note that we don't use GetFullPersistentKey() here as we want this
      // setting to be global (it isn't very useful to disable this msg box
      // only for one folder)
      if ( MDialog_YesNoDialog
           (
            _("This folder couldn't be opened last time, "
              "do you still want to try to open it (it "
              "will probably fail again)?"),
            frame,
            MDIALOG_YESNOTITLE,
            false,
            GetPersMsgBoxName(M_MSGBOX_OPEN_UNACCESSIBLE_FOLDER)
           ) )
      {
         if ( MDialog_YesNoDialog
              (
               _("Would you like to change folder "
                 "settings before trying to open it?"),
               frame,
               MDIALOG_YESNOTITLE,
               false,
               GetPersMsgBoxName(M_MSGBOX_CHANGE_UNACCESSIBLE_FOLDER_SETTINGS)
            ) )
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
   MailFolder::SetInteractive(m_Frame, profilename);

   MailFolder *mf = MailFolder::OpenFolder(profilename);
   SetFolder(mf);
   SafeDecRef(mf);
   m_ProfileName = profilename;

   MailFolder::ResetInteractive();
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

   if ( ConfigureSearchMessages(&criterium,GetProfile(),NULL) )
   {
      Ticket t = m_ASMailFolder->SearchMessages(&criterium, this);
      m_TicketList->Add(t);
   }
}

void wxFolderView::ExpungeMessages()
{
   if ( m_nDeleted )
   {
      m_ASMailFolder->ExpungeMessages();

      m_nDeleted = 0;
   }
   else
   {
      wxLogWarning(_("No deleted messages - nothing to expunge"));
   }
}

void wxFolderView::SelectAllUnread()
{
   wxFolderListCtrlBlockOnSelect dontHandleOnSelect(m_FolderCtrl);

   HeaderInfoList_obj hil = GetFolder()->GetHeaders();
   CHECK_RET( hil, "can't select unread messages without folder listing" );

   size_t count = hil->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( !(hil[n]->GetStatus() & MailFolder::MSG_STAT_SEEN) )
      {
         m_FolderCtrl->Select(n, true);
      }
   }
}

void wxFolderView::SelectAll(bool on)
{
   wxFolderListCtrlBlockOnSelect dontHandleOnSelect(m_FolderCtrl);

   for ( size_t n = 0; n < m_NumOfMessages; n++ )
   {
      m_FolderCtrl->Select(n, on);
   }
}

void
wxFolderView::OnCommandEvent(wxCommandEvent &event)
{
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
      templ = ChooseTemplateFor(templKind, m_Frame);
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
      case WXMENU_EDIT_COPY:
         if ( m_MessagePreview )
         {
            (void)m_MessagePreview->DoMenuCommand(cmd);
         }
         break;

      case WXMENU_MSG_SHOWRAWTEXT:
         ShowRawText(GetFocus());
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
         OpenMessages(GetSelections());
         break;

      case WXMENU_MSG_SAVE_TO_FOLDER:
         SaveMessagesToFolder(GetSelections());
         break;

      case WXMENU_MSG_MOVE_TO_FOLDER:
         SaveMessagesToFolder(GetSelections(), NULL, true);
         break;

      case WXMENU_MSG_SAVE_TO_FILE:
         SaveMessagesToFile(GetSelections());
         break;

      case WXMENU_MSG_REPLY:
      case WXMENU_MSG_FOLLOWUP:
         {
            int flags = cmd == WXMENU_MSG_FOLLOWUP ? MailFolder::REPLY_FOLLOWUP
                                                   : 0;

            UIdArray selections = GetSelections();

            m_TicketList->Add(
               m_ASMailFolder->ReplyMessages(
                                             &selections,
                                             MailFolder::Params(templ, flags),
                                             m_Frame,
                                             this
                                            )
            );
         }
         break;

      case WXMENU_MSG_FORWARD:
         {
            UIdArray selections = GetSelections();

            m_TicketList->Add(
                  m_ASMailFolder->ForwardMessages(
                                                  &selections,
                                                  MailFolder::Params(templ),
                                                  m_Frame,
                                                  this
                                                 )
               );
         }
         break;

      case WXMENU_MSG_QUICK_FILTER:
         // create a filter for the (first of) currently selected message(s)
         m_TicketList->Add(
               m_ASMailFolder->GetMessage(GetFocus(), this)
            );
         break;

      case WXMENU_MSG_FILTER:
         {
            UIdArray selections = GetSelections();

            m_TicketList->Add(
                  m_ASMailFolder->ApplyFilterRules(&selections, this)
               );
         }
         break;

      case WXMENU_MSG_UNDELETE:
         {
            UIdArray selections = GetSelections();

            m_TicketList->Add(
                  m_ASMailFolder->UnDeleteMessages(&selections, this)
               );
         }
         break;

      case WXMENU_MSG_DELETE:
         DeleteOrTrashMessages(GetSelections());
         break;

      case WXMENU_MSG_FLAG:
         ToggleMessages(GetSelections());
         break;

      case WXMENU_MSG_NEXT_UNREAD:
         m_FolderCtrl->SelectNextUnread();
         break;

      case WXMENU_MSG_PRINT:
         PrintMessages(GetSelections());
         break;

#ifdef USE_PS_PRINTING
      case WXMENU_MSG_PRINT_PS:
         //FIXME PrintMessages(GetSelections());
         break;
#endif // USE_PS_PRINTING

      case WXMENU_MSG_PRINT_PREVIEW:
         PrintPreviewMessages(GetSelections());
         break;

      case WXMENU_MSG_SELECTUNREAD:
         SelectAllUnread();
         break;

      case WXMENU_MSG_SELECTALL:
         SelectAll(true);
         break;

      case WXMENU_MSG_DESELECTALL:
         SelectAll(false);
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(MH_FOLDER_VIEW,GetWindow());
         break;

      case WXMENU_FILE_COMPOSE_WITH_TEMPLATE:
      case WXMENU_FILE_COMPOSE:
         {
            wxFrame *frame = m_Frame;

            wxString templ;
            if ( cmd == WXMENU_FILE_COMPOSE_WITH_TEMPLATE )
            {
               templ = ChooseTemplateFor(MessageTemplate_NewMessage, frame);
               if ( templ.empty() )
               {
                  // cancelled by user
                  break;
               }
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
         // this is probably not the way it should be done, but I don't know
         // how to get the folder to do all this synchronously otherwise -
         // and I don't think this operation gains anything from being async
         if ( m_MessagePreview )
         {
            CHECK_RET( m_ASMailFolder, "message preview without folder?" );

            MailFolder *mf = m_ASMailFolder->GetMailFolder();
            CHECK_RET( mf, "message preview without folder?" );

            const UIdArray& selections = GetSelections();

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
                                             (MFrame *)m_Frame);
            }
         }
         break;

#ifdef EXPERIMENTAL_show_uid
      case WXMENU_MSG_SHOWUID:
         {
            MailFolder *mf = m_ASMailFolder->GetMailFolder();

            String uidString = mf->GetMessageUID(GetFocus());
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
            wxFolderMenu *menu = m_FolderCtrl->GetFolderMenu();
            if ( menu )
            {
               MFolder *folder = menu->GetFolder(cmd);
               if ( folder )
               {
                  // it is, move messages there (it is "quick move" menu)
                  SaveMessagesToFolder(GetSelections(), folder, true /* move */);

                  folder->DecRef();
               }
            }
         }
         break;
      }
}

// ----------------------------------------------------------------------------
// selection/focus management
// ----------------------------------------------------------------------------

void
wxFolderView::OnFocusChange(void)
{
   long idx;
   UIdType uid = m_FolderCtrl->GetFocusedUId(&idx);

   if ( uid != m_uidFocused )
   {
      wxLogTrace(M_TRACE_SELECTION, "item %ld (uid = %lx) is now focused",
                 idx, uid);

      m_uidFocused = uid;

      if ( uid != UID_ILLEGAL && READ_CONFIG(m_Profile, MP_FVIEW_STATUS_UPDATE) )
      {
         String msg;
         if ( m_uidPreviewed != UID_ILLEGAL )
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

         wxLogStatus(m_Frame, msg);
      }
      //else: no status message
   }
}

bool
wxFolderView::HasSelection() const
{
   return m_FolderCtrl->HasSelection();
}

void
wxFolderView::SetPreviewUID(UIdType uid)
{
   wxLogTrace(M_TRACE_SELECTION, "%lx is now previewed", uid);

   m_uidPreviewed = uid;
}

UIdArray
wxFolderView::GetSelections() const
{
   return m_FolderCtrl->GetSelectionsOrFocus();
}

UIdType
wxFolderView::GetFocus() const
{
   return m_FolderCtrl->GetFocusedUId();
}

// ----------------------------------------------------------------------------
// operations on messages
// ----------------------------------------------------------------------------

void
wxFolderView::ShowRawText(long uid)
{
   // do it synchrnously (FIXME we shouldn't!)
   String text;
   Message *msg = m_MailFolder->GetMessage(uid);
   if ( msg )
   {
      msg->WriteToString(text, true);
      msg->DecRef();
   }

   if ( text.IsEmpty() )
   {
      wxLogError(_("Failed to get the raw text of the message."));
   }
   else
   {
      MDialog_ShowText(m_Parent, _("Raw message text"), text, "RawMsgPreview");
   }
}

void
wxFolderView::PreviewMessage(long uid)
{
   if ( (unsigned long)uid != m_uidPreviewed )
   {
      // select the item we preview in the folder control
      m_FolderCtrl->OnPreview();

      // show it in the preview window
      m_MessagePreview->ShowMessage(m_ASMailFolder, uid);

      // remember which item we preview
      SetPreviewUID(uid);
   }
}

void
wxFolderView::OpenMessages(const UIdArray& selections)
{
   if ( !selections.Count() )
      return;

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

   if ( n == 1 )
   {
      m_MessagePreview->Print();
   }
   else if ( n != 0 )
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
   if ( !selections.Count() )
      return;

   int n = selections.Count();

   if(n == 1)
   {
      m_MessagePreview->PrintPreview();
   }
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
wxFolderView::DeleteOrTrashMessages(const UIdArray& selections)
{
   if ( !selections.Count() )
      return;

   wxStatusProgress(m_Frame, _("Deleting messages..."));

   Ticket t = m_ASMailFolder->DeleteOrTrashMessages(&selections, this);
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
      bool flagged = (hil->GetItemByIndex(idx)->GetStatus() &
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
   size_t count = selections.GetCount();
   if ( !count )
      return ILLEGAL_TICKET;

   if ( !folder )
   {
      folder = MDialog_FolderChoose(GetFrame(m_Parent));
      if ( !folder )
         return ILLEGAL_TICKET;
   }
   else
   {
      folder->IncRef(); // to match DecRef() below
   }

   wxStatusProgress(m_Frame, _("Saving %d message(s) to '%s'..."),
                    count, folder->GetFullName().c_str());

   Ticket t = m_ASMailFolder->SaveMessagesToFolder(&selections,
                                                   m_Frame,
                                                   folder,
                                                   this);
   m_TicketList->Add(t);
   if ( del )
   {
      // also don't forget to delete messages once they're successfulyl saved
      m_TicketsToDeleteList->Add(t);
   }

   folder->DecRef();

   return t;
}

void
wxFolderView::DropMessagesToFolder(const UIdArray& selections, MFolder *folder)
{
   if ( !selections.Count() )
      return;

   wxLogTrace(M_TRACE_DND, "Saving %d message(s) to folder '%s'",
              selections.Count(), folder->GetFullName().c_str());

   Ticket t = SaveMessagesToFolder(selections, folder);

   if ( !m_TicketsDroppedList )
   {
      wxLogTrace(M_TRACE_DND, "Creating m_TicketsDroppedList");

      m_TicketsDroppedList = ASTicketList::Create();
   }

   m_TicketsDroppedList->Add(t);
}

void
wxFolderView::SaveMessagesToFile(const UIdArray& selections)
{
   size_t count = selections.Count();
   if ( !count )
      return;

   wxStatusProgress(m_Frame, _("Saving %d message(s) to file..."), count);

   bool rc = m_ASMailFolder->SaveMessagesToFile(&selections,
                                                m_Frame,
                                                this) != 0;

   String msg;
   if ( rc )
      msg.Printf(_("%d messages saved"), count);
   else
      msg.Printf(_("Saving messages failed."));

   wxLogStatus(m_Frame, msg);
}

void wxFolderView::OnFolderClosedEvent(MEventFolderClosedData& event)
{
   if ( event.GetFolder() == m_MailFolder )
   {
      SetFolder(NULL);
   }
}

void wxFolderView::OnFolderDeleteEvent(const String& folderName)
{
   if ( folderName == m_folderName )
   {
      // assume we're in a folder view frame
      wxLogStatus(GetFrame(m_Parent),
                  _("Closing folder '%s' because the underlying mail "
                    "folder was deleted."), m_folderName.c_str());

      SetFolder(NULL);
   }
}

// this function is called when messages are deleted from folder but no new
// ones appear
void
wxFolderView::OnFolderExpungeEvent(MEventFolderExpungeData &event)
{
   if ( (event.GetFolder() == m_MailFolder) )
   {
      // if we had exactly one message selected before, we want to keep the
      // selection after expunging
      bool hadUniqueSelection = m_FolderCtrl->GetUniqueSelection() != -1;

      // we might have to clear the preview if we delete the message
      // being previewed
      bool previewDeleted = false;

#ifdef BROKEN_LISTCTRL
      // wxGTK doesn't seem to keep focus correctly itself when we delete items,
      // help it
      long focus = m_FolderCtrl->GetFocusedItem();
      bool focusDeleted = focus == -1;
#endif // BROKEN_LISTCTRL

      size_t count = event.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         long item = event.GetItem(n);

#ifdef BROKEN_LISTCTRL
         if ( !focusDeleted )
         {
            if ( item < focus )
            {
               focus--;
            }
            else if ( item == focus )
            {
               focusDeleted = true;
            }
         }
#endif // BROKEN_LISTCTRL

         UIdType uid = m_FolderCtrl->GetUIdFromIndex(item);

         if ( uid == m_uidPreviewed )
            previewDeleted = true;

         m_FolderCtrl->DeleteItem(item);
      }

#ifdef BROKEN_LISTCTRL
      // restore focus if we had it
      if ( focusDeleted && (focus != -1) )
      {
         // take the next item as focus, if there is any - otherwise the last
         long itemMax = m_FolderCtrl->GetItemCount() - 1;
         if ( focus > itemMax )
            focus = itemMax;
      }

      // even if it wasn't deleted it might have changed because items before it
      // were deleted
      m_FolderCtrl->Focus(focus);
      OnFocusChange();
#endif // BROKEN_LISTCTRL

      // clear preview window if the message showed there had been deleted
      if ( previewDeleted )
      {
         m_MessagePreview->Clear();
         InvalidatePreviewUID();
      }

      if ( hadUniqueSelection )
      {
         // restore the selection
         m_FolderCtrl->SelectFocused();
      }

      // update the count of messages and deleted messages (of which there
      // shouldn't be left any)
      m_NumOfMessages -= count;
      m_nDeleted = 0;
   }
}

// this function gets called when new mail appears in the folder
void
wxFolderView::OnFolderUpdateEvent(MEventFolderUpdateData &event)
{
   if ( event.GetFolder() == m_MailFolder )
   {
      Update();
   }
}

// this is called when the status of one message changes
void
wxFolderView::OnMsgStatusEvent(MEventMsgStatusData &event)
{
   if ( event.GetFolder() == m_MailFolder )
   {
      size_t index = event.GetIndex();
      if ( index < (size_t)m_FolderCtrl->GetItemCount() )
      {
         const HeaderInfo *hi = event.GetHeaderInfo();
         int status = hi->GetStatus();
         String strStatus =
            MailFolder::ConvertMessageStatusToString(status);

         m_FolderCtrl->SetEntryStatus(index, strStatus);

         SetEntryColour(index, hi);

         if ( status & MailFolder::MSG_STAT_DELETED )
         {
            // remember that we have another deleted message
            m_nDeleted++;
         }

         UpdateTitleAndStatusBars("", "", m_Frame, m_MailFolder);
      }
      //else: this can happen if we didn't have to update the control yet, just
      //      ignore the event then as we will get the message with the
      //      correct status when we retrieve it from Update() anyhow
   }
}

bool
wxFolderView::DragAndDropMessages()
{
   bool didDrop = false;

   const UIdArray& selections = GetSelections();
   size_t countSel = selections.Count();
   if ( countSel > 0 )
   {
      MailFolder *mf = GetFolder()->GetMailFolder();
      CHECK( mf, false, "no mail folder to drag messages from?" );

      MMessagesDataObject dropData(this, mf, selections);

      // setting up the dnd icons can't be done in portable way :-(
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

      switch ( dropSource.DoDragDrop(true /* allow move */) )
      {
         default:
            wxFAIL_MSG( "unexpected DoDragDrop return value" );
            // fall through

         case wxDragError:
            // always give this one in debug mode, it is not supposed to
            // happen!
            wxLogDebug("An error occured during drag and drop operation");
            break;

         case wxDragNone:
         case wxDragCancel:
            wxLogTrace(M_TRACE_DND, "Drag and drop aborted by user.");
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

                  wxLogTrace(M_TRACE_DND, "Deleted previously dropped msgs.");
               }
               else
               {
                  // maybe we didn't have time to really copy the messages
                  // yet, then they will be deleted later
                  wxLogTrace(M_TRACE_DND, "Dropped msgs will be deleted later");
               }
            }
            break;

         case wxDragCopy:
            // nothing special to do
            break;
      }

      if ( m_TicketsDroppedList )
      {
         didDrop = true;

         m_TicketsDroppedList->DecRef();
         m_TicketsDroppedList = NULL;

         wxLogTrace(M_TRACE_DND, "DragAndDropMessages() done ok");
      }
      else
      {
         wxLogTrace(M_TRACE_DND, "Nothing dropped");
      }

      if ( !m_UIdsCopiedOk.IsEmpty() )
      {
         m_UIdsCopiedOk.Empty();
      }

      mf->DecRef();
   }

   // did we do anything?
   return didDrop;
}

void
wxFolderView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   String msg;
   ASMailFolder::Result *result = event.GetResult();
   const Ticket& t = result->GetTicket();

   if ( m_TicketList->Contains(t) )
   {
      ASSERT_MSG( result->GetUserData() == this, "got unexpected result" );

      m_TicketList->Remove(t);

      int value = ((ASMailFolder::ResultInt *)result)->GetValue();

      switch ( result->GetOperation() )
      {
         case ASMailFolder::Op_SaveMessagesToFile:
            if ( value )
            {
               msg.Printf(_("Saved %lu messages."), (unsigned long)
                          result->GetSequence()->Count());
            }
            else
            {
               msg.Printf(_("Saving messages failed."));
            }

            wxLogStatus(m_Frame, msg);
            break;

         case ASMailFolder::Op_SaveMessagesToFolder:
            ASSERT(result->GetSequence());

            // we may have to do a few extra things here:
            //
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

               // remove from lists before testing value: this should be done
               // whether we succeeded or failed
               if ( toDelete )
               {
                  m_TicketsToDeleteList->Remove(t);
               }

               if ( wasDropped )
               {
                  m_TicketsDroppedList->Remove(t);

                  wxLogTrace(M_TRACE_DND, "Dropped msgs copied ok");
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
               else // success
               {
                  // message was copied ok, what else to do with it?
                  UIdArray *seq = result->GetSequence();
                  unsigned long count = seq->Count();

                  if ( wasDropped )
                  {
                     if ( !toDelete )
                     {
                        // remember the UIDs as we may have to delete them later
                        // even if they are not in m_TicketsToDeleteList yet
                        WX_APPEND_ARRAY(m_UIdsCopiedOk, (*seq));
                     }
                     //else: dropped and already marked for deletion, delete below

                     msg.Printf(_("Dropped %lu messages."), count);
                  }
                  //else: not dropped

                  if ( toDelete )
                  {
                     // delete right now
                     m_TicketList->Add(
                           m_ASMailFolder->DeleteOrTrashMessages(seq, this));

                     if ( !wasDropped )
                     {
                        msg.Printf(_("Moved %lu messages."), count);
                     }
                     //else: message already given above
                  }
                  else // simple copy, not move
                  {
                     msg.Printf(_("Copied %lu messages."), count);
                  }
               }

               wxLogStatus(m_Frame, msg);
            }
            break;

         case ASMailFolder::Op_SearchMessages:
            ASSERT(result->GetSequence());
            if( value )
            {
               UIdArray *uidsMatching = result->GetSequence();
               if ( !uidsMatching )
               {
                  FAIL_MSG( "searched ok but no search results??" );
                  break;
               }

               unsigned long count = uidsMatching->Count();

               wxFolderListCtrlBlockOnSelect dontHandleOnSelect(m_FolderCtrl);

               /*
                  The returned message numbers are UIds which we must map
                  to our listctrl indices via the current HeaderInfo
                  structure.

                  VZ: I wonder why do we jump through all these hops - we might
                      return msgnos from search directly...
                */
               HeaderInfoList_obj hil = GetFolder()->GetHeaders();
               for ( unsigned long n = 0; n < uidsMatching->Count(); n++ )
               {
                  UIdType idx = hil->GetIdxFromUId((*uidsMatching)[n]);
                  if ( idx != UID_ILLEGAL )
                  {
                     m_FolderCtrl->Select(hil->GetPosFromIdx(idx));
                  }
                  else
                  {
                     FAIL_MSG( "found inexistent message??" );
                  }
               }

               msg.Printf(_("Found %lu messages."), count);
            }
            else
            {
               msg.Printf(_("No matching messages found."));
            }

            wxLogStatus(m_Frame, msg);
            break;

         case ASMailFolder::Op_ApplyFilterRules:
         {
            ASSERT(result->GetSequence());
            if ( value == -1 )
            {
               wxLogError(_("Filtering messages failed."));
            }
            else
            {
               msg.Printf(_("Applied filters to %lu messages, "
                            "see log window for details."),
                          (unsigned long)result->GetSequence()->Count());
               wxLogStatus(m_Frame, msg);
            }
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

                  String to;
                  (void)msg->GetHeaderLine("To", to);

                  if ( CreateQuickFilter(folder,
                                         msg->From(), msg->Subject(), to,
                                         m_Frame) )
                  {
                     // ask the user if he doesn't want to test his new filter
                     // right now
                     if ( MDialog_YesNoDialog
                          (
                           _("Would you like to apply the filter you have just "
                             "created to this message immediately?"),
                           m_Frame,
                           MDIALOG_YESNOTITLE,
                           true,
                           GetFullPersistentKey(M_MSGBOX_APPLY_QUICK_FILTER_NOW)
                          ) )
                     {
                        UIdArray selections;
                        selections.Add(msg->GetUId());
                        m_TicketList->Add(
                              m_ASMailFolder->ApplyFilterRules(&selections, this)
                           );
                     }
                  }
                  //else: filter not created, nothing to apply

                  msg->DecRef();
               }
            }
            break;

         default:
            FAIL_MSG( "MEvent handling not implemented yet" );
      }
   }
   //else: not out result at all

   result->DecRef();
}

// ----------------------------------------------------------------------------
// wxFolderViewFrame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
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

wxFolderViewFrame::wxFolderViewFrame(const String &name, wxMFrame *parent)
   : wxMFrame(name,parent)
{
   m_FolderView = NULL;

   // menu
   AddFileMenu();
   AddEditMenu();

   // no cut/paste for viewer
   wxMenuBar *menuBar = GetMenuBar();
   menuBar->Enable(WXMENU_EDIT_CUT, false);
   menuBar->Enable(WXMENU_EDIT_CUT, false);

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

IMPLEMENT_DYNAMIC_CLASS(wxFolderViewFrame, wxMFrame)

// ----------------------------------------------------------------------------
// other public functions (from include/FolderView.h)
// ----------------------------------------------------------------------------

bool OpenFolderViewFrame(const String& name, wxWindow *parent)
{
   return wxFolderViewFrame::Create(name, (wxMFrame *)GetFrame(parent)) != NULL;
}

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
                showSize == other.showSize &&
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

