///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxFolderView.cpp - window with folder view inside
// Purpose:     wxFolderView is used to show to the user folder contents
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by: VZ at 13.07.01: use virtual list control, update on demand
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

#  include "Sorting.h"
#  include "Threading.h"

#  include <wx/dynarray.h>
#  include <wx/colour.h>
#  include <wx/menu.h>

#  include <wx/stattext.h>

#  include <wx/sizer.h>
#else
#  include <wx/imaglist.h>
#endif // USE_PCH

#include <ctype.h>
#include "XFace.h"

#include <wx/file.h>
#include <wx/listctrl.h>
#include <wx/menuitem.h>
#include <wx/dnd.h>
#include <wx/fontmap.h>
#include <wx/encconv.h>
#include "wx/persctrl.h"

#include "MThread.h"
#include "MFolder.h"
#include "MFCache.h"

#include "FolderView.h"
#include "MailFolder.h"
#include "HeaderInfo.h"
#include "ASMailFolder.h"
#include "MessageView.h"
#include "TemplateDialog.h"
#include "Composer.h"
#include "MsgCmdProc.h"

#include "Sequence.h"

#include "gui/wxFolderView.h"
#include "gui/wxFolderMenu.h"
#include "gui/wxFiltersDialog.h" // for ConfigureFiltersForFolder()
#include "MFolderDialogs.h"      // for ShowFolderPropertiesDialog
#include "Mdnd.h"

#include "gui/wxMIds.h"
#include "MDialogs.h"
#include "MHelp.h"
#include "miscutil.h"            // for UpdateTitleAndStatusBars

// use XPMs under MSW as well as it's the simplest way to have transparent
// bitmaps like we need here
#include "../icons/sortdown.xpm"
#include "../icons/sortup.xpm"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOSHOW_FIRSTMESSAGE;
extern const MOption MP_AUTOSHOW_FIRSTUNREADMESSAGE;
extern const MOption MP_DATE_FMT;
extern const MOption MP_DATE_GMT;
extern const MOption MP_FLC_DATECOL;
extern const MOption MP_FLC_FROMCOL;
extern const MOption MP_FLC_SIZECOL;
extern const MOption MP_FLC_STATUSCOL;
extern const MOption MP_FLC_SUBJECTCOL;
extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_FROM_REPLACE_ADDRESSES;
extern const MOption MP_FVIEW_AUTONEXT_UNREAD_MSG;
extern const MOption MP_FVIEW_BGCOLOUR;
extern const MOption MP_FVIEW_DELETEDCOLOUR;
extern const MOption MP_FVIEW_FGCOLOUR;
extern const MOption MP_FVIEW_FLAGGEDCOLOUR;
extern const MOption MP_FVIEW_FONT;
extern const MOption MP_FVIEW_FONT_SIZE;
extern const MOption MP_FVIEW_FROM_REPLACE;
extern const MOption MP_FVIEW_NAMES_ONLY;
extern const MOption MP_FVIEW_NEWCOLOUR;
extern const MOption MP_FVIEW_PREVIEW_DELAY;
extern const MOption MP_FVIEW_RECENTCOLOUR;
extern const MOption MP_FVIEW_SIZE_FORMAT;
extern const MOption MP_FVIEW_STATUS_FMT;
extern const MOption MP_FVIEW_STATUS_UPDATE;
extern const MOption MP_FVIEW_UNREADCOLOUR;
extern const MOption MP_MSGS_SORTBY;
extern const MOption MP_MSGS_USE_THREADING;
extern const MOption MP_MSGVIEW_SHOWBAR;
extern const MOption MP_MSGVIEW_VIEWER;
extern const MOption MP_PREVIEW_ON_SELECT;
extern const MOption MP_USERLEVEL;
extern const MOption MP_USE_TRASH_FOLDER;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the ids of commands in the listctrl header popup menu
enum
{
   WXMENU_FVIEW_POPUP_BEGIN = 3000,
   WXMENU_FVIEW_CONFIG_SORT = WXMENU_FVIEW_POPUP_BEGIN,
   WXMENU_FVIEW_RESET_SORT,
   WXMENU_FVIEW_TOGGLE_THREAD,
   WXMENU_FVIEW_CONFIG_THREAD,

   // must be in the same order as MessageSizeShow enum members
   WXMENU_FVIEW_SIZE_AUTO,
   WXMENU_FVIEW_SIZE_AUTOBYTES,
   WXMENU_FVIEW_SIZE_BYTES,
   WXMENU_FVIEW_SIZE_KBYTES,
   WXMENU_FVIEW_SIZE_MBYTES,

   WXMENU_FVIEW_CONFIG_DATEFMT,
   WXMENU_FVIEW_FROM_NAMES_ONLY,

   // should be in the same order as wxFolderListColumn enum members
   WXMENU_FVIEW_SORT_BY_COL,
   WXMENU_FVIEW_SORT_BY_STATUS = WXMENU_FVIEW_SORT_BY_COL,
   WXMENU_FVIEW_SORT_BY_DATE,
   WXMENU_FVIEW_SORT_BY_SIZE,
   WXMENU_FVIEW_SORT_BY_FROM,
   WXMENU_FVIEW_SORT_BY_SUBJECT,

   WXMENU_FVIEW_SORT_BY_COL_REV,
   WXMENU_FVIEW_SORT_BY_STATUS_REV = WXMENU_FVIEW_SORT_BY_COL_REV,
   WXMENU_FVIEW_SORT_BY_DATE_REV,
   WXMENU_FVIEW_SORT_BY_SIZE_REV,
   WXMENU_FVIEW_SORT_BY_FROM_REV,
   WXMENU_FVIEW_SORT_BY_SUBJECT_REV,

   WXMENU_FVIEW_POPUP_END
};

static const char *wxFLC_ColumnNames[WXFLC_NUMENTRIES] =
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

// the trace mask for selection/focus handling
#define M_TRACE_FV_SELECTION "msgsel"

// the trace mask folder view events handling tracing
#define M_TRACE_FV_UPDATE    "fvupdate"

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
         *value = MailFolder::SizeToString(m_hi->GetSize());
#ifdef USE_HEADER_SCORE
      else if ( name == "score" )
         *value = m_hi->GetScore();
#endif // USE_HEADER_SCORE
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
// wxFolderMsgWindow: the window containing the message viewer
// ----------------------------------------------------------------------------

// NB: we have a separate window because the message viewer (and hence its
//     viewer) may change during our lifetime which is not very convenient
//     to handle in the code and, worse, provokes flicker when we call
//     wxSplitterWindow::ReplaceWindow() (mostly under MSW)
//
//     it also allows us to hook into the kbd processing and forward it to
//     wxFolderView

class wxFolderMsgWindow : public wxPanel
{
public:
   wxFolderMsgWindow(wxWindow *parent, wxFolderView *fv);
   virtual ~wxFolderMsgWindow();

   // called when the old viewer window is about to be destroyed and replaced
   // with the new one
   void SetViewerWindow(wxWindow *winViewerNew);

   // called when the folder we view changes
   void UpdateOptions();

   // don't get the focus from keyboard, we don't normally need it
   virtual bool AcceptsFocusFromKeyboard() const { return false; }

protected:
   // the event handlers
   void OnSize(wxSizeEvent& event);
   void OnButton(wxCommandEvent& event);
   void OnChoice(wxCommandEvent& event);

   // resize our own child to fill the entire window
   void Resize();

   // create/delete the controls we put above the viewer window
   void CreateViewerBar();
   void DeleteViewerBar();

   // update the state of the viewer bar
   void UpdateViewerBar();

private:
   // the associated folder view (never NULL)
   wxFolderView *m_folderView;

   // the current viewer window (may be NULL)
   wxWindow *m_winViewer;

   // the viewer bar: contains the controls we put above the viewer
   wxPanel *m_winBar;

   // the array containing the names of all the existing viewers
   wxArrayString m_namesViewers;

   // the event handler to hook the kbd input (NULL initially, !NULL later)
   class wxFolderMsgViewerEvtHandler *m_evtHandlerMsgView;

   // the event table ids
   enum
   {
      Button_Close = 100,
      Choice_Viewer
   };

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxFolderMsgViewerEvtHandler: helper of wxFolderMsgWindow
// ----------------------------------------------------------------------------

// this class intercepts key events in the viewer window and forwards them to
// the folder view
class wxFolderMsgViewerEvtHandler : public wxEvtHandler
{
public:
   wxFolderMsgViewerEvtHandler(wxFolderView *fv) { m_folderView = fv; }

protected:
   void OnChar(wxKeyEvent& event);

   wxFolderView *m_folderView;

private:
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxFolderListCtrl: the list ctrl showing the messages in the folder
// ----------------------------------------------------------------------------

class wxFolderListCtrl : public wxListView
{
public:
   /**
     Ctor and dtor
    */
   //@{

   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   virtual ~wxFolderListCtrl();

   //@}

   /**
     Listing stuff
    */
   //@{

   /// set the listing to use
   void SetListing(HeaderInfoList *listing);

   /// called when the number of items in the listing changes
   void UpdateListing(HeaderInfoList *headers);

   /// do we have headers at all?
   bool HasHeaders() const { return m_headers != NULL; }

   /// get the number of items we show
   size_t GetHeadersCount() const { return m_headers ? m_headers->Count() : 0; }

   /// invalidate the cached header(s)
   void InvalidateCache();

   //@}

   /**
     Column functions
    */
   //@{

   /// create the columns using order in m_columns and widths from profile
   void CreateColumns();

   /// return the string containing ':' separated columns widths
   String GetWidths() const;

   //@}

   /**
     Selection, focus &c.

     Note that when selecting a new item programmatically, GoToItem() should be
     used instead of directly Select()ing the item: GoToItem() maintains the
     single selection invariant, i.e. if the control had a unique item selected
     before it, it will unselect the previously selected item and select the
     new one, otherwise it will just add the new one to the selection. If this
     sounds complicated, just try it out and you'll see that this is exactly
     what we want to happen from the UI point of view.
    */
   //@{

   /**
     Focuses and possibly selects the given item and unselects the previously
     selected one if it was the only item selected. The specified item will be
     selected if we had exactly one selected item before, just focused
     otherwise.

     @param item the item to focus and select
     @return true if we selected this item, false if only focused it
    */
   bool GoToItem(long item);

   /** select the next (after the current one) message in the control which
       has the given status bit on or off, depending on the second parameter

       @param status the status bit to check
       @param isSet go to message with status bit set if true, unset if false
       @return true if we found such item
   */
   bool SelectNextByStatus(MailFolder::MessageStatus status, bool isSet);

   /**
     Select next unread or flagged message after the given one.

     It will always focus the message it found, if any.

     @param indexStart the index after which to look (exclusive)
     @param status the status bit to look for
     @param isSet if true, check that status bit is set, otherwise - cleared
     @return false if no such messages were found (and focus wasn't changed)
    */
   bool SelectNextUnreadAfter(long indexStart = -1,
                              MailFolder::MessageStatus status =
                                 MailFolder::MSG_STAT_SEEN,
                              bool isSet = FALSE);

   /// return true if we preview this item
   bool IsPreviewed(long item) const
      { return item == m_itemPreviewed; }

   /// get the item being previewed
   long GetPreviewedItem() const { return m_itemPreviewed; }

   /// return true if we preview the item with this UID
   bool IsUIdPreviewed(UIdType uid) const
      { return uid == m_uidPreviewed; }

   /// get the UID currently being viewed (may be UID_ILLEGAL)
   UIdType GetPreviewUId() const { return m_uidPreviewed; }

   /// get the UID of the given item
   UIdType GetUIdFromIndex(long item) const
   {
      CHECK( (size_t)item < GetHeadersCount(), UID_ILLEGAL,
             "invalid listctrl index" );

      return m_headers->GetItem((size_t)item)->GetUId();
   }

   /// get the UID and, optionally, the index of the focused item
   UIdType GetFocusedUId(long *idx = NULL) const;

   /// get the only selected item, return -1 if 0 or >= 2 items are selected
   long GetUniqueSelection() const;

   /// get the selected items (use the focused one if no selection)
   UIdArray GetSelectionsOrFocus() const;

   /// return true if we have either selection or valid focus
   bool HasSelection() const;

   /// return the position of the given UID in the control or -1
   long GetPosFromUID(UIdType uid);

   //@}

   /// @name the event handlers
   //@{
   void OnSelected(wxListEvent& event);
   void OnColumnClick(wxListEvent& event);
   void OnColumnRightClick(wxListEvent& event);
   void OnListKeyDown(wxListEvent& event);
   void OnActivated(wxListEvent& event);

   void OnChar(wxKeyEvent &event);

   void OnRightClick(wxMouseEvent& event);
   void OnDoubleClick(wxMouseEvent &event);
   void OnMouseMove(wxMouseEvent &event);

   void OnCommandEvent(wxCommandEvent& event)
      { m_FolderView->OnCommandEvent(event); }

   void OnPreviewTimer(wxTimerEvent& event);

   void OnIdle(wxIdleEvent& event);
   //@}

   /// tell us that the folder we're viewing has changed
   void OnFolderChange();

   /// change the options governing our appearance
   void ApplyOptions(const wxColour& fg, const wxColour& bg,
                     int fontFamily, int fontSize,
                     int columns[WXFLC_NUMENTRIES]);

   /// remember that we preview this message, return true if != old one
   bool SetPreviewMsg(long idx, UIdType uid);

   /// forget about the message we were previewing
   void InvalidatePreview();

   /// set m_PreviewOnSingleClick flag
   void SetPreviewOnSingleClick(bool flag) { m_PreviewOnSingleClick = flag; }

   /// set m_PreviewDelay value
   void SetPreviewDelay(unsigned long delay) { m_PreviewDelay = delay; }

   /// set the sort order to use (and notify everybody about it)
   void SetSortOrder(Profile *profile,
                     long sortOrder,
                     wxFolderListColumn col,
                     bool reverse);

   /// draw the sort direction arrow on the column used for sorting
   void UpdateSortIndicator();

