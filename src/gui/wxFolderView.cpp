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

#  include <wx/dynarray.h>
#  include <wx/colour.h>
#endif

#include <ctype.h>
#include "XFace.h"

#include <wx/file.h>
#include <wx/listctrl.h>
#include <wx/menuitem.h>
#include <wx/dnd.h>
#include <wx/fontmap.h>
#include <wx/encconv.h>
#include "wx/persctrl.h"

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
   //#define BROKEN_LISTCTRL
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
// wxFolderListCtrl: the list ctrl showing the messages in the folder
// ----------------------------------------------------------------------------

class wxFolderListCtrl : public wxListView
{
public:
   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   virtual ~wxFolderListCtrl();

   // set the listing to use
   void SetListing(HeaderInfoList *listing);

   // get the number of items we show
   size_t GetHeadersCount() const { return m_headers ? m_headers->Count() : 0; }

   // invlaidate the cached header(s)
   void InvalidateCache();

   // create the columns using order in m_columns and widths from profile
   void CreateColumns();

   // return the string containing ':' separated columns widths
   String GetWidths() const;

   /// select the item currently focused
   void SelectFocused() { Select(GetFocusedItem(), true); }

   /** select the next (after the current one) message in the control which
       has the given status bit on or off, depending on the second parameter

       @param status the status bit to check
       @param isSet go to message with status bit set if true, unset if false
       @return true if we found such item
   */
   bool SelectNextByStatus(MailFolder::MessageStatus status, bool isSet);

   /// select next unread or flagged message after the given one
   UIdType SelectNextUnreadAfter(const HeaderInfoList_obj& hil,
                                 long indexStart = -1,
                                 MailFolder::MessageStatus status =
                                    MailFolder::MSG_STAT_SEEN,
                                 bool isSet = FALSE);

   /// focus the given item and ensure it is visible
   void Focus(long index);

   /// return true if we preview this item
   bool IsPreviewed(long item) const
      { return item == m_itemPreviewed; }

   /// return true if we preview the item with this UID
   bool IsUIdPreviewed(UIdType uid) const
      { return m_FolderView->GetPreviewUId() == uid; }

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

   /// for wxFolderView
   wxFolderMenu *GetFolderMenu() const { return m_menuFolders; }

   /// reenable redrawing
   void Thaw() { if ( m_isFrozen ) { m_isFrozen = false; Refresh(); } }

protected:
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
   void UpdateUniqueSelFlag()
   {
      m_selIsUnique = GetUniqueSelection() != -1;
   }

   /// do we have a folder opened?
   bool HasFolder() const { return m_FolderView->GetFolder() != NULL; }

   /// get the folder view settings we use
   const wxFolderView::AllProfileSettings& GetSettings() const
      { return m_FolderView->m_settings; }

   /// get the header info for this index
   HeaderInfo *GetHeaderInfo(size_t index) const;

   /// the listing to use
   HeaderInfoList *m_headers;

   /// cached header info (used by OnGetItemXXX())
   HeaderInfo *m_hiCached;

   /// the index of m_hiCached in m_headers (or -1 if not cached)
   size_t m_indexHI;

   /// the last headers list modification "date"
   HeaderInfoList::LastMod m_cacheLastMod;

   // TODO: we should have the standard attributes for each of the possible
   //       message states/colours (new/recent/unread/flagged/deleted) and
   //       reuse them in OnGetItemAttr(), this should be quite faster

   /// cached attribute
   wxListItemAttr *m_attr;

   /// parent window
   wxWindow *m_Parent;

   /// the folder view
   wxFolderView *m_FolderView;

   /// true until SelectInitialMessage() is called, don't update the control
   bool m_isFrozen;

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
// FolderViewAsyncStatus: this object is constructed before launching an async
// operation to show some initial message
//
// Then it must be given the ticket to monitor - if the ticket is invalid, it
// deletes itself (with an error message) and the story stops here, otherwise
// it waits untiul OnASFolderResultEvent() deletes it and gives the final
// message indicating either success (default) or failure (if Fail() was
// called)
// ----------------------------------------------------------------------------

class FolderViewAsyncStatus
{
public:
   FolderViewAsyncStatus(wxFolderView *fv, const char *fmt, ...)
   {
      m_folderView = fv;
      m_ticket = ILLEGAL_TICKET;

      va_list argptr;
      va_start(argptr, fmt);
      m_msgInitial.PrintfV(fmt, argptr);
      va_end(argptr);

      m_folderView->AddAsyncStatus(this);

      wxLogStatus(m_folderView->GetParentFrame(), m_msgInitial);
      wxBeginBusyCursor();
   }