   /// save the widths of the columns in profile if needed
   void SaveColWidths();

   /// update the info about focused item if it changed
   void UpdateFocus();

   /// for wxFolderView
   wxFolderMenu *GetFolderMenu() const { return m_menuFolders; }

protected:
   /// go to the next unread message or to the next folder
   void MoveToNextUnread()
   {
      m_FolderView->MoveToNextUnread();
   }

   /// preview this item in folder view
   void PreviewItem(long idx, UIdType uid);

   /// schedule this item for previewing after m_PreviewDelay expires
   void PreviewItemDelayed(long idx, UIdType uid);

   /// get the colour to use for this entry (depends on status)
   wxColour GetEntryColour(const HeaderInfo *hi) const;

   /// return information about the list ctrl items on demand
   virtual wxString OnGetItemText(long item, long column) const;
   virtual int OnGetItemImage(long item) const;
   virtual wxListItemAttr *OnGetItemAttr(long item) const;

   /// read the column width string from profile or default one
   wxString GetColWidths() const;

   /// enable/disable handling of select events (returns old value)
   bool EnableOnSelect(bool enable)
   {
      bool enableOld = m_enableOnSelect;
      m_enableOnSelect = enable;
      return enableOld;
   }

   /// update our "unique selection" flag
   void UpdateUniqueSelFlag();

   /// update the number of items in the list control
   void UpdateItemCount() { SetItemCount(GetHeadersCount()); }

   /// do we have a folder opened?
   bool HasFolder() const { return m_FolderView->GetFolder() != NULL; }

   /// get the folder view settings we use
   const wxFolderView::AllProfileSettings& GetSettings() const
      { return m_FolderView->m_settings; }

   /**
     Header retrieving data
    */
   //@{

   /// get the header info for the header at this position
   HeaderInfo *GetHeaderInfo(size_t index) const;

   /// the listing to use
   HeaderInfoList *m_headers;

   /// are we inside a call to some HeaderInfoList method?
   MMutex m_mutexHeaders;

   /// cached header info (used by OnGetItemXXX())
   HeaderInfo *m_hiCached;

   /// the index of m_hiCached in m_headers (or -1 if not cached)
   size_t m_indexHI;

   /// the last headers list modification "date"
   HeaderInfoList::LastMod m_cacheLastMod;

   /// the positions of the headers we need to get
   wxArrayInt m_headersToGet;

   //@}

   // TODO: we should have the standard attributes for each of the possible
   //       message states/colours (new/recent/unread/flagged/deleted) and
   //       reuse them in OnGetItemAttr(), this should be quite faster

   /// cached attribute
   wxListItemAttr *m_attr;

   /// the associated folder view
   wxFolderView *m_FolderView;

   /**
     @name Currently selected item data

     We need to store the focused item to be able to detect when it changes
     and its UID to be able to refocus it after the sort order change.
    */
   //@{

   /// uid of the currently focused item or UID_ILLEGAL
   UIdType m_uidFocus;

   /// uid of the item currently previewer or UID_ILLEGAL (!= m_uidFocus)
   UIdType m_uidPreviewed;

   /// the currently focused item
   long m_itemFocus;

   /// the item currently previewed in the folder view
   long m_itemPreviewed;

   /// 1 => we have exactly 1 sel item, 0 - more, -1 - none at all
   int m_selIsUnique;

   //@}

   /// column order
   int m_columns[WXFLC_NUMENTRIES];

   /// string containing current column widths
   wxString m_widthsOld;

   /// the column where the sort indicator is currently drawn or WXFLC_NONE
   wxFolderListColumn m_colSort;

   /// do we preview a message on a single mouse click?
   bool m_PreviewOnSingleClick;

   /// delay between selecting a message and previewing it
   unsigned long m_PreviewDelay;

   /// the item which will be previewed once m_PreviewDelay expires
   long m_itemDelayed;

   /// uid of m_itemDelayed
   UIdType m_uidDelayed;

   /// timer used for delayed previewing
   wxTimer m_timerPreview;

   /// do we handle OnSelected()?
   bool m_enableOnSelect;

   /// the popup menu
   wxMenu *m_menu;

   /// and the folder submenu for it
   wxFolderMenu *m_menuFolders;

   /// flag to prevent reentrancy in OnRightClick()
   bool m_isInPopupMenu;

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
// private functions
// ----------------------------------------------------------------------------

// return the n-th shown column (WXFLC_NONE if no more columns)
static wxFolderListColumn GetColumnByIndex(const int *columns, size_t n)
{
   size_t col;
   for ( col = 0; col < WXFLC_NUMENTRIES; col++ )
   {
      if ( columns[col] == (int)n )
         break;
   }

   // WXFLC_NONE == WXFLC_NUMENTRIES so the return value is always correct
   return (wxFolderListColumn)col;
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
static wxFolderListColumn GetColumnByName(const wxString& name)
{
   size_t n;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      if ( name == GetColumnName(n) )
         break;
   }

   return (wxFolderListColumn)n;
}

static wxFolderListColumn ColFromSortOrder(MessageSortOrder sortOrder)
{
   switch ( sortOrder )
   {
      case MSO_DATE:
         return WXFLC_DATE;

      case MSO_SENDER:
         return WXFLC_FROM;

      case MSO_SUBJECT:
         return WXFLC_SUBJECT;

      case MSO_SIZE:
         return WXFLC_SIZE;

      case MSO_STATUS:
         return WXFLC_STATUS;

      default:
         wxFAIL_MSG( "invalid column" );
         // fall through

      case MSO_NONE:
         return WXFLC_NONE;
   }
}

static MessageSortOrder SortOrderFromCol(wxFolderListColumn col)
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

// check if the status bit is set/unset
inline bool CheckStatusBit(int status, int bit, bool isSet)
{
   return !(status & bit) == !isSet;
}

// ============================================================================
// wxFolderMsgWindow and wxFolderMsgViewerEvtHandler implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFolderMsgViewerEvtHandler, wxEvtHandler)
   EVT_CHAR(wxFolderMsgViewerEvtHandler::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderMsgWindow, wxWindow)
   EVT_SIZE(wxFolderMsgWindow::OnSize)

   EVT_BUTTON(wxFolderMsgWindow::Button_Close, wxFolderMsgWindow::OnButton)
   EVT_CHOICE(wxFolderMsgWindow::Choice_Viewer, wxFolderMsgWindow::OnChoice)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderMsgWindow
// ----------------------------------------------------------------------------

wxFolderMsgWindow::wxFolderMsgWindow(wxWindow *parent, wxFolderView *fv)
                 : wxPanel(parent, -1,
                            wxDefaultPosition, wxDefaultSize,
                            fv ? wxBORDER_NONE : wxBORDER_DEFAULT)
{
   m_winViewer = NULL;
   m_winBar = NULL;
   m_folderView = fv;
   m_winViewer = NULL;
   m_evtHandlerMsgView = NULL;
}

// called when the old viewer window is about to be destroyed and replaced
// with the new one
void wxFolderMsgWindow::SetViewerWindow(wxWindow *winViewerNew)
{
   if ( winViewerNew == m_winViewer )
      return;

   if ( m_winViewer )
   {
      m_winViewer->PopEventHandler(false /* don't delete it */);
   }
   else // we didn't have any viewer window before
   {
      if ( m_folderView->GetFolder() )
      {
         Profile_obj profile(m_folderView->GetFolderProfile());

         if ( READ_CONFIG(profile, MP_MSGVIEW_SHOWBAR) )
         {
            CreateViewerBar();
         }
      }
      //else: don't create the viewer bar if we don't have any folder
   }

   m_winViewer = winViewerNew;

   if ( m_winViewer )
   {
      if ( !m_evtHandlerMsgView )
      {
         m_evtHandlerMsgView = new wxFolderMsgViewerEvtHandler(m_folderView);
      }

      m_winViewer->PushEventHandler(m_evtHandlerMsgView);

      Resize();
   }
   else // we don't have any viewer any longer
   {
      if ( m_winBar )
      {
         DeleteViewerBar();
      }
      //else: we might not have created it
   }
}

wxFolderMsgWindow::~wxFolderMsgWindow()
{
   if ( m_winViewer )
   {
      m_winViewer->PopEventHandler(true /* delete it */);
   }
}

// ----------------------------------------------------------------------------
// wxFolderMsgWindow viewer bar stuff
// ----------------------------------------------------------------------------

void wxFolderMsgWindow::CreateViewerBar()
{
   ASSERT_MSG( !m_winBar, "recreating the viewer bar twice?" );

   // create the window and the sizer containing all controls
   m_winBar = new wxPanel(this, -1);

   wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

   // create the label
   wxStaticText *labelViewer = new wxStaticText(m_winBar, -1,
                                                _("Select the &viewer to use: "));

   sizer->Add(labelViewer, 0, wxALL | wxALIGN_CENTRE_VERTICAL, LAYOUT_X_MARGIN);

   // create the choice control for actually choosing the viewer and fill it
   // with all the available ones
   wxChoice *choice = new wxChoice(m_winBar, Choice_Viewer);

   wxArrayString descViewers;
   size_t count = MessageView::GetAllAvailableViewers(&m_namesViewers,
                                                      &descViewers);

   for ( size_t n = 0; n < count; n++ )
   {
      choice->Append(descViewers[n]);
   }

   choice->SetSize(choice->GetBestSize());

   UpdateViewerBar();

   sizer->Add(choice, 0, wxALL | wxALIGN_CENTRE_VERTICAL, LAYOUT_X_MARGIN);

   // add the spacer and the button at the far right to close this bar
   sizer->Add(5, 0, 1); // expandable

   wxBitmap bmp = mApplication->GetIconManager()->GetBitmap("tb_close");

#ifdef OS_WIN
   bmp.SetMask(new wxMask(bmp, *wxLIGHT_GREY));
#endif // OS_WIN

   wxBitmapButton *btnClose = new wxBitmapButton
                                  (
                                    m_winBar,
                                    Button_Close,
                                    bmp,
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxBORDER_NONE
                                  );
   btnClose->SetToolTip(_("Close this tool bar"));

   sizer->Add(btnClose, 0, wxALL | wxALIGN_CENTRE_VERTICAL, LAYOUT_X_MARGIN);

   // final initializations
   m_winBar->SetSizer(sizer);
   m_winBar->SetAutoLayout(TRUE);
   sizer->Fit(m_winBar);
}

void wxFolderMsgWindow::DeleteViewerBar()
{
   // harmless but stupid
   ASSERT_MSG( m_winBar, "deleting non existent viewer bar?" );

   delete m_winBar;
   m_winBar = NULL;
}

void wxFolderMsgWindow::UpdateViewerBar()
{
   Profile_obj profile(m_folderView->GetFolderProfile());

   wxString name = READ_CONFIG(profile, MP_MSGVIEW_VIEWER);
   int curViewer = m_namesViewers.Index(name);
   if ( curViewer != -1 )
   {
      wxChoice *choice = wxStaticCast(FindWindow(Choice_Viewer), wxChoice);

      if ( choice )
      {
         choice->SetSelection(curViewer);
      }
      else
      {
         FAIL_MSG( "where is out choice control?" );
      }
   }
   else
   {
      FAIL_MSG( "current viewer not in the list of all viewers?" );
   }
}

void wxFolderMsgWindow::UpdateOptions()
{
   if ( !m_winViewer )
   {
      // we don't show the viewer bar anyhow if we don't have the viewer
      return;
   }

   bool hasBar = m_winBar != NULL;
   bool toggleBar;
   if ( m_folderView->GetFolder() )
   {
      Profile_obj profile(m_folderView->GetFolderProfile());

      // folder options have changed, do we need to update?
      toggleBar = READ_CONFIG_BOOL(profile, MP_MSGVIEW_SHOWBAR) != hasBar;
   }
   else // no folder
   {
      // hide the viewer bar only if we show it
      toggleBar = hasBar;
   }

   if ( toggleBar )
   {
      if ( m_winBar )
         DeleteViewerBar();
      else
         CreateViewerBar();

      Resize();
   }

   // also update the selection in the choice control
   if ( m_winBar )
   {
      UpdateViewerBar();
   }
}

// ----------------------------------------------------------------------------
// wxFolderMsgWindow event handlers
// ----------------------------------------------------------------------------

void wxFolderMsgWindow::OnSize(wxSizeEvent& event)
{
   if ( m_winViewer )
   {
      Resize();
   }
}

void wxFolderMsgWindow::OnButton(wxCommandEvent& event)
{
   MDialog_Message
   (
      _("You can reenable this tool bar in the \"Message View\" page\n"
        "of the folders dialog later if you wish."),
      GetFrame(this),
      MDIALOG_MSGTITLE,
      GetPersMsgBoxName(M_MSGBOX_VIEWER_BAR_TIP)
   );

   // TODO: maybe ask the user if he wants to hide it just for now or disable
   //       it permanently? And/or also if he wants to do it just for this
   //       folder or for all of them?
   Profile_obj profile(m_folderView->GetFolderProfile());
   profile->writeEntry(MP_MSGVIEW_SHOWBAR, 0l);

   DeleteViewerBar();

   Resize();
}

void wxFolderMsgWindow::OnChoice(wxCommandEvent& event)
{
   int n = event.GetInt();

   CHECK_RET( n >= 0 && (size_t)n < m_namesViewers.GetCount(),
              "invalid viewer selected?" );

   Profile_obj profile(m_folderView->GetFolderProfile());

   profile->writeEntry(MP_MSGVIEW_VIEWER, m_namesViewers[n]);

   MEventManager::Send(new MEventOptionsChangeData
                           (
                            profile,
                            MEventOptionsChangeData::Ok
                           ));
}

// resize our own child to fill the entire available for it area
void wxFolderMsgWindow::Resize()
{
   wxSize size = GetClientSize();
   int y;
   if ( m_winBar )
   {
      y = m_winBar->GetSize().y;
      m_winBar->SetSize(0, 0, size.x, y);
   }
   else // no viewer bar
   {
      y = 0;
   }

   m_winViewer->SetSize(0, y, size.x, size.y - y);
}

// ----------------------------------------------------------------------------
// wxFolderMsgViewerEvtHandler
// ----------------------------------------------------------------------------

void wxFolderMsgViewerEvtHandler::OnChar(wxKeyEvent& event)
{
   // HandleCharEvent() will skip the event if it doesn't process it
   m_folderView->HandleCharEvent(event);
}

// ============================================================================
// wxFolderListCtrl implementation
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
   EVT_LIST_COL_RIGHT_CLICK(-1, wxFolderListCtrl::OnColumnRightClick)

   EVT_LIST_KEY_DOWN(-1, wxFolderListCtrl::OnListKeyDown)

   EVT_TIMER(-1, wxFolderListCtrl::OnPreviewTimer)

   EVT_IDLE(wxFolderListCtrl::OnIdle)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderListCtrl ctor/dtor
// ----------------------------------------------------------------------------

wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
                : m_timerPreview(this)
{
   m_headers = NULL;
   m_indexHI = (size_t)-1;
   m_hiCached = NULL;
   m_attr = NULL;

   m_PreviewOnSingleClick = false;
   m_PreviewDelay = 0;

   m_FolderView = fv;
   m_enableOnSelect = true;
   m_menu = NULL;
   m_menuFolders = NULL;

   m_colSort = WXFLC_NONE;
   // FIXME: replace by a more efficient call
   for (size_t i = 0; i < WXFLC_NUMENTRIES; ++i) {
      m_columns[i] = 0;
   }

   m_isInPopupMenu = false;

   // no items focused/previewed yet
   m_uidFocus =
   m_uidPreviewed = UID_ILLEGAL;

   m_itemFocus =
   m_itemPreviewed = -1;

   // no selection at all
   m_selIsUnique = -1;

   // do create the control
   Create(parent, M_WXID_FOLDERVIEW_LISTCTRL,
          wxDefaultPosition, parent->GetClientSize(),
          wxLC_REPORT | wxLC_VIRTUAL | wxNO_BORDER);

   // add the images to use for the columns
   wxBitmap bmpDown = sort_down_xpm,
            bmpUp = sort_up_xpm;
   wxImageList *imagelist = new wxImageList(bmpDown.GetWidth(),
                                            bmpDown.GetHeight());

   // this must be consistent with logic in UpdateSortIndicator(): first icon
   // is used for the reversed sort, second for the normal one
   imagelist->Add(bmpUp);
   imagelist->Add(bmpDown);
   AssignImageList(imagelist, wxIMAGE_LIST_SMALL);

   // create a drop target for dropping messages on us
   new MMessagesDropTarget(new FolderViewMessagesDropWhere(m_FolderView), this);
}

wxFolderListCtrl::~wxFolderListCtrl()
{
   SafeDecRef(m_headers);

   delete m_attr;

   delete m_menuFolders;
   delete m_menu;
}

void wxFolderListCtrl::OnFolderChange()
{
   // if the folder we show changes, don't keep the old headers
   if ( m_headers )
   {
      m_headers->DecRef();
      m_headers = NULL;

      InvalidateCache();
   }

   // wait until we get the headers
   Freeze();
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
   else // same columns in same order
   {
      // but their widths could have changed, so update them
      String widthsString = GetColWidths();
      if ( widthsString != m_widthsOld )
      {
         m_widthsOld = widthsString;

         wxArrayString widths = UnpackWidths(widthsString);
         size_t count = widths.GetCount();
         for ( size_t n = 0; n < count; n++ )
         {
            SetColumnWidth(n, atol(widths[n]));
         }
      }
   }

   UpdateSortIndicator();
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl event handlers
// ----------------------------------------------------------------------------

void wxFolderListCtrl::OnChar(wxKeyEvent& event)
{
   // don't process events until we're fully initialized
   if( !HasFolder() || !m_FolderView->m_MessagePreview )
   {
      // can't process any commands now
      event.Skip();
      return;
   }

   (void)m_FolderView->HandleCharEvent(event);
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

   if ( HasFolder() )
   {
      // start the drag and drop operation
      if ( event.Dragging() )
      {
         const UIdArray& selections = m_FolderView->GetSelections();
         if ( !selections.IsEmpty() &&
                  m_FolderView->m_msgCmdProc->
                     ProcessCommand(WXMENU_MSG_DRAG, selections) )
         {
            // skipping event.Skip() below
            return;
         }
      }
   }

   event.Skip();
}

void wxFolderListCtrl::OnRightClick(wxMouseEvent& event)
{
   if ( !HasFolder() )
      return;

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
         WXMENU_MSG_REPLY_ALL,
         WXMENU_MSG_REPLY_ALL_WITH_TEMPLATE,
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
         WXMENU_MSG_MARK_READ,
         WXMENU_MSG_MARK_UNREAD,
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

   // constructing the menu may take a long time
   wxBeginBusyCursor();
   m_menu->Insert(0, WXMENU_POPUP_FOLDER_MENU, _("&Quick move"),
                  m_menuFolders->GetMenu());
   wxEndBusyCursor();

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
   mApplication->UpdateAwayMode();

   if ( !HasFolder() )
      return;

   // there is exactly one item with the focus on it
   long focused = GetFocusedItem();

   // this would be a wxWin bug
   CHECK_RET( focused != -1, "activated unfocused item?" );

   // show the focused message
   UIdType uid = GetUIdFromIndex(focused);
   if ( m_PreviewOnSingleClick )
   {
      // view on double click then
      ShowMessageViewFrame(GetFrame(this), m_FolderView->GetFolder(), uid);
   }
   else // preview on double click
   {
      PreviewItem(focused, uid);
   }
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
   if ( m_enableOnSelect )
   {
      // update it as it is normally only updated in OnIdle() which wasn't
      // called yet
      m_itemFocus = GetFocusedItem();

      // interestingly enough, wxMSW returns -1 from GetFocusedItem() when the
      // item had been selected with Ctrl-click and not a simple click
      if ( m_itemFocus == -1 )
      {
         m_itemFocus = event.GetIndex();

         CHECK_RET( m_itemFocus != -1, "must have a valid item" );
      }

      // check if we already have this item - maybe we need to retrieve it
      // from server? this may happen if the header retrieval was cancelled
      UIdType uid = GetUIdFromIndex(m_itemFocus);

      if ( uid == UID_ILLEGAL )
      {
         MLocker lock(m_mutexHeaders);

         if ( !m_headers->ReallyGet(m_itemFocus) )
         {
            // we failed to get it, what can we do?
            return;
         }

         uid = GetUIdFromIndex(m_itemFocus);

         ASSERT_MSG( uid != UID_ILLEGAL, "invalid uid after ReallyGet()?" );
      }

      // we need to update m_selIsUnique right now because usually it's updated
      // lazily during the idle time but we need the real value of it in the
      // test below, otherwise the second item selected would be previewed as
      // well because at that moment m_selIsUnique would still be 1
      UpdateUniqueSelFlag();

      // preview the message when it is the first one we select or if there is
      // exactly one currently selected message (which will be deselected by
      // PreviewItem then); selecting subsequent messages just extends the
      // selection but doesn't show them
      if ( m_selIsUnique != 0 )
      {
         // if m_PreviewDelay != 0, use delayed preview, i.e. just launch the
         // timer now and only preview the messages when it expires: this
         // allows to quickly move through the message list without having to
         // wait for the message to be previewed but when you stop, the
         // message will be previewed automatically
         //
         // if m_PreviewDelay == 0, this is just the same as PreviewItem()
         PreviewItemDelayed(m_itemFocus, uid);
      }
      //else: don't react to selecting another message
   }
   //else: processing this notification is temporarily blocked
}

// called by RETURN press
void wxFolderListCtrl::OnActivated(wxListEvent& event)
{
   mApplication->UpdateAwayMode();

   long item = event.m_itemIndex;
   if ( IsPreviewed(item) )
   {
      // scroll down one line, go to the next unread if already at the end of
      // this one
      if ( !m_FolderView->m_MessagePreview->LineDown() )
      {
         MoveToNextUnread();
      }
   }
   else // do preview
   {
      UIdType uid = GetUIdFromIndex(item);

      PreviewItem(item, uid);
   }
}

void wxFolderListCtrl::OnColumnRightClick(wxListEvent& event)
{
   // get some non NULL profile
   Profile_obj profile(m_FolderView->GetFolderProfile());

   String name = profile->GetName();
   if ( !name.StartsWith(String(M_PROFILE_CONFIG_SECTION) + '/', &name) )
   {
      if ( name != M_PROFILE_CONFIG_SECTION )
      {
         FAIL_MSG( "unexpected profile - what folder does it correspond to?" );
      }
      else
      {
         // editing global settings
         name.clear();
      }
   }

   wxString menuTitle;
   if ( name.empty() )
   {
      menuTitle = _("Default");
   }
   else // we're changing options for some folder
   {
      menuTitle = name.AfterLast('/');
   }

   wxMenu menu(menuTitle);

   // add items to sort by this column in direct/reverse order if we have a
   // valid column and if we can sort by it
   wxFolderListColumn col = GetColumnByIndex(m_columns, event.GetColumn());
   if ( col != WXFLC_NONE && SortOrderFromCol(col) != MSO_NONE )
   {
      String colName = GetColumnName(col).Lower();

      menu.Append(WXMENU_FVIEW_SORT_BY_COL + col,
                  wxString::Format(_("Sort by %s"), colName.c_str()));
      menu.Append(WXMENU_FVIEW_SORT_BY_COL_REV + col,
                  wxString::Format(_("Reverse sort by %s"), colName.c_str()));
      menu.AppendSeparator();
   }
   //else: clicked outside any column or on a column we can't use for sorting

   // more sorting stuff
   menu.Append(WXMENU_FVIEW_RESET_SORT, _("&Don't sort at all"));
   menu.Append(WXMENU_FVIEW_CONFIG_SORT, _("Configure &sort order..."));

   // threading
   menu.AppendSeparator();
   menu.Append(WXMENU_FVIEW_TOGGLE_THREAD, _("&Thread messages"), "", TRUE);
   menu.Append(WXMENU_FVIEW_CONFIG_THREAD, _("&Configure threading..."));

   // add column-specific entries
   switch ( col )
   {
      case WXFLC_DATE:
         menu.AppendSeparator();
         menu.Append(WXMENU_FVIEW_CONFIG_DATEFMT, _("&Date format..."));
         break;

      case WXFLC_FROM:
         menu.AppendSeparator();
         menu.Append(WXMENU_FVIEW_FROM_NAMES_ONLY, _("&Show names only"), "", TRUE);

         if ( READ_CONFIG(profile, MP_FVIEW_NAMES_ONLY) )
         {
            menu.Check(WXMENU_FVIEW_FROM_NAMES_ONLY, TRUE);
         }
         break;

      case WXFLC_SIZE:
         menu.AppendSeparator();
         menu.Append(WXMENU_FVIEW_SIZE_AUTO, _("&Automatic units"), "", TRUE);
         menu.Append(WXMENU_FVIEW_SIZE_AUTOBYTES, _("Automatic b&yte units"), "", TRUE);
         menu.Append(WXMENU_FVIEW_SIZE_BYTES, _("Use &bytes"), "", TRUE);
         menu.Append(WXMENU_FVIEW_SIZE_KBYTES, _("Use &Kbytes"), "", TRUE);
         menu.Append(WXMENU_FVIEW_SIZE_MBYTES, _("Use &Mbytes"), "", TRUE);

         {
            // we rely on the fact the SIZE_XXX menu items are in the same
            // order as MessageSize_XXX constants
            long sizeFmt = READ_CONFIG(profile, MP_FVIEW_SIZE_FORMAT);

            sizeFmt += WXMENU_FVIEW_SIZE_AUTO;
            if ( sizeFmt > WXMENU_FVIEW_SIZE_MBYTES )
            {
               FAIL_MSG( "incorrect MP_FVIEW_SIZE_FORMAT value" );

               sizeFmt = WXMENU_FVIEW_SIZE_AUTO;
            }

            menu.Check(sizeFmt, TRUE);
         }
         break;

      default:
         FAIL_MSG( "forgot to update the switch after adding a new column?" );

      case WXFLC_STATUS:
      case WXFLC_SUBJECT:
         // nothing special to do
         break;
   }

   // set the initial state of the menu items

   if ( !READ_CONFIG(profile, MP_MSGS_SORTBY) )
   {
      // we're already unsorted, this command doesn't make sense
      menu.Enable(WXMENU_FVIEW_RESET_SORT, FALSE);
   }

   if ( READ_CONFIG(profile, MP_MSGS_USE_THREADING) )
   {
      menu.Check(WXMENU_FVIEW_TOGGLE_THREAD, TRUE);
   }

   // and show the menu
   PopupMenu(&menu, event.GetPoint());
}

void wxFolderListCtrl::OnColumnClick(wxListEvent& event)
{
   mApplication->UpdateAwayMode();

   // get the column which was clicked
   wxFolderListColumn col = GetColumnByIndex(m_columns, event.GetColumn());
   wxCHECK_RET( col != WXFLC_NONE, "should have a valid column" );

   MessageSortOrder orderCol = SortOrderFromCol(col);
   if ( orderCol == MSO_NONE )
   {
      // we can't sort by this column
      wxLogStatus(GetFrame(this),
                  _("Impossible to sort messages using %s column"),
                  GetColumnName(col).Lower().c_str());
      return;
   }

   // give an explanatory message as this stuff may be quite confusing
   MDialog_Message
   (
      _("Clicking on the column sorts messages in the folder by this\n"
        "column or, if they were already sorted by it, reverses the\n"
        "sort direction\n"
        "\n"
        "Please use \"Sort\" button in the \"Folder View\" page of the\n"
        "preferences dialog or right click a column to configure the\n"
        "sorting options in more details."),
      GetFrame(this),
      MDIALOG_MSGTITLE,
      GetPersMsgBoxName(M_MSGBOX_EXPLAIN_COLUMN_CLICK)
   );

   // we're going to change the sort order either for this profile in the
   // advanced mode but the global sort order in the novice mode: otherwise,
   // it would be surprizing for the novice user as he'd see one thing in the
   // options dialog and another thing on the screen (the sorting page is not
   // shown in the folder dialog in this mode)
   Profile *profile;
   bool usingGlobalProfile;
   if ( READ_APPCONFIG(MP_USERLEVEL) == (long)M_USERLEVEL_NOVICE )
   {
      profile = mApplication->GetProfile();
      profile->IncRef();

      usingGlobalProfile = true;
   }
   else
   {
      profile = m_FolderView->GetFolderProfile();

      usingGlobalProfile = false;
   }

   // sort by this column: if we already do this, then toggle the sort
   // direction
   long sortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   if ( usingGlobalProfile )
   {
      Profile *profileFolder = m_FolderView->GetProfile();
      if ( profileFolder &&
            READ_CONFIG(profileFolder, MP_MSGS_SORTBY) != sortOrder )
      {
         // as we're going to change the global one but this profiles settings
         // will override it!
         wxLogDebug("Changing the sort order won't take effect.");
      }
   }

   // There are 2 possibilities when the column Y is clicked and we currently
   // use the column X for sorting: either we resort using just Y or we sort
   // by Y first and then by X. The latter is called "complex sort" here and
   // is disabled for now because it is much less efficient than simple sort
   // because it can be reversed instantaneously (reverse of "Y, then X" must
   // be computed OTOH)
#ifdef USE_COMPLEX_SORT
   wxArrayInt sortOrders = SplitSortOrder(sortOrder);

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

   sortOrder = BuildSortOrder(sortOrders);
#else // !USE_COMPLEX_SORT
   MessageSortOrder orderCur = GetSortCritDirect(sortOrder);
   if ( orderCur == orderCol )
   {
      // reverse the criterium specified by orderCol
      //
      // NB: MSO_XXX_REV == MSO_XXX + 1
      sortOrder = IsSortCritReversed(sortOrder) ? orderCol : orderCol + 1;
   }
   else // sort by another column
   {
      sortOrder = orderCol;
   }
#endif // USE_COMPLEX_SORT/!USE_COMPLEX_SORT

   SetSortOrder(profile, sortOrder, col, GetSortCrit(sortOrder) != orderCol);

   profile->DecRef();
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
      case WXK_SPACE:
         // update the unique selection flag as we can lose it now (the keys
         // above can change the focus/selection)
         UpdateUniqueSelFlag();
         break;
   }

   event.Skip();
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl preview helpers
// ----------------------------------------------------------------------------

void wxFolderListCtrl::PreviewItem(long idx, UIdType uid)
{
   // remember this item as being previewed
   if ( SetPreviewMsg(idx, uid) )
   {
      // and actually show it in the folder view if it's different from the
      // old one
      m_FolderView->PreviewMessage(uid);
   }
}

void wxFolderListCtrl::OnPreviewTimer(wxTimerEvent& event)
{
   // preview timer expirer, do we still have the message we wanted to preview
   // selected?
   if ( GetUniqueSelection() == m_itemDelayed )
   {
      // do we still have this message? it might have been deleted
      if ( GetUIdFromIndex(m_itemDelayed) == m_uidDelayed )
         PreviewItem(m_itemDelayed, m_uidDelayed);
   }

   m_itemDelayed = -1;
   m_uidDelayed = UID_ILLEGAL;
}

void wxFolderListCtrl::PreviewItemDelayed(long idx, UIdType uid)
{
   // first of all, do we need to delay item previewing at all?
   if ( m_PreviewDelay )
   {
      // remember the item we wanted to preview
      m_itemDelayed = idx;
      m_uidDelayed = uid;

      // start (or restart) the timer
      m_timerPreview.Start(m_PreviewDelay, TRUE /* one shot */);
   }
   else // no, preview the item immediately
   {
      PreviewItem(idx, uid);
   }
}

bool wxFolderListCtrl::SetPreviewMsg(long idx, UIdType uid)
{
   if ( uid == m_uidPreviewed )
      return false;

   CHECK( idx != -1 && uid != UID_ILLEGAL, false,
          "invalid params in SetPreviewMsg" );

   // note that the item which has just been selected for being previewed must
   // be focused too
   m_itemFocus =
   m_itemPreviewed = idx;

   m_uidFocus =
   m_uidPreviewed = uid;

   wxLogTrace(M_TRACE_FV_SELECTION,
              "SetPreviewMsg(): index = %ld, UID = %08x",
              idx, uid);

   // as folder view calls us itself, no need to notify it
   wxFolderListCtrlBlockOnSelect noselect(this);

   // select the item being previewed
   if ( !GoToItem(m_itemPreviewed) )
   {
      // select unconditionally
      Select(m_itemPreviewed, true);
   }

   return true;
}

void wxFolderListCtrl::InvalidatePreview()
{
   m_itemPreviewed = -1;
   m_uidPreviewed = UID_ILLEGAL;

   wxLogTrace(M_TRACE_FV_SELECTION, "Invalidated preview");
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl header info
// ----------------------------------------------------------------------------

long wxFolderListCtrl::GetPosFromUID(UIdType uid)
{
   MLocker lockHeaders(m_mutexHeaders);

   size_t idx = m_headers->GetIdxFromUId(uid);

   return idx == INDEX_ILLEGAL ? -1 : (long)m_headers->GetPosFromIdx(idx);
}

void wxFolderListCtrl::UpdateListing(HeaderInfoList *headers)
{
   if ( headers == m_headers )
   {
      CHECK_RET( headers, "wxFolderListCtrl::UpdateListing: NULL listing" );

      // we already have it
      headers->DecRef();

      // update the number of items the listctrl thinks to have
      UpdateItemCount();

      // keep the same item selected if possible: use its UID as its index
      // might have changed if the sort order in the folder changed
      long posFocus = -1; // the new position of the focused item
      if ( m_uidFocus != UID_ILLEGAL )
      {
         posFocus = GetPosFromUID(m_uidFocus);
         if ( posFocus != -1 )
         {
            Focus(posFocus);
         }
      }
      // this does seem to happen when opening the folder for the first time
      // under wxGTK - it seems to be harmless but I need to look deeper into
      // this (FIXME)
#if 0
      else
      {
         ASSERT_MSG( m_itemFocus == -1, "why no focused UID?" );
      }
#endif // 0

      // TODO: should keep the currently selected items selected - but for
      //       this we need to store UIDs for all of them which we don't do
      //       currently :-(

      {
         wxFolderListCtrlBlockOnSelect dontHandleOnSelect(this);

         long numMessages = GetItemCount();
         for ( long n = 0; n < numMessages; n++ )
         {
            Select(n, false);
         }
      }

      // so for now just make sure that the unique selected message, if any, is
      // updated correctly
      if ( m_selIsUnique == 1 && m_uidPreviewed != UID_ILLEGAL )
      {
         // a common optimization for the case when the same message is both
         // focused and previewed
         long posPreview;
         if ( m_uidPreviewed == m_uidFocus && posFocus != -1 )
         {
            posPreview = posFocus;
         }
         else // need to find it out
         {
            posPreview = GetPosFromUID(m_uidPreviewed);
         }

         if ( posPreview != -1 )
         {
            Select(posPreview, true);
         }
      }
   }
   else // new listing
   {
      SetListing(headers);

      m_FolderView->SelectInitialMessage();

      // we can redraw now
      Thaw();
   }
}

void wxFolderListCtrl::InvalidateCache()
{
   m_indexHI = (size_t)-1;

   m_hiCached = NULL;

   m_headersToGet.Empty();
}

void wxFolderListCtrl::SetListing(HeaderInfoList *listing)
{
   CHECK_RET( listing, "NULL listing in wxFolderListCtrl" );

   m_cacheLastMod = listing->GetLastMod();

   if ( m_headers )
   {
      m_headers->DecRef();

      InvalidateCache();
   }

   m_headers = listing;

   // we'll crash if we use it after the listing changed!
   //
   // NB: do it before calling UpdateItemCount() as under GTK it redraws
   //     the window immediately, hence calling back our OnGetItemText() which
   //     sets m_hiCached
   ASSERT_MSG( !m_hiCached, "should be reset" );

   UpdateItemCount();
}

HeaderInfo *wxFolderListCtrl::GetHeaderInfo(size_t index) const
{
   CHECK( m_headers, NULL, "no listing hence no header info" );

   wxFolderListCtrl *self = wxConstCast(this, wxFolderListCtrl);

   if ( m_headers->HasChanged(m_cacheLastMod) )
   {
      self->m_cacheLastMod = m_headers->GetLastMod();

      self->InvalidateCache();
   }

   if ( m_indexHI != index )
   {
      if ( !m_headers->IsInCache(index) )
      {
         // we will retrieve it later as it may take a long time to do it now
         // and we shouldn't block inside OnGetItemXXX() functions which are,
         // themselves, called from the list control OnPaint()
         if ( m_headersToGet.Index(index) == wxNOT_FOUND )
         {
            self->m_headersToGet.Add(index);
         }

         return NULL;
      }

      // if the header is already cached, check if it's not the focused one
      if ( (long)index == m_itemFocus )
      {
         // it is, update m_uidFocus here (for non cached indices it would be
         // updated in OnIdle() as we can't do it before retrieving the
         // header)
         if ( m_uidFocus == UID_ILLEGAL )
         {
            self->m_uidFocus = GetUIdFromIndex(index);

            wxLogTrace(M_TRACE_FV_SELECTION,
                       "Updated focused UID, now %08x (index = %ld)",
                       m_uidFocus, index);
         }
      }

      self->m_hiCached = m_headers->GetItem(index);
      self->m_indexHI = index;
   }

   return m_hiCached;
}

void wxFolderListCtrl::OnIdle(wxIdleEvent& event)
{
   event.Skip();

   // there are many various reasons which can prevent us from being able to
   // call c-client (safely) from here
   if ( mApplication->AllowBgProcessing() )
   {
      if ( !m_headersToGet.IsEmpty() && !m_mutexHeaders.IsLocked() )
      {
         CHECK_RET( m_headers, "can't get headers without listing" );

         int posMin = INT_MAX,
             posMax = 0;

         Sequence seq;

         size_t count = m_headersToGet.GetCount();
         for ( size_t n = 0; n < count; n++ )
         {
            int pos = m_headersToGet[n];
            if ( pos < posMin )
               posMin = pos;

            if ( pos > posMax )
               posMax = pos;

            seq.Add(pos);
         }

         m_headersToGet.Empty();

         {
            MLocker lockHeaders(m_mutexHeaders);

            m_headers->CachePositions(seq);
         }

         // we may now (quickly!) update m_uidFocus, see comment in UpdateFocus()
         // to understand why we do it here
         if ( m_uidFocus == UID_ILLEGAL && m_itemFocus != -1 )
         {
            if ( m_headers->IsInCache(m_itemFocus) )
            {
               m_uidFocus = GetUIdFromIndex(m_itemFocus);

               wxLogTrace(M_TRACE_FV_SELECTION,
                          "Updated focused UID from OnIdle(), now %08x (index = %ld)",
                          m_uidFocus, m_itemFocus);

               m_FolderView->OnFocusChange(m_itemFocus, m_uidFocus);
            }
            //else: hmm, can it happen at all?
         }

         // now the header info should be in cache, so GetHeaderInfo() will
         // return it
         RefreshItems(posMin, posMax);
      }
   }

   // check if we [still] have exactly one selection
   UpdateUniqueSelFlag();

   // check if the focus changed
   UpdateFocus();
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl column width stuff
// ----------------------------------------------------------------------------

void wxFolderListCtrl::SaveColWidths()
{
   // save the widths if they changed
   String str = GetWidths();
   if ( str != FOLDER_LISTCTRL_WIDTHS_D && str != GetColWidths() )
   {
      // the widths did change, so save them for this folder
      Profile *profile = m_FolderView->GetProfile();
      if ( profile )
         profile->writeEntry(FOLDER_LISTCTRL_WIDTHS, str);

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
   String widthsString;

   Profile *profile = m_FolderView->GetProfile();
   if ( profile )
   {
      widthsString = READ_CONFIG_PRIVATE(profile, FOLDER_LISTCTRL_WIDTHS);
   }

   if ( widthsString.empty() || widthsString == FOLDER_LISTCTRL_WIDTHS_D )
   {
      // try the global setting
      widthsString =
         mApplication->GetProfile()->readEntry(USER_COLUMNS_WIDTHS,
                                               FOLDER_LISTCTRL_WIDTHS_D);
   }

   return widthsString;
}

String wxFolderListCtrl::GetWidths() const
{
   String str;

   int count = GetColumnCount();
   for ( int col = 0; col < count; col++ )
   {
      if ( !str.empty() )
         str << COLUMNS_WIDTHS_SEP;

      str << GetColumnWidth(col);
   }

   return str;
}

void wxFolderListCtrl::CreateColumns()
{
   // delete existing columns
   ClearAll();

   // read the widths array
   m_widthsOld = GetColWidths();

   // complete the array if necessary - this can happen if the previous folder
   // had less columns than this one
   wxArrayString widths = UnpackWidths(m_widthsOld);
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
      wxFolderListColumn col = GetColumnByIndex(m_columns, n);
      if ( col == WXFLC_NONE )
         break;

      long width;
      if ( !widths[n].ToLong(&width) )
         width = -1;

      // we have to specify an image initially - otherwise setting it later for
      // the column will have no effect under MSW (bug in the native control)
      wxListItem item;
      item.m_mask = wxLIST_MASK_TEXT |
                    wxLIST_MASK_IMAGE |
                    wxLIST_MASK_FORMAT |
                    wxLIST_MASK_WIDTH;
      item.m_format = wxLIST_FORMAT_LEFT;
      item.m_width = width;
      item.m_text = GetColumnName(col);
      item.m_image = -1;
      InsertColumn(n, item);
   }
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl sorting
// ----------------------------------------------------------------------------

void wxFolderListCtrl::SetSortOrder(Profile *profile,
                                    long sortOrder,
                                    wxFolderListColumn col,
                                    bool reverse)
{
   wxLogStatus(GetFrame(this), _("Now sorting by %s%s"),
               GetColumnName(col).Lower().c_str(),
               reverse ? _(" (reverse)") : "");

   profile->writeEntry(MP_MSGS_SORTBY, sortOrder);

   UpdateSortIndicator();

   MEventManager::Send(new MEventOptionsChangeData
                           (
                            profile,
                            MEventOptionsChangeData::Ok
                           ));
}

void wxFolderListCtrl::UpdateSortIndicator()
{
   // which column do we use now?
   Profile_obj profile(m_FolderView->GetFolderProfile());

   long sortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   MessageSortOrder orderPrimary = GetSortCritDirect(sortOrder);

   wxFolderListColumn colSort = ColFromSortOrder(orderPrimary);

   // if it's not the same as the old one, clear the indicator in the prev
   // column
   if ( colSort != m_colSort )
   {
      if ( m_colSort != WXFLC_NONE )
      {
         int colIdx = m_columns[m_colSort];
         if ( colIdx != -1 )
            ClearColumnImage(colIdx);
      }

      m_colSort = colSort;
   }

   // sorting by something? if yes, show it
   if ( m_colSort != WXFLC_NONE )
   {
      int colIdx = m_columns[m_colSort];
      if ( colIdx != -1 )
      {
         SetColumnImage(colIdx, IsSortCritReversed(sortOrder));
      }
   }
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl focus/selection management
// ----------------------------------------------------------------------------

void wxFolderListCtrl::UpdateFocus()
{
   long itemFocus = GetFocusedItem();

   if ( itemFocus == m_itemFocus )
      return;

   m_itemFocus = itemFocus;

   // if there is no focus, we can notify the folder view immediately as it
   // doesn't cost much - but if there is, it will be done later, when
   // m_uidFocus will have been set
   if ( m_itemFocus == -1 )
   {
      m_FolderView->OnFocusChange(-1, UID_ILLEGAL);
   }
   else
   {
      if ( m_headers->IsInCache(m_itemFocus) )
      {
         m_uidFocus = GetUIdFromIndex(m_itemFocus);
      }
      else
      {
         // we can't call GetUIdFromIndex() from here if the item is not
         // cached as we don't want to block now - we'll set it later when we
         // retrieve the focused item anyhow
         m_uidFocus = UID_ILLEGAL;
      }

      wxLogTrace(M_TRACE_FV_SELECTION,
                 "UpdateFocus(): index = %ld, UID = %08x",
                 itemFocus, m_uidFocus);
   }
}

void wxFolderListCtrl::UpdateUniqueSelFlag()
{
   long item = GetFirstSelected();
   if ( item != -1 )
   {
      if ( GetNextSelected(item) != -1 )
      {
         // >= 2 items selected
         m_selIsUnique = 0;
      }
      else
      {
         // exactly 1 item selected
         m_selIsUnique = 1;
      }
   }
   else
   {
      // no items selected
      m_selIsUnique = -1;
   }
}

long wxFolderListCtrl::GetUniqueSelection() const
{
   long item = GetFirstSelected();
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

   long item = GetFirstSelected();
   while ( item != -1 )
   {
      uids.Add(GetUIdFromIndex(item));

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
   return GetFirstSelected() != -1 || GetFocusedItem() != -1;
}

UIdType
wxFolderListCtrl::GetFocusedUId(long *idx) const
{
   long item = GetFocusedItem();
   if ( idx )
      *idx = item;

   return item == -1 ? UID_ILLEGAL : GetUIdFromIndex(item);
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl items callbacks
// ----------------------------------------------------------------------------

wxString wxFolderListCtrl::OnGetItemText(long item, long column) const
{
   wxString text;

   // this does happen because the number of messages in m_headers is
   // decremented first and our OnFolderExpungeEvent() is called much later
   // (during idle time), so we have no choice but to ignore the requests for
   // non existing items
   if ( (size_t)item >= GetHeadersCount() )
   {
      return text;
   }

   HeaderInfo *hi = GetHeaderInfo((size_t)item);
   if ( !hi )
   {
      // will get it later
      return _("...");
   }

   wxFolderListColumn field = GetColumnByIndex(m_columns, column);

   // deal with invalid headers first - these headers were not retrieved from
   // server because the user aborted the operation
   if ( !hi->IsValid() )
   {
      switch ( field )
      {
         case WXFLC_DATE:
         case WXFLC_SIZE:
         case WXFLC_STATUS:
            text = "???";
            break;

         case WXFLC_FROM:
         case WXFLC_SUBJECT:
            // "???" here would look too weird, just leave them empty
            break;

         default:
            wxFAIL_MSG( "unknown column" );
      }

      return text;
   }

   switch ( field )
   {
      case WXFLC_STATUS:
         text = MailFolder::ConvertMessageStatusToString(hi->GetStatus());
         break;

      case WXFLC_FROM:
         {
            // optionally replace the "From" with "To: someone" for messages sent
            // by the user himself
            HeaderInfo::HeaderKind hdrKind = HeaderInfo::GetFromOrTo
                                             (
                                                hi,
                                                GetSettings().replaceFromWithTo,
                                                GetSettings().returnAddresses,
                                                &text
                                             );

            // optionally leave only the name part of the address (do it before
            // mangling it below)
            if ( GetSettings().senderOnlyNames )
            {
               String name = Message::GetNameFromAddress(text);
               if ( !name.empty() )
               {
                  text = name;
               }
               //else: leave address without names as is
            }

            switch ( hdrKind )
            {
               default:
               case HeaderInfo::Invalid:
                  FAIL_MSG( "unexpected GetFromOrTo() return value" );
                  // fall through

               case HeaderInfo::From:
                  // nothing special to do
                  break;

               case HeaderInfo::To:
                  text.Prepend(_("To: "));
                  break;

               case HeaderInfo::Newsgroup:
                  text.Prepend(_("Posted to: "));
                  break;
            }
         }
         break;

      case WXFLC_DATE:
         text = strutil_ftime(hi->GetDate(),
                              GetSettings().dateFormat,
                              GetSettings().dateGMT);

         break;

      case WXFLC_SIZE:
         text = MailFolder::SizeToString(hi->GetSize(),
                                         hi->GetLines(),
                                         GetSettings().showSize);
         break;

      case WXFLC_SUBJECT:
         // FIXME: hard coded 3 spaces
         text = wxString(' ', 3*m_headers->GetIndentation((size_t)item))
                  + hi->GetSubject();
         break;

      default:
         wxFAIL_MSG( "unknown column" );
   }

   if ( field == WXFLC_FROM || field == WXFLC_SUBJECT )
   {
      wxFontEncoding encoding = hi->GetEncoding();

      // optionally convert from UTF-8 to environment's default encoding
      // FIXME it won't be needed when full Unicode support is available
      if ( encoding == wxFONTENCODING_UTF8 )
      {
         text = wxString(text.wc_str(wxConvUTF8), wxConvLocal);
         encoding = wxLocale::GetSystemEncoding();
      }

      // we might do conversion if we can't show this encoding directly
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

                  text = conv.Convert(text);
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

   }

   return text;
}

int wxFolderListCtrl::OnGetItemImage(long item) const
{
   return -1;
}

wxColour
wxFolderListCtrl::GetEntryColour(const HeaderInfo *hi) const
{
   wxColour col;

#ifdef USE_HEADER_COLOUR
   if ( !hi->GetColour().empty() ) // entry has its own colour setting
   {
      GetColourByName(&col, hi->GetColour(), GetStringDefault(MP_FVIEW_FGCOLOUR));
   }
   else // set the colour depending on the status of the message
#endif // USE_HEADER_COLOUR
   {
      int status = hi->GetStatus();

      const wxFolderView::AllProfileSettings& settings = GetSettings();

      if ( status & MailFolder::MSG_STAT_FLAGGED )
         col = settings.FlaggedCol;
      else if ( status & MailFolder::MSG_STAT_DELETED )
         col = settings.DeletedCol;
      else if ( status & MailFolder::MSG_STAT_RECENT )
         col = status & MailFolder::MSG_STAT_SEEN ? settings.RecentCol
                                                  : settings.NewCol;
      else if ( !(status & MailFolder::MSG_STAT_SEEN) )
         col = settings.UnreadCol;
      //else: normal message, show in default colour
   }

   return col;
}

wxListItemAttr *wxFolderListCtrl::OnGetItemAttr(long item) const
{
   // see comment in the beginning of OnGetItemText()
   if ( (size_t)item >= GetHeadersCount() )
   {
      return NULL;
   }

   HeaderInfo *hi = GetHeaderInfo((size_t)item);
   if ( !hi )
   {
      // will get it later
      return NULL;
   }

   if ( !hi->IsValid() )
   {
      // no attributes for the headers we didn't retrieve
      return NULL;
   }

   if ( !m_attr )
   {
      // do it only once, then always reuse the same
      wxConstCast(this, wxFolderListCtrl)->m_attr = new wxListItemAttr;
   }

   // GetEntryColour() may return invalid colour, but that's ok, it will just
   // reset the colour to default
   m_attr->SetTextColour(GetEntryColour(hi));

   // cache the last used encoding as creating new font is an expensive
   // operation
   wxFontEncoding enc = hi->GetEncoding();

   if ( enc == wxFONTENCODING_UTF8 )
   {
      // As we converted text to environment's default encoding above, encoding
      // is no longer wxFONTENCODING_UTF8, but wxLocale::GetSystemEncoding().
      enc = wxLocale::GetSystemEncoding();
   }

   if ( enc != wxFONTENCODING_SYSTEM )
   {
      if ( !m_attr->HasFont() || m_attr->GetFont().GetEncoding() != enc )
      {
         wxFont font = GetFont();
         font.SetEncoding(enc);
         m_attr->SetFont(font);
      }
   }
   else // default encoding now
   {
      if ( m_attr->HasFont() && m_attr->GetFont().GetEncoding() != enc )
      {
         m_attr->SetFont(wxNullFont);
      }
   }

   return m_attr;
}

// ----------------------------------------------------------------------------
// wxFolderListCtrl selection
// ----------------------------------------------------------------------------

bool wxFolderListCtrl::GoToItem(long item)
{
   Focus(item);

   long itemOldSel = GetUniqueSelection();
   if ( itemOldSel != -1 )
   {
      Select(itemOldSel, false);
      Select(item, true);

      // we selected the new item, return true
      return true;
   }

   // this means we didn't select the specified item
   return false;
}

bool
wxFolderListCtrl::SelectNextByStatus(MailFolder::MessageStatus status,
                                     bool isSet)
{
   if( !m_headers || m_headers->Count() == 0 )
   {
      // cannot do anything without listing
      return false;
   }

   long idxFocused = GetFocusedItem();
   if ( SelectNextUnreadAfter(idxFocused, status, isSet) )
   {
      // always preview the selected message, if we did "Show next unread"
      // we really want to see it regardless of m_PreviewOnSingleClick
      // setting
      //
      // NB: SelectNextUnreadAfter() has already changed the focus
      PreviewItem(GetFocusedItem(), GetFocusedUId());

      return true;
   }
   //else: no next unread msg found

   String msg;
   if ( (idxFocused != -1) &&
            CheckStatusBit(m_headers->GetItem(idxFocused)->GetStatus(),
                           status, isSet) )
   {
      // "more" because the one selected previously was unread
      msg = _("No more unread or flagged messages in this folder.");
   }
   else
   {
      // no unread messages at all
      msg = _("No unread or flagged messages in this folder.");
   }

   wxLogStatus(GetFrame(this), msg);

   return false;
}


bool
wxFolderListCtrl::SelectNextUnreadAfter(long idxFocused,
                                        MailFolder::MessageStatus status,
                                        bool isSet)
{
   CHECK( m_headers, false, "no headers in SelectNextUnreadAfter" );

   MLocker lockHeaders(m_mutexHeaders);

   size_t idx = m_headers->FindHeaderByFlagWrap(status, isSet, idxFocused);

   if ( idx == INDEX_ILLEGAL )
   {
      // no such messages
      return false;
   }

   Focus(idx);

   return true;
}

// ============================================================================
// wxFolderView implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFolderView ctor and dtor
// ----------------------------------------------------------------------------

wxFolderView *
wxFolderView::Create(wxWindow *parent)
{
   wxCHECK_MSG(parent, NULL, "NULL parent frame in wxFolderView ctor");
   wxFolderView *fv = new wxFolderView(parent);
   return fv;
}

wxFolderView::wxFolderView(wxWindow *parent)
{
   m_FocusFollowMode = false;
   m_Profile = NULL;
   m_Parent = parent;

   m_Frame = GetFrame(m_Parent);

   m_ASMailFolder = NULL;
   m_regOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);

   m_TicketList = ASTicketList::Create();

   m_nDeleted = UID_ILLEGAL;

   m_SplitterWindow = new wxFolderSplitterWindow(m_Parent);
   m_FolderCtrl = new wxFolderListCtrl(m_SplitterWindow, this);
   m_MessageWindow = new wxFolderMsgWindow(m_SplitterWindow, this);

   m_MessagePreview = MessageView::Create(m_MessageWindow, this);

   m_MessageWindow->SetViewerWindow(m_MessagePreview->GetWindow());

   m_msgCmdProc = MsgCmdProc::Create(m_MessagePreview, m_FolderCtrl);
   m_msgCmdProc->SetFrame(GetFrame(m_FolderCtrl));

   ReadProfileSettings(&m_settings);
   ApplyOptions();
   m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);
   m_FolderCtrl->SetPreviewDelay(m_settings.previewDelay);

   m_SplitterWindow->SplitHorizontally(m_FolderCtrl,
                                       m_MessageWindow,
                                       m_Parent->GetClientSize().y/3);
   m_SplitterWindow->SetMinimumPaneSize(10);
   m_SplitterWindow->SetFocus();
}

wxFolderView::~wxFolderView()
{
   MEventManager::Deregister(m_regOptionsChange);
   DeregisterEvents();

   if ( m_TicketList )
      m_TicketList->DecRef();

   delete m_MessagePreview;

   // set it to NULL so that Clear() knows we are being destroyed - see the
   // code there
   m_MessagePreview = NULL;

   Clear();

   // must have been deleted in Clear()
   ASSERT_MSG( !m_Profile, "profile leak in wxFolderView" );

   delete m_msgCmdProc;
}

// ----------------------------------------------------------------------------
// wxFolderView navigation in headers
// ----------------------------------------------------------------------------

inline size_t wxFolderView::GetHeadersCount() const
{
   return m_FolderCtrl->GetHeadersCount();
}

unsigned long wxFolderView::GetDeletedCount() const
{
   if ( m_nDeleted == UID_ILLEGAL )
   {
      MailFolder_obj mf = GetMailFolder();

      // const_cast
      ((wxFolderView *)this)->m_nDeleted = mf->CountDeletedMessages();
   }

   return m_nDeleted;
}

bool wxFolderView::MoveToNextUnread(bool takeNextIfNoUnread)
{
   if ( !READ_CONFIG(m_Profile, MP_FVIEW_AUTONEXT_UNREAD_MSG) )
   {
      // this feature is disabled
      return false;
   }

   return SelectNextUnread(takeNextIfNoUnread);
}

bool wxFolderView::SelectNextUnread(bool takeNextIfNoUnread)
{
   HeaderInfoList_obj hil = m_ASMailFolder->GetHeaders();

   int idxFocused = m_FolderCtrl->GetFocusedItem();
   if ( idxFocused == -1 )
      return false;

   size_t idx = hil->FindHeaderByFlag(MailFolder::MSG_STAT_SEEN, false,
                                      idxFocused);

   // no unread?
   if ( idx == UID_ILLEGAL )
   {
      // don't even try going to the next one
      if ( !takeNextIfNoUnread )
      {
         // nothing to do
         return false;
      }

      // is there another message in the folder?
      if ( idxFocused == m_FolderCtrl->GetItemCount() - 1 )
      {
         // no, don't do anything
         return false;
      }

      // take just the next one
      idx = idxFocused + 1;
   }

   m_FolderCtrl->Focus(idx);

   UIdType uid = m_FolderCtrl->GetUIdFromIndex(idx);
   m_FolderCtrl->SetPreviewMsg(idx, uid);
   PreviewMessage(uid);

   return true;
}

void
wxFolderView::SelectInitialMessage()
{
   // select some "interesting" message initially: the logic here is a bit
   // hairy, but, hopefully, this does what expected.
   //
   // explanations: if MP_AUTOSHOW_FIRSTUNREADMESSAGE is off, then we
   // just select either the first message (if MP_AUTOSHOW_FIRSTMESSAGE) or
   // the last one (otherwise). If it is on and we have an unread message,
   // we always select first unread message, but if there are no unread
   // messages, we revert to the previous behaviour, i.e. select the first
   // or the last one
   size_t numMessages = GetHeadersCount();
   if ( !numMessages )
   {
      // nothing to select anyhow
      return;
   }

   MsgnoType idx = INDEX_ILLEGAL;
   if ( READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTUNREADMESSAGE) )
   {
      // don't waste time looking for unread messages if we already know that
      // there are none (as we often do)
      MailFolder_obj mf = GetMailFolder();

      if ( mf )
      {
         MailFolderStatus status;
         if ( MfStatusCache::Get()->GetStatus(mf->GetName(), &status) &&
               status.unread &&
                 m_FolderCtrl->SelectNextUnreadAfter() )
         {
            idx = m_FolderCtrl->GetFocusedItem();
         }
      }
      else
      {
         FAIL_MSG( "no mail folder?" );
      }
   }

   if ( idx == INDEX_ILLEGAL )
   {
      // select first unread is off or no unread message, so select the first
      // or the last one depending on the options
      //
      // NB: idx is always a valid index because numMessages >= 1
      idx = READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTMESSAGE) ? 0
                                                             : numMessages - 1;

      m_FolderCtrl->Focus(idx);
   }

   // the item is already focused, now preview it automatically too if we're
   // configured to do this
   if ( m_settings.previewOnSingleClick )
   {
      HeaderInfoList_obj hil = GetFolder()->GetHeaders();

      const HeaderInfo *hi = hil[idx];
      CHECK_RET( hi, "Failed to get the uid of preselected message" );

      UIdType uid = hi->GetUId();
      if ( uid != UID_ILLEGAL )
      {
         m_FolderCtrl->SetPreviewMsg(idx, uid);
         PreviewMessage(uid);
      }
   }
}

// ----------------------------------------------------------------------------
// wxFolderView profile stuff (options support, ...)
// ----------------------------------------------------------------------------

String wxFolderView::GetFullPersistentKey(MPersMsgBox key)
{
   String s;
   s << Profile::FilterProfileName(m_fullname)
     << '/'
     << GetPersMsgBoxName(key);
   return s;
}

void
wxFolderView::ReadProfileSettings(AllProfileSettings *settings)
{
   Profile *profile = m_Profile ? m_Profile : mApplication->GetProfile();

#ifdef OS_WIN
   // MP_DATE_FMT contains '%' which are being (mis)interpreted as env var
   // expansion characters under Windows - prevent this from happening
   ProfileEnvVarSave noEnvVars(profile);
#endif // OS_WIN

   settings->dateFormat = READ_CONFIG_TEXT(profile, MP_DATE_FMT);
   settings->dateGMT = READ_CONFIG_BOOL(profile, MP_DATE_GMT);

   GetColourByName(&settings->FgCol,
                   READ_CONFIG(profile, MP_FVIEW_FGCOLOUR),
                   GetStringDefault(MP_FVIEW_FGCOLOUR));
   GetColourByName(&settings->BgCol,
                   READ_CONFIG(profile, MP_FVIEW_BGCOLOUR),
                   GetStringDefault(MP_FVIEW_BGCOLOUR));
   GetColourByName(&settings->FlaggedCol,
                   READ_CONFIG(profile, MP_FVIEW_FLAGGEDCOLOUR),
                   GetStringDefault(MP_FVIEW_FLAGGEDCOLOUR));
   GetColourByName(&settings->NewCol,
                   READ_CONFIG(profile, MP_FVIEW_NEWCOLOUR),
                   GetStringDefault(MP_FVIEW_NEWCOLOUR));
   GetColourByName(&settings->RecentCol,
                   READ_CONFIG(profile, MP_FVIEW_RECENTCOLOUR),
                   GetStringDefault(MP_FVIEW_RECENTCOLOUR));
   GetColourByName(&settings->DeletedCol,
                   READ_CONFIG(profile, MP_FVIEW_DELETEDCOLOUR),
                   GetStringDefault(MP_FVIEW_DELETEDCOLOUR));
   GetColourByName(&settings->UnreadCol,
                   READ_CONFIG(profile, MP_FVIEW_UNREADCOLOUR),
                   GetStringDefault(MP_FVIEW_UNREADCOLOUR));

   static const int fontFamilies[] =
   {
      wxDEFAULT,
      wxDECORATIVE,
      wxROMAN,
      wxSCRIPT,
      wxSWISS,
      wxMODERN,
      wxTELETYPE
   };

   settings->font = READ_CONFIG(profile,MP_FVIEW_FONT);
   if ( settings->font < 0 || (size_t)settings->font > WXSIZEOF(fontFamilies) )
   {
      wxFAIL_MSG( "bad font setting in config" );

      settings->font = 0;
   }

   settings->font = fontFamilies[settings->font];
   settings->size = READ_CONFIG(profile, MP_FVIEW_FONT_SIZE);
   settings->senderOnlyNames =
       READ_CONFIG_BOOL(profile, MP_FVIEW_NAMES_ONLY);

   settings->showSize =
      (MessageSizeShow)(long)READ_CONFIG(profile, MP_FVIEW_SIZE_FORMAT);

   settings->replaceFromWithTo =
       READ_CONFIG_BOOL(profile, MP_FVIEW_FROM_REPLACE);
   if ( settings->replaceFromWithTo )
   {
      String returnAddrs = READ_CONFIG(profile, MP_FROM_REPLACE_ADDRESSES);
      if ( returnAddrs == GetStringDefault(MP_FROM_REPLACE_ADDRESSES) )
      {
         // the default for this option is just the return address
         returnAddrs = READ_CONFIG_TEXT(profile, MP_FROM_ADDRESS);
      }

      settings->returnAddresses = strutil_restore_array(':', returnAddrs);
   }

   settings->previewOnSingleClick =
      READ_CONFIG_BOOL(profile, MP_PREVIEW_ON_SELECT);

   settings->previewDelay = READ_CONFIG(profile, MP_FVIEW_PREVIEW_DELAY);

   ReadColumnsInfo(profile, settings->columns);
}

void
wxFolderView::ApplyOptions()
{
   m_FolderCtrl->DeleteAllItems();

   m_FolderCtrl->ApplyOptions(m_settings.FgCol,
                              m_settings.BgCol,
                              m_settings.font,
                              m_settings.size,
                              m_settings.columns);
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

   // same as previewOnSingleClick
   m_settings.previewDelay = settings.previewDelay;

   // did any other, important, setting change?
   if ( settings != m_settings )
   {
      // yes: need to repopulate the list ctrl because its appearance changed
      m_settings = settings;

      ApplyOptions();

      if ( m_ASMailFolder )
         Update();
   }

   // do it unconditionally as it's fast
   m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);
   m_FolderCtrl->SetPreviewDelay(m_settings.previewDelay);

   m_FolderCtrl->UpdateSortIndicator();

   m_MessageWindow->UpdateOptions();
}

// ----------------------------------------------------------------------------
// wxFolderView open a folder
// ----------------------------------------------------------------------------

void
wxFolderView::Update()
{
   MailFolder_obj mf = GetMailFolder();
   CHECK_RET( mf, "wxFolderView::Update(): no folder" );

   m_FolderCtrl->UpdateListing(mf->GetHeaders());

   wxLogTrace(M_TRACE_FV_UPDATE, "wxFolderView::Update(): %ld headers.",
              m_FolderCtrl->GetItemCount());

   m_nDeleted = UID_ILLEGAL;

   UpdateTitleAndStatusBars("", "", m_Frame, mf);
}

void
wxFolderView::Clear(bool keepTheViewer)
{
   // really clear the GUI
   m_FolderCtrl->DeleteAllItems();
   if ( m_MessagePreview )
      m_MessagePreview->Clear();

   // reset them as they don't make sense for the new folder
   m_FolderCtrl->InvalidatePreview();

   if ( m_ASMailFolder )
   {
      // clean up old folder

      // NB: the test for m_MessagePreview is needed because of wxMSW bug which
      //     prevents us from showing a dialog box when called from dtor and it
      //     is NULL only when we're in dtor
      if ( m_MessagePreview )
      {
         if ( GetHeadersCount() > 0 &&
             (m_ASMailFolder->GetType() == MF_NNTP ||
              m_ASMailFolder->GetType() == MF_NEWS) )
         {
            MailFolder_obj mf = GetMailFolder();
            if ( mf && mf->IsOpened() )
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
                  m_ASMailFolder->SetFlagForAll(MailFolder::MSG_STAT_DELETED);
               }
            }
         }

         CheckExpungeDialog(m_ASMailFolder, m_Frame);
      }

      // this folder is not associated with our frame any more
      MailFolder_obj mf = m_ASMailFolder->GetMailFolder();
      mf->SetInteractiveFrame(NULL);

      m_ASMailFolder->DecRef();
      m_ASMailFolder = NULL;

      m_msgCmdProc->SetFolder(NULL);

      if ( !keepTheViewer && m_MessagePreview )
      {
         m_MessagePreview->SetFolder(NULL);
         m_MessageWindow->UpdateOptions();
      }
   }
   //else: no old folder

   if ( m_Profile )
   {
      m_Profile->DecRef();

      m_Profile = NULL;
   }

   // save the columns widths of the old folder view
   m_FolderCtrl->SaveColWidths();

   // disable the message menu as we have no folder
   EnableMMenu(MMenu_Message, m_FolderCtrl, false);
}

void
wxFolderView::ShowFolder(MailFolder *mf)
{
   CHECK_RET( mf, "NULL MailFolder in wxFolderView::ShowFolder" );

   m_ASMailFolder = ASMailFolder::Create(mf);
   m_msgCmdProc->SetFolder(m_ASMailFolder);

   // this is not supposed to happen, it's an internal operation
   CHECK_RET( m_ASMailFolder, "ASMailFolder::Create() failed??" );

   m_Profile = m_ASMailFolder->GetProfile();
   if ( m_Profile )
      m_Profile->IncRef();

   m_MessageWindow->UpdateOptions();
   m_MessagePreview->SetFolder(m_ASMailFolder);

   m_folderName = m_ASMailFolder->GetName();

   // read in our new profile settigns
   ReadProfileSettings(&m_settings);

   m_FolderCtrl->OnFolderChange();
   ApplyOptions();

   m_msgCmdProc->SetWindowForDnD(m_FolderCtrl);

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
   if ( GetHeadersCount() == 0 )
   {
      Update();
   }

   m_FocusFollowMode = READ_CONFIG_BOOL(m_Profile, MP_FOCUS_FOLLOWSMOUSE);

   // so we can react to keyboard events
   m_FolderCtrl->SetFocus();

   EnableMMenu(MMenu_Message, m_FolderCtrl, true);
}

void
wxFolderView::SetFolder(MailFolder *mf)
{
   // probably not needed, but hold on it to make sure it doesn't go away
   // before we call ShowFolder()
   if ( mf )
   {
      mf->IncRef();
   }

   // keep the viewer (by passing true to Clear()) only if we're going to open
   // a new folder soon: this avoids flicker but still ensures that we close
   // the current viewer if we are not going to open any folder
   Clear(mf != NULL);

   if ( mf )
   {
      ShowFolder(mf);

      // to compensate IncRef() above
      mf->DecRef();
   }
}

bool
wxFolderView::OpenFolder(MFolder *folder, bool readonly)
{
   CHECK( folder, false, "NULL folder in wxFolderView::OpenFolder" );

   m_fullname = folder->GetFullName();

   int flags = folder->GetFlags();

   // special check for IMAP servers (i.e. IMAP folders without path): we
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
             m_Frame,
             MDIALOG_YESNOTITLE,
             true,
             GetFullPersistentKey(M_MSGBOX_BROWSE_IMAP_SERVERS)
           ) )
      {
         // create all folders under the IMAP server
         ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folder);
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