   // monitor the given ticket, give error message if the corresponding
   // operation terminates with an error
   bool Monitor(Ticket ticket, const char *fmt, ...)
   {
      va_list argptr;
      va_start(argptr, fmt);
      m_msgError.PrintfV(fmt, argptr);
      va_end(argptr);

      m_ticket = ticket;

      if ( ticket == ILLEGAL_TICKET )
      {
         // self delete as wxFolderView will never do it as it will never get
         // the notifications for the invalid ticket
         m_folderView->RemoveAsyncStatus(this);

         delete this;

         return FALSE;
      }

      m_folderView->GetTicketList()->Add(ticket);

      return TRUE;
   }

   // used by OnASFolderResultEvent() to find the matching progress indicator
   Ticket GetTicket() const { return m_ticket; }

   // use to indicate that our operation finally failed
   void Fail() { m_ticket = ILLEGAL_TICKET; }

   // use different message on success (default is initial message + done)
   void SetSuccessMsg(const char *fmt, ...)
   {
      va_list argptr;
      va_start(argptr, fmt);
      m_msgOk.PrintfV(fmt, argptr);
      va_end(argptr);
   }

   // give the appropariate message
   ~FolderViewAsyncStatus()
   {
      if ( m_ticket == ILLEGAL_TICKET )
      {
         // also put it into the status bar to overwrite the previous message
         // there
         wxLogStatus(m_folderView->GetParentFrame(), m_msgError);

         // and show the error to the user
         wxLogError(m_msgError);
      }
      else // success
      {
         if ( m_msgOk.empty() )
         {
            m_msgOk << m_msgInitial << _(" done.");
         }
         //else: set explicitly, use it as is

         wxLogStatus(m_folderView->GetParentFrame(), m_msgOk);
      }

      wxEndBusyCursor();
   }

private:
   // the folder view which initiated the async operation
   wxFolderView *m_folderView;

   // the ticket for our operation
   Ticket m_ticket;