         return false;
      }
      //else: continue opening the server, this will just open INBOX on it
   }

   // check if we didn't mark this folder as unopenable the last time
   if ( (flags & MF_FLAGS_UNACCESSIBLE) && !(flags & MF_FLAGS_MODIFIED) )
   {
      // note that we don't use GetFullPersistentKey() here as we want this
      // setting to be global (it isn't very useful to disable this msg box
      // only for one folder)
      if ( !MDialog_YesNoDialog
           (
            wxString::Format(
               _("The folder '%s' couldn't be opened last time, "
                 "do you still want to try to open it (it "
                 "will probably fail again)?"),
               m_fullname.c_str()
            ),
            m_Frame,
            MDIALOG_YESNOTITLE,
            false, // [No] default
            GetPersMsgBoxName(M_MSGBOX_OPEN_UNACCESSIBLE_FOLDER)
           ) )
      {
         // no, we don't want to open it again
         mApplication->SetLastError(M_ERROR_CANCEL);
         return false;
      }

      if ( MDialog_YesNoDialog
           (
            _("Would you like to change folder "
              "settings before trying to open it?"),
            m_Frame,
            MDIALOG_YESNOTITLE,
            false,
            GetPersMsgBoxName(M_MSGBOX_CHANGE_UNACCESSIBLE_FOLDER_SETTINGS)
         ) )
      {
         // invoke the folder properties dialog
         if ( !ShowFolderPropertiesDialog(folder, m_Frame) )
         {
            // the dialog was cancelled
            wxLogStatus(m_Frame, _("Opening the folder '%s' cancelled."),
                        m_fullname.c_str());

            mApplication->SetLastError(M_ERROR_CANCEL);
            return false;
         }
      }
   }

   wxBeginBusyCursor();

   MailFolder *mf = MailFolder::OpenFolder(folder,
                                           readonly ? MailFolder::ReadOnly
                                                    : MailFolder::Normal,
                                           m_Frame);
   SetFolder(mf);

   if ( mf )
   {
      mf->DecRef();
   }

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
                           "to add its subfolders to the folder tree.\n"
                           "\n"
                           "If you believe this message to be incorrect, "
                           "you may reset \"Can be opened\" flag in the\n"
                           "folder properties dialog and try again."),
                         m_fullname.c_str());
            break;

         default:
            // TODO analyze the error here!

            // set the flag saying that this folder couldn't be opened which
            // will be used when the user tries to open it the next time

            // it's not modified any more, even if it had been
            folder->ResetFlags(MF_FLAGS_MODIFIED);

            // ... and it is unacessible because we couldn't open it
            folder->AddFlags(MF_FLAGS_UNACCESSIBLE);

            // propose to show the folder properties dialog right here
            String key =
               GetFullPersistentKey(M_MSGBOX_EDIT_FOLDER_ON_OPEN_FAIL);
            if ( wxPMessageBoxEnabled(key) )
            {
               if ( MDialog_YesNoDialog
                    (
                     wxString::Format(_("The folder '%s' could not be opened, "
                                        "would you like to change its settings?"),
                                      m_fullname.c_str()),
                     m_Frame,
                     MDIALOG_YESNOTITLE,
                     true,
                     key
                    ) )
               {
                  MDialog_FolderProfile(m_Frame, m_fullname);
               }
            }
            else // msg box disabled, at least show the error msg
            {
               wxLogError(_("The folder '%s' could not be opened, "
                            "please check its settings."),
                          m_fullname.c_str());
            }
      }
   }
   else
   {
      // reset the unaccessible and modified flags this folder might have had
      // before as now we could open it
      folder->ResetFlags(MF_FLAGS_MODIFIED | MF_FLAGS_UNACCESSIBLE);
   }

   return mf != NULL;
}

// ----------------------------------------------------------------------------
// wxFolderView operations
// ----------------------------------------------------------------------------

void
wxFolderView::PreviewMessage(long uid)
{
   // show it in the preview window
   m_MessagePreview->ShowMessage(uid);
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
   if ( GetDeletedCount() )
   {
      m_ASMailFolder->ExpungeMessages();

      m_nDeleted = 0;
   }
   else
   {
      wxLogWarning(_("No deleted messages - nothing to expunge"));
   }
}


void wxFolderView::SelectAllByStatus(MailFolder::MessageStatus status,
                                     bool isSet)
{
   wxFolderListCtrlBlockOnSelect dontHandleOnSelect(m_FolderCtrl);

   HeaderInfoList_obj hil = GetFolder()->GetHeaders();
   CHECK_RET( hil, "can't select unread or flagged messages without folder listing" );

   MsgnoArray *indices = hil->GetAllHeadersByFlag(status, isSet);
   if ( !indices )
      return;

   size_t count = indices->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_FolderCtrl->Select(hil->GetIdxFromMsgno(indices->Item(n)), true);
   }

   delete indices;
}

void wxFolderView::SelectAll(bool on)
{
   wxFolderListCtrlBlockOnSelect dontHandleOnSelect(m_FolderCtrl);

   size_t numMessages = GetHeadersCount();
   for ( size_t n = 0; n < numMessages; n++ )
   {
      m_FolderCtrl->Select(n, on);
   }
}