   // the initial message and the final message in case of success and failure
   wxString m_msgInitial,
            m_msgOk,
            m_msgError;
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

// check if the status bit is set/unset
inline bool CheckStatusBit(int status, int bit, bool isSet)
{
   return !(status & bit) == !isSet;
}

// used for array sorting
int CMPFUNC_CONV compareLongsReverse(long *first, long *second)
{
   return *second - *first;
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
   EVT_LIST_KEY_DOWN(-1, wxFolderListCtrl::OnListKeyDown)

   EVT_IDLE(wxFolderListCtrl::OnIdle)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderListCtrl ctor/dtor
// ----------------------------------------------------------------------------

wxFolderListCtrl::wxFolderListCtrl(wxWindow *parent, wxFolderView *fv)
{
   m_Parent = parent;
   m_profile = fv->GetProfile();

   m_headers = NULL;
   m_attr = NULL;
   InvalidateCache();

   m_PreviewOnSingleClick = false;

   m_profile->IncRef(); // we wish to keep it until dtor
   m_FolderView = fv;
   m_enableOnSelect = true;
   m_menu = NULL;
   m_menuFolders = NULL;

   m_isInPopupMenu = false;

#ifdef BROKEN_LISTCTRL
   m_suppressFocusTracking = false;
#endif // BROKEN_LISTCTRL

   // start in frozen state, wxFolderView should call Thaw() later
   m_isFrozen = true;

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
          wxLC_REPORT | wxLC_VIRTUAL | wxNO_BORDER);

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

   SafeDecRef(m_headers);

   delete m_attr;

   m_profile->DecRef();

   delete m_menuFolders;
   delete m_menu;
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

// ----------------------------------------------------------------------------
// wxFolderListCtrl event handlers
// ----------------------------------------------------------------------------

void wxFolderListCtrl::OnChar(wxKeyEvent& event)
{
   // don't process events until we're fully initialized and also only process
   // simple characters here (without Ctrl/Alt)
   if( !HasFolder() ||
       !m_FolderView->m_MessagePreview ||
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
      case 'M': // move
         if ( !m_FolderView->MoveMessagesToFolder(selections) )
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

      case 'C': // copy (to folder)
         m_FolderView->SaveMessagesToFolder(selections);
         break;

      case 'S': // save (to file)
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

   if ( HasFolder() )
   {
      // start the drag and drop operation
      if ( event.Dragging() )
      {
         if ( m_FolderView->DragAndDropMessages() )
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
      new wxMessageViewFrame(m_FolderView->GetFolder(), uid, m_FolderView);
   }
   else // preview on double click
   {
      m_FolderView->PreviewMessage(uid);
   }
}

void wxFolderListCtrl::OnSelected(wxListEvent& event)
{
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
   mApplication->UpdateAwayMode();

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
   mApplication->UpdateAwayMode();

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

// ----------------------------------------------------------------------------
// header info
// ----------------------------------------------------------------------------

void wxFolderListCtrl::InvalidateCache()
{
   m_indexHI = (size_t)-1;
   m_hiCached = NULL;
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

   SetItemCount(GetHeadersCount());

   // we'll crash if we use it after the listing changed!
   ASSERT_MSG( !m_hiCached, "should be reset" );
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
      self->m_hiCached = m_headers->GetItem(index);
      self->m_indexHI = index;
   }

   return m_hiCached;
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

// ----------------------------------------------------------------------------
// wxFolderListCtrl focus/selection management
// ----------------------------------------------------------------------------

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

   if ( selIsUnique && m_itemFocus != -1 )
   {
      // will set m_selIsUnique to true back again
      Select(m_itemFocus, true);
   }
}

void wxFolderListCtrl::Focus(long index)
{
   SetItemState(index, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED);

   // this doesn't work well with wxWin <= 2.2.5 in debug mode as calling
   // EnsureVisible() results in an assert failure which is harmless but
   // _very_ annoying as it happens all the time
#if !defined(__WXDEBUG__) || wxCHECK_VERSION(2,2,6)
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
#endif // wxWin >= 2.2.6

   // we will need the items nearby as well
   if ( m_headers )
   {
      long page = GetCountPerPage(),
           total = GetItemCount();

      // cache slightly more than the visible page around the current message
      page *= 3;
      page /= 2;

      long from = index - page / 2;
      if ( from < 0 )
      {
         from = 0;
      }

      long to = from + page;
      if ( to >= total )
         to = total - 1;

      m_headers->HintCache((size_t)from, (size_t)to);
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

   if ( m_isFrozen )
   {
      return text;
   }

   // this does happen because the number of messages in m_headers is
   // decremented first and our OnFolderExpungeEvent() is called much later
   // (during idle time), so we have no choice but to ignore the requests for
   // non existing items
   if ( (size_t)item >= GetHeadersCount() )
   {
      return text;
   }

   HeaderInfo *hi = GetHeaderInfo((size_t)item);
   CHECK( hi, text, "no header info in OnGetItemText" );

   wxFolderListCtrlFields field = GetColumnByIndex(m_columns, column);
   switch ( field )
   {
      case WXFLC_STATUS:
         text = MailFolder::ConvertMessageStatusToString(hi->GetStatus());
         break;

      case WXFLC_FROM:
         // optionally replace the "From" with "To: someone" for messages sent
         // by the user himself
         switch ( HeaderInfo::GetFromOrTo(hi,
                                          GetSettings().replaceFromWithTo,
                                          GetSettings().returnAddresses,
                                          &text) )
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

         // optionally leave only the name part of the address
         if ( GetSettings().senderOnlyNames )
         {
            text = Message::GetNameFromAddress(text);
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
      GetColourByName(&col, hi->GetColour(), MP_FVIEW_FGCOLOUR_D);
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
   if ( m_isFrozen )
   {
      return NULL;
   }

   // see comment in the beginning of OnGetItemText()
   if ( (size_t)item >= GetHeadersCount() )
   {
      return NULL;
   }

   HeaderInfo *hi = GetHeaderInfo((size_t)item);
   CHECK( hi, NULL, "no header info in OnGetItemText" );

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
// wxFolderListCtrl "select next" logic
// ----------------------------------------------------------------------------

bool
wxFolderListCtrl::SelectNextByStatus(MailFolder::MessageStatus status,
                                     bool isSet)
{
   HeaderInfoList_obj hil = m_FolderView->GetFolder()->GetHeaders();
   if( !hil || hil->Count() == 0 )
   {
      // cannot do anything without listing
      return false;
   }

   long idxFocused = GetFocusedItem();
   UIdType uid = SelectNextUnreadAfter(hil, idxFocused, status, isSet);
   if ( uid != UID_ILLEGAL )
   {
      // unselect the previously selected message if there was exactly one
      // selected - i.e. move selection to the new focus
      if ( m_selIsUnique )
      {
         Select(m_itemFocus, false);
      }

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
            CheckStatusBit(hil[idxFocused]->GetStatus(), status, isSet) )
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


UIdType
wxFolderListCtrl::SelectNextUnreadAfter(const HeaderInfoList_obj& hil,
                                        long idxFocused,
                                        MailFolder::MessageStatus status,
                                        bool isSet)
{
   size_t idx = hil->FindHeaderByFlagWrap(status, isSet, idxFocused);

   if ( idx != INDEX_ILLEGAL )
   {
      const HeaderInfo *hi = hil[idx];
      CHECK( hi, UID_ILLEGAL, "failed to get header" );

      Focus(idx);

      return hi->GetUId();
   }

   return UID_ILLEGAL;
}

// ============================================================================
// wxFolderView implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFolderView ctor and dtor
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// wxFolderView initialization
// ----------------------------------------------------------------------------

inline size_t wxFolderView::GetHeadersCount() const
{
   return m_FolderCtrl->GetHeadersCount();
}

void
wxFolderView::SelectInitialMessage(const HeaderInfoList_obj& hil)
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
      // select first unread is off or no unread message, so select the first
      // or the last one depending on the options
      unsigned long idx
         = READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTMESSAGE)
            ? 0
            : numMessages - 1;

      // note that idx is always a valid index because numMessages >= 1

      m_FolderCtrl->Focus(idx);

      const HeaderInfo *hi = hil[idx];
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
      m_nDeleted = 0;
      Update();
   }

   m_FolderCtrl->SetPreviewOnSingleClick(m_settings.previewOnSingleClick);
}

// ----------------------------------------------------------------------------
// wxFolderView open a folder
// ----------------------------------------------------------------------------

void
wxFolderView::Update()
{
   if ( !m_ASMailFolder )
      return;

   m_FolderCtrl->SetListing(m_ASMailFolder->GetHeaders());

   m_nDeleted = m_MailFolder->CountDeletedMessages();

   UpdateTitleAndStatusBars("", "", m_Frame, m_MailFolder);
}

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
         if( GetHeadersCount() > 0 &&
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
      if ( GetHeadersCount() == 0 )
      {
         Update();
      }

      // the list control is created in "frozen" state, i.e. it doesn't show
      // anything before we thaw it: this allows us to avoid retrieving the
      // headers in the start of the folder if we're going to scroll to the
      // first unread message (which is usually near the end) as the first
      // thing anyhow
      SelectInitialMessage(m_ASMailFolder->GetHeaders());
      m_FolderCtrl->Thaw();

      m_FocusFollowMode = READ_CONFIG(m_Profile, MP_FOCUS_FOLLOWSMOUSE) != 0;
      if ( wxWindow::FindFocus() != m_FolderCtrl )
      {
         // so we can react to keyboard events
         m_FolderCtrl->SetFocus();
      }
   }
   //else: no new folder

   EnableMMenu(MMenu_Message, m_FolderCtrl, (m_ASMailFolder != NULL) );
   m_SetFolderSemaphore = false;
}

bool
wxFolderView::OpenFolder(MFolder *folder)
{
   CHECK( folder, false, "NULL folder in wxFolderView::OpenFolder" );

   // just a cast
   wxFrame *frame = m_Frame;
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
            _("This folder couldn't be opened last time, "
              "do you still want to try to open it (it "
              "will probably fail again)?"),
            frame,
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
                        m_fullname.c_str());

            mApplication->SetLastError(M_ERROR_CANCEL);
            return false;
         }
      }
   }

   wxBeginBusyCursor();
   MailFolder::SetInteractive(m_Frame, m_fullname);

   MailFolder *mf = MailFolder::OpenFolder(folder);
   SetFolder(mf);
   SafeDecRef(mf);

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
      m_FolderCtrl->Select(indices->Item(n), true);
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

void
wxFolderView::OnCommandEvent(wxCommandEvent& event)
{
   // we can't do much if we're not opened
   if ( !m_ASMailFolder )
   {
      event.Skip();
      return;
   }

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

#if defined(EXPERIMENTAL_MARK_READ)
      case WXMENU_MSG_MARK_READ:
         MarkRead(GetSelections(), true);
         break;

      case WXMENU_MSG_MARK_UNREAD:
         MarkRead(GetSelections(), false);
         break;
#endif // EXPERIMENTAL_MARK_READ

      case WXMENU_MSG_OPEN:
         OpenMessages(GetSelections());
         break;

      case WXMENU_MSG_SAVE_TO_FOLDER:
         SaveMessagesToFolder(GetSelections());
         break;

      case WXMENU_MSG_MOVE_TO_FOLDER:
         MoveMessagesToFolder(GetSelections());
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

            size_t count = selections.GetCount();
            if ( count )
            {
               FolderViewAsyncStatus *status =
                  new FolderViewAsyncStatus(this,
                                            _("Applying filter rules to %u "
                                              "messages..."), count);
               Ticket t = m_ASMailFolder->ApplyFilterRules(&selections, this);
               if ( status->Monitor(t, _("Failed to apply filter rules.")) )
               {
                  status->SetSuccessMsg(_("Applied filters to %u messages, "
                                          "see log window for details."),
                                        count);
               }
            }
            else // no messages
            {
               wxLogStatus(m_Frame, _("Please select messages to filter."));
            }
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
         m_FolderCtrl->SelectNextByStatus(MailFolder::MSG_STAT_SEEN, FALSE);
         break;

      case WXMENU_MSG_NEXT_FLAGGED:
         m_FolderCtrl->SelectNextByStatus(MailFolder::MSG_STAT_FLAGGED, TRUE);
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

            MProgressDialog *dlg;
            if ( count > 10 )  // FIXME: hardcoded
            {
               dlg = new MProgressDialog
                         (
                           _("Extracting addresses"),
                           _("Scanning messages..."),
                           count,
                           m_Frame,     // parent
                           true,        // disable parent only
                           true         // abort button
                         );
            }
            else
            {
               dlg = NULL;
            }

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

               if ( dlg && !dlg->Update(n) )
               {
                  // interrupted
                  break;
               }
            }

            delete dlg;

            mf->DecRef();

            wxArrayString addresses = strutil_uniq_array(addressesSorted);
            if ( !addresses.IsEmpty() )
            {
               InteractivelyCollectAddresses(addresses,
                                             READ_APPCONFIG(MP_AUTOCOLLECT_ADB),
                                             m_fullname,
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
                  MoveMessagesToFolder(GetSelections(), folder);

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
   m_itemPreviewed = uid == UID_ILLEGAL ? -1 : m_FolderCtrl->GetFocusedItem();
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

   FolderViewAsyncStatus *status =
      new FolderViewAsyncStatus(this, _("Deleting messages..."));
   status->Monitor(m_ASMailFolder->DeleteOrTrashMessages(&selections, this),
                   _("Failed to delete messages"));
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
wxFolderView::SaveMessagesToFolder(const UIdArray& selections, MFolder *folder)
{
   size_t count = selections.GetCount();
   CHECK( count, ILLEGAL_TICKET, "no messages to save" );

   if ( !folder )
   {
      folder = MDialog_FolderChoose(GetFrame(m_Parent));
      if ( !folder )
      {
         // cancelled by user
         return ILLEGAL_TICKET;
      }
   }
   else
   {
      folder->IncRef(); // to match DecRef() below
   }

   FolderViewAsyncStatus *status =
      new FolderViewAsyncStatus(this, _("Saving %d message(s) to '%s'..."),
                                count, folder->GetFullName().c_str());

   Ticket t = m_ASMailFolder->
                  SaveMessagesToFolder(&selections, m_Frame, folder, this);

   status->Monitor(t,
                   _("Failed to save messages to the folder '%s'."),
                   folder->GetFullName().c_str());

   folder->DecRef();

   return t;
}

#if defined(EXPERIMENTAL_MARK_READ)
Ticket
wxFolderView::MarkRead(UIdArray& selections, bool read)
{
   size_t count = selections.GetCount();
   CHECK( count, ILLEGAL_TICKET, "no messages to mark" );

   FolderViewAsyncStatus *status =
      new FolderViewAsyncStatus(this, _("Marking %d message(s) ..."),
                                count, (read ? "read" : "unread"));

   Ticket t = m_ASMailFolder->
                  MarkRead(&selections, this, read);

   status->Monitor(t,
                   _("Failed to mark messages."));

      return t;
}
#endif // EXPERIMENTAL_MARK_READ

Ticket
wxFolderView::MoveMessagesToFolder(const UIdArray& messages, MFolder *folder)
{
   Ticket t = SaveMessagesToFolder(messages, folder);
   if ( t != ILLEGAL_TICKET )
   {
      // delete messages once they're successfully saved
      m_TicketsToDeleteList->Add(t);
   }

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

   FolderViewAsyncStatus *status =
      new FolderViewAsyncStatus(this, _("Saving %d message(s) to file..."),
                                count);

   Ticket t = m_ASMailFolder->SaveMessagesToFile(&selections, m_Frame, this);
   status->Monitor(t, _("Saving message(s) to file failed."));
}

bool
wxFolderView::DragAndDropMessages()
{
   CHECK( GetFolder(), false, "can't drag and drop without opened folder" );

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

// ----------------------------------------------------------------------------
// wxFolderView MEvent processing
// ----------------------------------------------------------------------------

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
   if ( event.GetFolder() != m_MailFolder )
      return;

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

   size_t n,
          count = event.GetCount();

   wxArrayLong itemsDeleted;
   itemsDeleted.Alloc(count);
   for ( n = 0; n < count; n++ )
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

      // we can't use m_FolderCtrl->GetUIdFromIndex(item) here because the item
      // is not in the headers any more, so we use indices instead of UIDs even
      // if it is less simple
      if ( !previewDeleted && (item + (long)n == m_itemPreviewed) )
      {
         previewDeleted = true;
      }

      itemsDeleted.Add(item);
   }

   // really delete the items: do it from end to avoid changing the indices
   // while we are doing it
   itemsDeleted.Sort(compareLongsReverse);

   count = itemsDeleted.GetCount();
   for ( n = 0; n < count; n++ )
   {
      m_FolderCtrl->DeleteItem(itemsDeleted[n]);
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
   if ( focus != -1 )
   {
      m_FolderCtrl->Focus(focus);
      OnFocusChange();
   }
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

   // we don't have any deleted messages any more
   m_nDeleted = 0;

   UpdateTitleAndStatusBars("", "", m_Frame, m_MailFolder);
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

         if ( status & MailFolder::MSG_STAT_DELETED )
         {
            // remember that we have another deleted message
            m_nDeleted++;
         }

         m_FolderCtrl->RefreshItem(index);

         // TODO: should also do it only once, after all status changes
         UpdateTitleAndStatusBars("", "", m_Frame, m_MailFolder);
      }
      //else: this can happen if we didn't have to update the control yet, just
      //      ignore the event then as we will get the message with the
      //      correct status when we retrieve it from Update() anyhow
   }
}

// ----------------------------------------------------------------------------
// wxFolderView async stuff
// ----------------------------------------------------------------------------

void
wxFolderView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   ASMailFolder::Result *result = event.GetResult();
   const Ticket& t = result->GetTicket();

   if ( result->GetUserData() == this )
   {
      ASSERT_MSG( m_TicketList->Contains(t), "unexpected async result ticket" );

      m_TicketList->Remove(t);

      int ok = ((ASMailFolder::ResultInt *)result)->GetValue() != 0;

      // find the corresponding FolderViewAsyncStatus object, if any
      bool hadStatusObject = false;
      size_t progressCount = m_arrayAsyncStatus.GetCount();
      for ( size_t n = 0; n < progressCount; n++ )
      {
         FolderViewAsyncStatus *asyncStatus = m_arrayAsyncStatus[n];
         if ( asyncStatus->GetTicket() == t )
         {
            if ( !ok )
            {
               // tell it to show the error message, not success one
               asyncStatus->Fail();
            }

            delete asyncStatus;
            m_arrayAsyncStatus.RemoveAt(n);

            hadStatusObject = true;

            break;
         }
      }

      String msg;
      switch ( result->GetOperation() )
      {
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

               if ( !ok )
               {
                  // something failed - what?
                  if ( toDelete )
                     msg = _("Moving messages failed.");
                  else if ( wasDropped )
                     msg = _("Dragging messages failed.");
                  else
                     msg = _("Copying messages failed.");

                  wxLogError(msg);

                  // avoid logging status message as well below
                  msg.clear();
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
                     if ( !hadStatusObject )
                     {
                        msg.Printf(_("Copied %lu messages."), count);
                     }
                     //else: status message already given
                  }
               }

               if ( !msg.empty() )
               {
                  wxLogStatus(m_Frame, msg);
               }
            }
            break;

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

         // nothing special to do for these cases
         case ASMailFolder::Op_ApplyFilterRules:
         case ASMailFolder::Op_SaveMessagesToFile:
         case ASMailFolder::Op_ReplyMessages:
         case ASMailFolder::Op_ForwardMessages:
         case ASMailFolder::Op_DeleteMessages:
         case ASMailFolder::Op_DeleteOrTrashMessages:
         case ASMailFolder::Op_UnDeleteMessages:
#if defined(EXPERIMENTAL_MARK_READ)
         case ASMailFolder::Op_MarkRead:
         case ASMailFolder::Op_MarkUnread:
#endif // EXPERIMENTAL_MARK_READ
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
            FAIL_MSG( "unexpected async operation result" );
      }
   }
   //else: not out result at all