// ----------------------------------------------------------------------------
// wxFolderView event handlers
// ----------------------------------------------------------------------------

bool
wxFolderView::HandleCharEvent(wxKeyEvent& event)
{
   // which command?
   // --------------

   long key = event.KeyCode();
   switch ( key )
   {
      case WXK_F1:
         mApplication->Help(MH_FOLDER_VIEW_KEYBINDINGS, GetWindow());
         return true;

      case WXK_PRIOR:
      case WXK_NEXT:
         // Shift-PageUp/Down have a predefined meaning in the wxListCtrl, so
         // let it have it
         if ( event.ShiftDown() )
         {
            event.Skip();
            return false;
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
                View, Group reply (== followup), List reply
            */
            static const char keycodes_en[] = gettext_noop("DUXCSMRFOPHGL");
            static const char *keycodes = _(keycodes_en);

            key = toupper(key);

            int idx = 0;
            for ( ; keycodes[idx] && keycodes[idx] != key; idx++ )
               ;

            key = keycodes[idx] ? keycodes_en[idx] : 0;
         }
   }

   // we only process unmodified key presses normally - except for Ctrl-Del
   // and '*' (which may need Shift on some keyboards (including US))
   if ( event.AltDown() ||
        (event.ShiftDown() && key != '*') ||
        (event.ControlDown() && key != 'D') )
   {
      event.Skip();
      return false;
   }

   // do command
   // ----------

   // we can operate either on all selected items
   const UIdArray& selections = GetSelections();

   // get the focused item
   long focused = m_FolderCtrl->GetFocusedItem();

   // find the next item
   long newFocus = focused;
   if ( newFocus == -1 )
      newFocus = 0;
   else if ( focused < m_FolderCtrl->GetItemCount() - 1 )
      newFocus++;

   switch ( key )
   {
      case 'M': // move
         if ( !m_msgCmdProc->ProcessCommand(WXMENU_MSG_MOVE_TO_FOLDER,
                                            selections) )
         {
            // don't delete the messages if we couldn't save them!
            newFocus = -1;
         }
         break;

      case 'D': // delete
         if ( event.ControlDown() )
         {
            m_msgCmdProc->ProcessCommand(WXMENU_MSG_DELETE_EXPUNGE, selections);
            newFocus = -1;
         }
         else // normal delete
         {
            m_msgCmdProc->ProcessCommand(WXMENU_MSG_DELETE, selections);

            // only move on if we mark as deleted, for trash usage, selection
            // remains the same:
            if ( READ_CONFIG(m_Profile, MP_USE_TRASH_FOLDER) )
            {
               // don't move focus
               newFocus = -1;
            }
         }
         break;

      case 'U': // undelete
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_UNDELETE, selections);
         break;

      case 'X': // expunge
         ExpungeMessages();
         newFocus = -1;
         break;

      case 'C': // copy (to folder)
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_SAVE_TO_FOLDER, selections);
         break;

      case 'S': // save (to file)
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_SAVE_TO_FILE, selections);
         break;

      case 'R': // reply
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_REPLY, selections);
         newFocus = -1;
         break;

      case 'G': // group reply
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_REPLY_ALL, selections);
         newFocus = -1;
         break;

      case 'L': // list reply
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_REPLY_LIST, selections);
         newFocus = -1;
         break;

      case 'F': // forward
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_FORWARD, selections);
         newFocus = -1;
         break;

      case 'O': // open
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_OPEN, selections);
         break;

      case 'P': // print
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_PRINT, selections);
         break;

      case 'H': // headers
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_TOGGLEHEADERS, selections);

         // don't move focus
         newFocus = -1;
         break;

      case '*':
         m_msgCmdProc->ProcessCommand(WXMENU_MSG_FLAG, selections);

         // don't move focus
         newFocus = -1;
         break;

      case WXK_NEXT:
         // scroll down the preview window
         if ( m_FolderCtrl->IsPreviewed(focused) )
         {
            if ( !m_MessagePreview->PageDown() )
            {
               // go to the next message if we were already at the end
               MoveToNextUnread();
            }
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
         if ( m_FolderCtrl->IsPreviewed(focused) )
         {
            m_MessagePreview->PageUp();
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

      case WXK_UP:
      case WXK_DOWN:
      case 0:
         event.Skip();
         return false;
   }

   if ( newFocus != -1 )
   {
      // move focus, possibly selecting the new item as well
      (void)m_FolderCtrl->GoToItem(newFocus);
   }

   return true;
}

void
wxFolderView::OnCommandEvent(wxCommandEvent& event)
{
   int cmd = event.GetId();

   // we can process these commands even without having an opened folder
   if ( cmd >= WXMENU_FVIEW_POPUP_BEGIN && cmd < WXMENU_FVIEW_POPUP_END )
   {
      OnHeaderPopupMenu(cmd);

      return;
   }

   // but we can't do anything else if we're not opened
   if ( !m_ASMailFolder )
   {
      event.Skip();
      return;
   }

   const UIdArray& selections = GetSelections();
   if ( selections.IsEmpty() )
   {
      // nothing to do
      return;
   }

   // first check if it is not something which can be directly processed by
   // the message viewer
   if ( m_MessagePreview )
   {
      if ( m_msgCmdProc->ProcessCommand(cmd, selections) )
         return;
   }

   switch ( cmd )
   {
      case WXMENU_MSG_SEARCH:
         SearchMessages();
         break;

      case WXMENU_MSG_EXPUNGE:
         ExpungeMessages();
         break;

      case WXMENU_MSG_QUICK_FILTER:
         // create a filter for the (first of) currently selected message(s)
         m_TicketList->Add(m_ASMailFolder->GetMessage(GetFocus(), this));
         break;

      case WXMENU_MSG_NEXT_UNREAD:
         m_FolderCtrl->SelectNextByStatus(MailFolder::MSG_STAT_SEEN, FALSE);
         break;

      case WXMENU_MSG_NEXT_FLAGGED:
         m_FolderCtrl->SelectNextByStatus(MailFolder::MSG_STAT_FLAGGED, TRUE);
         break;

      case WXMENU_MSG_SELECTALL:
         SelectAll(true);
         break;

      case WXMENU_MSG_SELECTUNREAD:
         SelectAllByStatus(MailFolder::MSG_STAT_SEEN, FALSE);
         break;

      case WXMENU_MSG_SELECTFLAGGED:
         SelectAllByStatus(MailFolder::MSG_STAT_FLAGGED, TRUE);
         break;

      case WXMENU_MSG_DESELECTALL:
         SelectAll(false);
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(MH_FOLDER_VIEW, GetWindow());
         break;

      default:
         if ( cmd >= WXMENU_POPUP_FOLDER_MENU )
         {
            // it might be a folder from popup menu
            wxFolderMenu *menu = m_FolderCtrl->GetFolderMenu();
            if ( menu )
            {
               MFolder *folder = menu->GetFolder(cmd);
               if ( folder )
               {
                  // it is, move messages there (it is "quick move" menu)
                  m_msgCmdProc->ProcessCommand(WXMENU_MSG_MOVE_TO_FOLDER,
                                               selections, folder);

                  folder->DecRef();
               }
            }
         }
         break;
   }
}