   result->DecRef();
}

void wxFolderView::RemoveAsyncStatus(FolderViewAsyncStatus *asyncStatus)
{
   size_t progressCount = m_arrayAsyncStatus.GetCount();
   for ( size_t n = 0; n < progressCount; n++ )
   {
      if ( asyncStatus == m_arrayAsyncStatus[n] )
      {
         m_arrayAsyncStatus.RemoveAt(n);

         return;
      }
   }

   wxFAIL_MSG( "async status not found in m_arrayAsyncStatus" );
}

void wxFolderView::AddAsyncStatus(FolderViewAsyncStatus *asyncStatus)
{
   m_arrayAsyncStatus.Add(asyncStatus);
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
wxFolderViewFrame::Create(MFolder *folder, wxMFrame *parent)
{
   if ( !parent )
   {
      parent = mApplication->TopLevelFrame();
   }

   wxFolderViewFrame *
      frame = new wxFolderViewFrame(folder->GetFullName(), parent);

   wxFolderView *fv = wxFolderView::Create(frame);
   if ( !fv->OpenFolder(folder) )
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

bool OpenFolderViewFrame(MFolder *folder, wxWindow *parent)
{
   return wxFolderViewFrame::Create(folder, (wxMFrame *)GetFrame(parent))
            != NULL;
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
   return dateFormat == other.dateFormat &&
          dateGMT == other.dateGMT &&
          senderOnlyNames == other.senderOnlyNames &&
          replaceFromWithTo == other.replaceFromWithTo &&
          showSize == other.showSize &&
          memcmp(columns, other.columns, sizeof(columns)) == 0 &&
          returnAddresses == other.returnAddresses;
}