void wxFolderView::OnHeaderPopupMenu(int cmd)
{
   // edit the global settings if we don't have any open folder
   Profile_obj profile(GetFolderProfile());

   switch ( cmd )
   {
      case WXMENU_FVIEW_CONFIG_SORT:
         if ( !ConfigureSorting(profile, GetWindow()) )
         {
            // nothing changed, skip MEventManager::Send() below
            return;
         }
         break;

      case WXMENU_FVIEW_CONFIG_THREAD:
         if ( !ConfigureThreading(profile, GetWindow()) )
         {
            // nothing changed, skip MEventManager::Send() below
            return;
         }
         break;

      case WXMENU_FVIEW_RESET_SORT:
         // we try to keep our profiles tidy and clean, so first just delete
         // the current sorting order
         profile->DeleteEntry(MP_MSGS_SORTBY);
         if ( READ_CONFIG(profile, MP_MSGS_SORTBY) )
         {
            // but it didn't help - our parent must have a non 0 sort order
            // too, so we're forced to override it in our profile
            profile->writeEntry(MP_MSGS_SORTBY, 0l);
         }
         break;

      case WXMENU_FVIEW_TOGGLE_THREAD:
         {
            bool thread = READ_CONFIG_BOOL(profile, MP_MSGS_USE_THREADING);
            profile->writeEntry(MP_MSGS_USE_THREADING, !thread);
         }
         break;

      case WXMENU_FVIEW_SIZE_AUTO:
      case WXMENU_FVIEW_SIZE_AUTOBYTES:
      case WXMENU_FVIEW_SIZE_BYTES:
      case WXMENU_FVIEW_SIZE_KBYTES:
      case WXMENU_FVIEW_SIZE_MBYTES:
         // this is an artefact of missing radio menu support in wxWin: the
         // user could just unselect the selected checkbox instead of choosing
         // another one, check for this - this will go away once true radio
         // menu items are supported
         cmd -= WXMENU_FVIEW_SIZE_AUTO;
         if ( cmd == (long)READ_CONFIG(profile, MP_FVIEW_SIZE_FORMAT) )
         {
            // nothing changed
            return;
         }

         // we rely on the fact the SIZE_XXX menu items are in the same order
         // as MessageSize_XXX constants
         profile->writeEntry(MP_FVIEW_SIZE_FORMAT, cmd);
         break;

      case WXMENU_FVIEW_CONFIG_DATEFMT:
         if ( !ConfigureDateFormat(profile, GetWindow()) )
         {
            // nothing changed, skip MEventManager::Send() below
            return;
         }
         break;

      case WXMENU_FVIEW_FROM_NAMES_ONLY:
         {
            bool namesOnly = READ_CONFIG_BOOL(profile, MP_FVIEW_NAMES_ONLY);
            profile->writeEntry(MP_FVIEW_NAMES_ONLY, !namesOnly);
         }
         break;

      case WXMENU_FVIEW_SORT_BY_STATUS:
      case WXMENU_FVIEW_SORT_BY_DATE:
      case WXMENU_FVIEW_SORT_BY_SIZE:
      case WXMENU_FVIEW_SORT_BY_FROM:
      case WXMENU_FVIEW_SORT_BY_SUBJECT:
      case WXMENU_FVIEW_SORT_BY_STATUS_REV:
      case WXMENU_FVIEW_SORT_BY_DATE_REV:
      case WXMENU_FVIEW_SORT_BY_SIZE_REV:
      case WXMENU_FVIEW_SORT_BY_FROM_REV:
      case WXMENU_FVIEW_SORT_BY_SUBJECT_REV:
         {
            bool reverse = cmd >= WXMENU_FVIEW_SORT_BY_STATUS_REV;
            wxFolderListColumn col = (wxFolderListColumn)
               (reverse ? cmd - WXMENU_FVIEW_SORT_BY_STATUS_REV
                        : cmd - WXMENU_FVIEW_SORT_BY_STATUS);

            ASSERT_MSG( col < WXFLC_NUMENTRIES, "invalid column" );

            long sortOrder = SortOrderFromCol(col);

            ASSERT_MSG( sortOrder != MSO_NONE, "invalid sort order" );

            m_FolderCtrl->SetSortOrder(profile,
                                       reverse ? sortOrder + 1 : sortOrder,
                                       col, reverse);

            // SetSortOrder() notifies about the options change event itself,
            // don't do it here
            return;
         }

      default:
         FAIL_MSG( "unexpected command" );
   }

   // this will resort/thread the messages and refresh us a bit later
   MEventManager::Send(
                        new MEventOptionsChangeData
                            (
                              profile,
                              MEventOptionsChangeData::Ok
                            )
                      );
}

// ----------------------------------------------------------------------------
// wxFolderView selection/focus management
// ----------------------------------------------------------------------------

void
wxFolderView::OnFocusChange(long idx, UIdType uid)
{
   wxLogTrace(M_TRACE_FV_SELECTION, "item %ld (uid = %lx) is now focused",
              idx, uid);

   if ( uid != UID_ILLEGAL && READ_CONFIG(m_Profile, MP_FVIEW_STATUS_UPDATE) )
   {
      HeaderInfoList_obj hil = GetFolder()->GetHeaders();
      CHECK_RET( hil, "failed to get headers" );

      wxString fmt = READ_CONFIG(m_Profile, MP_FVIEW_STATUS_FMT);
      HeaderVarExpander expander(hil[idx],
                                 m_settings.dateFormat,
                                 m_settings.dateGMT);

      wxLogStatus(m_Frame, ParseMessageTemplate(fmt, expander));
   }
      //else: no status message
}

UIdType
wxFolderView::GetPreviewUId(void) const
{
   return m_FolderCtrl->GetPreviewUId();
}

bool
wxFolderView::HasSelection() const
{
   return m_FolderCtrl->HasSelection();
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
// wxFolderView message viewer handling
// ----------------------------------------------------------------------------

void wxFolderView::OnMsgViewerChange(wxWindow *viewerNew)
{
   m_MessageWindow->SetViewerWindow(viewerNew);
}

// ----------------------------------------------------------------------------
// wxFolderView MEvent processing
// ----------------------------------------------------------------------------

void wxFolderView::OnFolderClosedEvent(MEventFolderClosedData& event)
{
   MailFolder_obj mf = GetMailFolder();

   if ( event.GetFolder() == mf )
   {
      wxLogTrace(M_TRACE_FV_UPDATE, "wxFolderView::Clear()");

      Clear();
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

      Clear();
   }
}

// this function is called when messages are deleted from folder but no new
// ones appear
void
wxFolderView::OnFolderExpungeEvent(MEventFolderExpungeData& event)
{
   MailFolder_obj mf = GetMailFolder();

   if ( event.GetFolder() != mf )
      return;

   // deal with the special case when we get the expunge notification before
   // we had a chance to show the headers at all - in this case just don't
   // process it as we don't need update (we need full refresh which will be
   // done soon)
   if ( !m_FolderCtrl->HasHeaders() )
      return;

   // if we had exactly one message selected before, we want to keep the
   // selection after expunging
   bool hadUniqueSelection = m_FolderCtrl->GetUniqueSelection() != -1;

   // we might have to clear the preview if we delete the message
   // being previewed
   bool previewDeleted = false;

   size_t n,
          count = event.GetCount();

   wxLogTrace(M_TRACE_FV_UPDATE, "wxFolderView::Expunge(%u items), now %ld",
              count, m_FolderCtrl->GetItemCount());

   HeaderInfoList_obj hil = GetFolder()->GetHeaders();

   long itemPreviewed = m_FolderCtrl->GetPreviewedItem();

   wxArrayLong itemsDeleted;
   itemsDeleted.Alloc(count);
   for ( n = 0; n < count; n++ )
   {
      // we need to get the display position from the msgnos we're given
      long item = event.GetItemPos(n);

      // we can't use m_FolderCtrl->GetUIdFromIndex(item) here because the item
      // is not in the headers any more, so we use indices instead of UIDs even
      // if it is less simple (we have to modify it to adjust for index offset)
      if ( !previewDeleted && item == itemPreviewed-- )
      {
         previewDeleted = true;
      }

      itemsDeleted.Add(item);
   }

   // really delete the items: notice that is is just fine that the indices
   // change while we delete the items because the msgnos we get from the event
   // are adjusted like this too (i.e. when the first 2 messages are expunged,
   // the msgnos are going to be "1,1" and not "1,2")
   count = itemsDeleted.GetCount();
   for ( n = 0; n < count; n++ )
   {
      m_FolderCtrl->DeleteItem(itemsDeleted[n]);
   }

   // clear preview window if the message showed there had been deleted
   if ( previewDeleted )
   {
      m_MessagePreview->Clear();
      m_FolderCtrl->InvalidatePreview();
   }

   if ( hadUniqueSelection )
   {
      // restore the selection
      long focus = m_FolderCtrl->GetFocusedItem();
      if ( focus != -1 )
      {
         m_FolderCtrl->Select(focus);
      }
   }

   // we don't have any deleted messages any more
   m_nDeleted = 0;

   UpdateTitleAndStatusBars("", "", m_Frame, mf);
}

// this function gets called when new mail appears in the folder
void
wxFolderView::OnFolderUpdateEvent(MEventFolderUpdateData &event)
{
   MailFolder_obj mf = GetMailFolder();

   if ( event.GetFolder() == mf )
   {
      Update();
   }
}

// this is called when the status of one message changes
void
wxFolderView::OnMsgStatusEvent(MEventMsgStatusData& event)
{
   MailFolder_obj mf = GetMailFolder();

   if ( event.GetFolder() == mf )
   {
      HeaderInfoList_obj hil = GetFolder()->GetHeaders();

      CHECK_RET( hil, "no headers but msg status changed?" );

      size_t posMin = (size_t)-1,
             posMax = 0;

      size_t count = event.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         MsgnoType msgno = event.GetMsgno(n);
         if ( msgno > hil->Count() )
         {
            // FIXME: message was expunged, what to do??
            continue;
         }

         size_t pos = hil->GetPosFromIdx(hil->GetIdxFromMsgno(msgno));
         if ( pos >= (size_t)m_FolderCtrl->GetItemCount() )
         {
            // this can happen if we didn't have to update the control yet,
            // just ignore the event then as we will get the message with the
            // correct status when we retrieve it from Update() anyhow
            continue;
         }

         // update m_nDeleted counter
         int statusOld = event.GetStatusOld(n),
             statusNew = event.GetStatusNew(n);

         // what we do here is "+= isDeleted - wasDeleted"
         int deletedChange = (statusNew & MailFolder::MSG_STAT_DELETED)
                           - (statusOld & MailFolder::MSG_STAT_DELETED);
         if ( deletedChange )
         {
            m_nDeleted = GetDeletedCount() + deletedChange;
         }

         // remember the items to update
         if ( pos < posMin )
            posMin = pos;
         if ( pos > posMax )
            posMax = pos;
      }

      // update the affected item appearance
      if ( posMin != (size_t)-1 )
      {
         m_FolderCtrl->RefreshItems(posMin, posMax);
      }

      // update the number of unread messages showin in the title/status bars
      UpdateTitleAndStatusBars("", "", m_Frame, mf);
   }
}

// ----------------------------------------------------------------------------
// wxFolderView async stuff
// ----------------------------------------------------------------------------

void
wxFolderView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   ASMailFolder::Result *result = event.GetResult();

   if ( result->GetUserData() == this )
   {
      const Ticket& t = result->GetTicket();

      ASSERT_MSG( m_TicketList->Contains(t), "unexpected async result ticket" );

      m_TicketList->Remove(t);

      int ok = ((ASMailFolder::ResultInt *)result)->GetValue() != 0;

      String msg;
      switch ( result->GetOperation() )
      {
         case ASMailFolder::Op_SearchMessages:
            ASSERT(result->GetSequence());
            if( ok )
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
                      return msgnos from search directly... (FIXME)
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

         case ASMailFolder::Op_ApplyFilterRules:
            // we get this one when applying newly created Quick Filter rule
            // from the code above
            break;

         default:
            FAIL_MSG( "unexpected async operation result" );
      }
   }
   //else: not out result at all

   result->DecRef();
}

// ============================================================================
// wxFolderViewFrame
// ============================================================================

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFolderViewFrame, wxMFrame)
   EVT_MENU(-1,    wxFolderViewFrame::OnCommandEvent)
   EVT_TOOL(-1,    wxFolderViewFrame::OnCommandEvent)

   // only enable message operations if the selection is not empty
   // (the range should contain _only_ these operations!)
   EVT_UPDATE_UI_RANGE(WXMENU_MSG_OPEN, WXMENU_MSG_UNDELETE,
                       wxFolderViewFrame::OnUpdateUI)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxFolderViewFrame, wxMFrame)

// ----------------------------------------------------------------------------
// wxFolderViewFrame ctor and such
// ----------------------------------------------------------------------------

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
wxFolderViewFrame::Create(MFolder *folder,
                          wxMFrame *parent,
                          MailFolder::OpenMode openmode)
{
   if ( !parent )
   {
      parent = mApplication->TopLevelFrame();
   }

   wxFolderViewFrame *
      frame = new wxFolderViewFrame(folder->GetFullName(), parent);

   wxFolderView *fv = wxFolderView::Create(frame);

   // doesn't make sense to half open we're going to view
   ASSERT_MSG( openmode != MailFolder::HalfOpen,
               "invalid open mode in wxFolderViewFrame::Create" );

   if ( !fv->OpenFolder(folder, openmode == MailFolder::ReadOnly) )
   {
      delete fv;
      delete frame;
      return NULL;
   }

   frame->InternalCreate(fv, parent);

   return frame;
}

wxFolderViewFrame::wxFolderViewFrame(const String& name, wxMFrame *parent)
                 : wxMFrame(name, parent)
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
   delete m_FolderView;
}

// ----------------------------------------------------------------------------
// wxFolderViewFrame event handlers
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// other public functions (from include/FolderView.h)
// ----------------------------------------------------------------------------

bool OpenFolderViewFrame(MFolder *folder,
                         wxWindow *parent,
                         MailFolder::OpenMode openmode)
{
   return wxFolderViewFrame::Create(folder,
                                    (wxMFrame *)GetFrame(parent),
                                    openmode) != NULL;
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
   strWidths = UnpackWidths(READ_CONFIG_PRIVATE(profile, FOLDER_LISTCTRL_WIDTHS));
   strWidthsStandard = UnpackWidths(FOLDER_LISTCTRL_WIDTHS_D);

   wxArrayString choices;
   wxArrayInt status;
   size_t n;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      // find the n-th column shown currently
      wxFolderListColumn col = GetColumnByIndex(columns, n);
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
      wxFolderListColumn col = GetColumnByName(choices[n]);
      columns[col] = status[n] ? index++ : -1;
   }

   WriteColumnsInfo(profile, columns);

   // update the widths
   wxArrayString strWidthsNew;
   for ( n = 0; n < WXFLC_NUMENTRIES; n++ )
   {
      // find the n-th column shown currently
      wxFolderListColumn col = GetColumnByIndex(columns, n);
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
   return dateFormat == other.dateFormat &&
          dateGMT == other.dateGMT &&
          senderOnlyNames == other.senderOnlyNames &&
          replaceFromWithTo == other.replaceFromWithTo &&
          showSize == other.showSize &&
          memcmp(columns, other.columns, sizeof(columns)) == 0 &&
          returnAddresses == other.returnAddresses;
}

