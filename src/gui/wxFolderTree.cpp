///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxFolderTreeImpl.cpp - tree control for folders
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.10.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"

#  include <wx/menu.h>

#  include "Mdefaults.h"               // for READ_APPCONFIG
#  include "Mcclient.h"
#  include "gui/wxIconManager.h"
#  include "gui/wxMApp.h"
#  include "gui/wxMFrame.h"            // for SetIcon()
#endif // USE_PCH

#include "gui/wxMDialogs.h"

#include "gui/wxMenuDefs.h"
#include "MFolder.h"
#include "MFolderDialogs.h"
#include "FolderView.h"

#include "ColourNames.h"               // for ParseColourString()

#include <wx/tokenzr.h>
#include <wx/imaglist.h>               // for wxImageList
#include <wx/persctrl.h>

#include "gui/wxOptionsDlg.h"
#include "gui/wxFolderTree.h"

#include "MFCache.h"
#include "MFStatus.h"

#if wxUSE_DRAG_AND_DROP
   #include "Mdnd.h"
#endif // wxUSE_DRAG_AND_DROP

class MPersMsgBox;

extern "C"
{
   #include "utf8.h"  // for utf8_text_utf7()
}

// support for UTF-8 has been only added in 2.3.0
#if wxCHECK_VERSION(2, 3, 0)
   #define USE_UTF8
#else
   #undef USE_UTF8
#endif // wxWin 2.3.0

// wxMSW treectrl has a bug: it's impossible to prevent it from
// collapsing/expanding a branch when it is double clicked, even if we do
// process the activate message
#ifdef __WXMSW__
   #define USE_TREE_ACTIVATE_BUGFIX

   #include <commctrl.h>      // for NM_DBLCLK
#endif // __WXMSW__

// we can make the middle mouse work like a double click using the generic
// version and I think it is so convenient that I'm even ready to use this
// ugly hack to achieve this - unfortunately, this still doesn't work with the
// native Win32 control
#ifdef __WXGTK__
   #define USE_MIDDLE_CLICK_HACK
#endif // __WXGTK__

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_FTREE_BGCOLOUR;
extern const MOption MP_FTREE_FGCOLOUR;
extern const MOption MP_FTREE_SHOWOPENED;
extern const MOption MP_FOLDER_TREEINDEX;
extern const MOption MP_FTREE_FORMAT;
extern const MOption MP_FTREE_HOME;
extern const MOption MP_FTREE_NEVER_UNREAD;
extern const MOption MP_FTREE_PROPAGATE;
extern const MOption MP_FVIEW_FGCOLOUR;
extern const MOption MP_FVIEW_FLAGGEDCOLOUR;
extern const MOption MP_FVIEW_NEWCOLOUR;
extern const MOption MP_FVIEW_UNREADCOLOUR;
extern const MOption MP_OPEN_ON_CLICK;
extern const MOption MP_SHOW_HIDDEN_FOLDERS;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_CONFIRM_FOLDER_DELETE;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return true if we're showing all folders, even hidden ones, in the tree
static bool ShowHiddenFolders()
{
   return READ_APPCONFIG_BOOL(MP_SHOW_HIDDEN_FOLDERS);
}

// can this folder be opened?
static bool CanOpen(const MFolder *folder)
{
   return folder && CanOpenFolder(folder->GetType(), folder->GetFlags());
}

// used to sort wxArrayFolder
int CompareFoldersByTreePos(MFolder **pf1, MFolder **pf2)
{
   int pos1 = (*pf1)->GetTreeIndex(),
       pos2 = (*pf2)->GetTreeIndex();

   // -1 means to put it in the end, so anything is less than -1
   if ( pos1 == -1 )
      return pos2 == -1 ? 0 : 1;

   return pos2 == -1 ? -1 : pos1 - pos2;
}

// ----------------------------------------------------------------------------
// pseudo template classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MFolder *, wxArrayFolder);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// a tree item whose icon was changed (used internally by wxFolderTreeImpl::
// UpdateIcon() and RestoreIcon())
struct ItemWithChangedIcon
{
   ItemWithChangedIcon(const wxTreeItemId& item_, int image_)
   {
      item = item_;
      image = image_;
   }

   wxTreeItemId item;      // the tree item id
   int          image;     // the (old) icon
};

// and the array of such things
WX_DECLARE_OBJARRAY(ItemWithChangedIcon, ArrayOfItemsWithChangedIcon);
#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(ArrayOfItemsWithChangedIcon);

// tree element
class wxFolderTreeNode : public wxTreeItemData
{
public:
   // the [visible] folder status in the tree: note that the elements must be
   // ordered from least to the most important
   enum Status
   {
      Folder_Normal,       // normal state
      Folder_Flagged,      // folder has flagged messages
      Folder_Unseen,       // folder has unread messages
      Folder_New,          // folder has new messages
      Folder_StatusMax
   };

   // the ctor creates the element and inserts it in the tree
   wxFolderTreeNode(wxTreeCtrl *tree,
                    MFolder *folder,
                    wxFolderTreeNode *parent = NULL,
                    int index = -1);

   // dtor
   //
   // NB: the folder passed to the ctor will be released by this class, i.e.
   //     it takes ownership of it, the caller should not release it!
   virtual ~wxFolderTreeNode() { SafeDecRef(m_folder); }

   // accessors
      // has the tree item been already expanded?
   bool WasExpanded() const { return m_wasExpanded; }

      // get the associated folder
      //
      // NB: DecRef() shouldn't be called on the pointer returned by this
      //     function
   MFolder *GetFolder() const { return m_folder; }

      // get out parent in the tree
   wxFolderTreeNode *GetParent() const { return m_parent; }

      // get the name (base part of the label) of this folder
   String GetName() const
#ifdef USE_UTF8
      ;
#else
      { return m_folder->GetName(); }
#endif

   // expanded flag
      // must be called from OnTreeExpanding() only!
   void SetExpandedFlag() { m_wasExpanded = true; }

      // reset the "was expanded" flag (useful when the subtree must be
      // recreated)
   void ResetExpandedFlag() { m_wasExpanded = false; }


   // the folder status functions

      // set the "own" status for this folder, this doesn't always change the
      // folder appearance as GetShownStatus() might stay unchanged
   void SetStatus(wxTreeCtrl *tree, const MailFolderStatus& status);

      // translate MailFolder status to tree item status
   static Status GetTreeStatusFromMf(const MailFolderStatus& status);

protected:
      // get the current "own" status, i.e. don't take children into account
   Status GetStatus() const { return m_status; }

      // get the status induced by children: new if any child has new
      // status, unseen if any child has unseen status and normal otherwise
   Status GetChildrenStatus() const;

      // get the status shown in the tree: the most important of the folder
      // status and the status of its children
   Status GetShownStatus() const
      { return wxMax(GetStatus(), GetChildrenStatus()); }

      // called whenever the status of one of the children changes from
      // statusOld to statusNew and allows the parent to update its own status
   void OnChildStatusChange(wxTreeCtrl *tree,
                            Status statusOld, Status statusNew);

      // update the status of the folder shown on screen
   void UpdateShownStatus(wxTreeCtrl *tree, Status statusShownBefore);

   // get the label suffix, i.e. the part which is added to the label in the
   // tree to show the number of messages in the folder
   String GetLabelSuffix(const MailFolderStatus& status) const;

private:
   // not implemented
   wxFolderTreeNode(const wxFolderTreeNode&);
   wxFolderTreeNode& operator=(const wxFolderTreeNode&);

   MFolder *m_folder;            // the folder we represent

   wxFolderTreeNode *m_parent;   // the parent node (may be NULL)

   // function for changing the m_nXXX fiedls below
   void Dec(size_t& n)
   {
      wxCHECK_RET( n > 0, _T("logic error in folder status change code") );

      n--;
   }

   // status info: our current status, our real own status (i.e. corresponding
   // to the messages in this folder) and the number of children with
   // new/unseen messages we have
   Status m_status,
          m_statusOwn;
   size_t m_nNewChildren,
          m_nUnseenChildren,
          m_nFlaggedChildren;

   bool m_wasExpanded;
};

// the tree itself
class wxFolderTreeImpl : public wxPTreeCtrl, public MEventReceiver
{
public:
   // ctor
   wxFolderTreeImpl(wxFolderTree *sink,
                    wxWindow *parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size);
   // and dtor
   ~wxFolderTreeImpl();

   // accessors
   wxFolderTreeNode *GetSelection() const { return m_current; }

   // helpers
      // find the tree item by full name (as this function changes the state
      // of the tree - it may expand it - it is not `const')
   wxTreeItemId GetTreeItemFromName(const String& fullname);

      // find the next/previous folder with unread status in the tree
   wxTreeItemId FindNextUnreadFolder(wxTreeItemId id, bool next = true) const;

      // go to the next folder with unread messages after the current one
   bool GoToNextUnreadFolder(bool next = true);

      // go to home folder, give an error message if there is none
   void GoToHomeFolder();

      // go to the home folder, if any (return true in this case)
   bool GoToHomeFolderIfAny();

      // go to the given tree item: select it and ensure it is visible
   void GoToItem(wxTreeItemId id)
   {
      SelectItem(id);
      EnsureVisible(id);
   }

      // change the currently opened (in the main frame) folder name: we need
      // it to notify the main frame if this folder is deleted
   void SetOpenFolderName(const String& name);

      // used to process menu commands from the global menubar and from our
      // own popup menu
   bool ProcessMenuCommand(int id);

      // update the tree colours
   void UpdateColours();

   // tree event handlers
   // -------------------

   void OnKeyDown(wxTreeEvent& event);
   void OnChar(wxKeyEvent& event);

   void OnRightDown(wxMouseEvent& event);
#ifdef USE_MIDDLE_CLICK_HACK
   void OnMiddleDown(wxMouseEvent& event);
#endif // USE_MIDDLE_CLICK_HACK

   void OnMenuCommand(wxCommandEvent&);

   void OnTreeExpanding(wxTreeEvent&);
   void OnTreeSelect(wxTreeEvent&);
   void OnTreeActivate(wxTreeEvent& event)
      { if ( !OnDoubleClick() ) event.Skip(); }
   void OnBeginLabelEdit(wxTreeEvent&);
   void OnEndLabelEdit(wxTreeEvent&);

   void OnTreeBeginDrag(wxTreeEvent& event);
   void OnTreeEndDrag(wxTreeEvent& event);

   void OnIdle(wxIdleEvent& event);

   // event processing function
   virtual bool OnMEvent(MEventData& event);

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   void OnMouseMove(wxMouseEvent &event)
   {
#ifdef OS_WIN
      // workaround for workaround: we have to test this to avoid the frame
      // containing the list ctrl being raised to the top from behind another top
      // level frame
      HWND hwndTop = ::GetForegroundWindow();
      wxFrame *frame = GetFrame(this);//m_FolderView->m_Frame;
      if ( frame && frame->GetHWND() == (WXHWND)hwndTop )
#endif // OS_WIN
         if ( m_FocusFollowMode && FindFocus() != this )
            SetFocus();
   }
#endif

   // get the folder tree node object from item id
   wxFolderTreeNode *GetFolderTreeNode(const wxTreeItemId& item) const
   {
      return (wxFolderTreeNode *)GetItemData(item);
   }

   // (safely) get the folder from item id
   MFolder *GetFolderFromTreeItem(const wxTreeItemId& item) const
   {
      wxFolderTreeNode *node = GetFolderTreeNode(item);
      CHECK( node, NULL, _T("tree item without associated data?") );

      MFolder *folder = node->GetFolder();
      ASSERT_MSG( folder, _T("tree node without associated folder?") );

      return folder;
   }

protected:
   // return true if the current folder may be opened
   bool CanOpen() const
   {
      // is the root item chosen?
      if ( wxTreeCtrl::GetSelection() == GetRootItem() )
         return false;

      wxFolderTreeNode *node = GetSelection();

      CHECK( node, false, _T("shouldn't be called if no selection") );

      return ::CanOpen(node->GetFolder());
   }

   // return true if this folder can be renamed
   bool CanRenameFolder(const MFolder *folder) const;

   // return true if this folder can be moved
   bool CanMoveFolder(const MFolder *folder) const;

   // this is the real handler for double-click and enter events
   bool OnDoubleClick();

   // always opens the folder in a separate view
   void DoFolderOpen();

   // open folder read only in the same view
   void DoFolderView();

   void DoFolderCreate();
   void DoFolderRename();
   void DoFolderMove();
   void DoFolderDelete(bool removeOnly = true);
   void DoFolderClear();
   void DoFolderClose();
   void DoFolderUpdate();

   void DoBrowseSubfolders();

   void DoFolderProperties();
   void DoToggleHidden();

   void DoPopupMenu(const wxPoint& pos);

   // are we editing an item in place?
   bool IsEditingInPlace() const;

   // reexpand branch -- called when something in the tree changes
   void ReopenBranch(wxTreeItemId parent);

   // returns true if the given node has hidden flag set in the profile
   bool IsHidden(wxFolderTreeNode *node)
   {
      return node ? (node->GetFolder()->GetFlags() & MF_FLAGS_HIDDEN) != 0
                  : false;
   }

   // process the event which can result in changing of a tree item colour due
   // to change in new/recent messages and also update the number of messages
   // shown in the tree
   void ProcessMsgNumberChange(const wxString& folderName);

   // process the folder tree change event
   void ProcessFolderTreeChange(const MEventFolderTreeChangeData& event);

   // get the next/previouos item after/before this one
   wxTreeItemId GetNextItem(wxTreeItemId id, bool next = true) const;

   // reposition the src folder between its two siblings, the caller must call
   // ReopenBranch() to refresh the tree if wanted
   //
   // idBefore may be invalid, the other 2 are always valid
   void MoveFolderBetween(wxTreeItemId id,
                          wxTreeItemId idBefore,
                          wxTreeItemId idAfter) const;

   // MoveFolderBetween() helpers, see comments in them
   int SetIndicesBefore(wxTreeItemId id) const;
   int OffsetIndicesAfter(wxTreeItemId id) const;

private:
   class FolderMenu : public wxMenu
   {
   public:
      // menu items ids
      enum
      {
         Open,
         View,
         New,
         Remove = WXMENU_FOLDER_REMOVE,
         Delete = WXMENU_FOLDER_DELETE,
         Rename = WXMENU_FOLDER_RENAME,
         Move = WXMENU_FOLDER_MOVE,
         Close = WXMENU_FOLDER_CLOSE,
         Update = WXMENU_FOLDER_UPDATE,
         BrowseSub = WXMENU_FOLDER_BROWSESUB,
         Properties = WXMENU_FOLDER_PROP,
         ShowHidden
      };

      FolderMenu(bool isRoot)
      {
         if ( !isRoot )
         {
            Append(Open, _("&Open"));
            Append(View, _("Open read onl&y"));

            AppendSeparator();
         }

         Append(New, _("Create &new folder..."));
         if ( !isRoot )
         {
            Append(Remove, _("&Remove from tree"));
            Append(Delete, _("&Delete folder"));
         }
         Append(Rename, _("Re&name folder..."));
         if ( !isRoot )
         {
            Append(Close, _("&Close folder"));
         }
         Append(Update, _("&Update status"));

         AppendSeparator();

         Append(BrowseSub, _("Browse &subfolders..."));

         AppendSeparator();

         if ( isRoot )
         {
            Append(ShowHidden, _("Show &hidden folders"), _T(""), true);
         }

         Append(Properties, _("&Properties"));
      }

   private:
      DECLARE_NO_COPY_CLASS(FolderMenu)
   } *m_menuRoot,                 // popup menu for the root folder
     *m_menu;                     // popup menu for all folders

   wxFolderTree      *m_sink;     // for sending events
   wxFolderTreeNode  *m_current;  // current selection (NULL if none)

   // event registration handles
   void *m_eventFolderChange;    // for folder creation/destruction
   void *m_eventOptionsChange;   // options change (update icons)
   void *m_eventFolderStatus;    // number of messages changed
   void *m_eventFolderClose;     // folder has been closed

   // the full names of the folder currently opened in the main frame and
   // of the current selection in the tree ctrl (empty if none)
   String m_openFolderName,
          m_selectedFolderName;

   // icon management
      // update the icon for the selected folder, if "tmp" is true remember the
      // old value to be able to restore it later with RestoreIcon()
   void UpdateIcon(const wxTreeItemId item, bool tmp = false);

      // restore the icon previously changed by UpdateIcon(false)
   void RestoreIcon(const wxTreeItemId item);

      // find item in m_itemsWithChangedIcons and return its index in the out
      // parameter and true if found (otherwise return false)
   bool FindItemWithChangedIcon(const wxTreeItemId& item, size_t *index);

      // all the items whose icons were changed by UpdateIcon()
   ArrayOfItemsWithChangedIcon m_itemsWithChangedIcons;

   // an ugly hack to prevent sending of the "selection changed" notification
   // when the folder is selected from the program: this is needed because
   // otherwise popup menu would be never shown for a folder which can't be
   // opened (bad password...) in the "click to open mode".
   //
   // MT-note: but this class is always used only by the GUI thread, so
   //          it's not too bad
   bool     m_suppressSelectionChange;
   MFolder *m_previousFolder;

   // to avoid reexpanding the entire folder tree each time, remember whether
   // we show hidden folders at all in the tree and whether the current folder
   // is hidden
   bool     m_showHidden;
   bool     m_curIsHidden;

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   // give focus to the tree when mouse enters it [used under unix only]
   bool m_FocusFollowMode;
#endif

   // the fg and bg colour names
   wxString m_colFgName,
            m_colBgName;

   // the id of the item being edited in place
   wxTreeItemId m_idEditedInPlace;

   // the id of the item last opened in the sense that OnOpenHere() has been
   // called for it
   wxTreeItemId m_idOpenedHere;

   // the temporarily saved suffix part of the tree item label being edited
   wxString m_suffix;


   // the item currently being dragged or invalid item
   wxTreeItemId m_idDragged;

#ifdef USE_TREE_ACTIVATE_BUGFIX
   bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result);

   bool m_openedFolderOnDblClick;
#endif // USE_TREE_ACTIVATE_BUGFIX

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderTreeImpl)
};

#if wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// dnd classes
// ----------------------------------------------------------------------------

// TreeMessagesDropWhere: implementation of MMessagesDropWhere which returns
// the folder in the tree at the mouse position
class TreeMessagesDropWhere : public MMessagesDropWhere
{
public:
   TreeMessagesDropWhere(wxFolderTreeImpl *tree) { m_tree = tree; }

   virtual MFolder *GetFolder(wxCoord x, wxCoord y) const
   {
      wxTreeItemId item = m_tree->HitTest(wxPoint(x, y));
      if ( !item.IsOk() )
      {
         // no item, no folder
         return NULL;
      }

      // get the folder for this item
      wxFolderTreeNode *node = m_tree->GetFolderTreeNode(item);
      MFolder *folder = node->GetFolder();
      SafeIncRef(folder);

      return folder;
   }

   // for TreeDropTarget below
   wxWindow *GetTreeWindow() const { return m_tree; }

private:
   wxFolderTreeImpl *m_tree;
};

// TreeDropTarget: slightly enhanced version of MMessagesDropTarget which
// shows in the status bar which folder is the current drop target
class TreeDropTarget : MMessagesDropTarget
{
public:
   TreeDropTarget(MMessagesDropWhere *where, wxWindow *win)
      : MMessagesDropTarget(where, win) { m_folderLast = NULL; }

   virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
   {
      TreeMessagesDropWhere *
         dropWhere = ((TreeMessagesDropWhere *)GetDropWhere());

      MFolder *folder = dropWhere->GetFolder(x, y);

      // avoid flicker in the status bar by only calling wxLogStatus if the
      // folder under mouse changed
      if ( folder != m_folderLast )
      {
         SafeDecRef(m_folderLast);
         m_folderLast = folder;

         String msg;

         // we can't drop messages to the root folder
         if ( folder && folder->GetType() != MF_ROOT )
         {
            msg << _("Drop messages to the folder '")
                << folder->GetName() << '\'';
         }
         else // no folder under mouse
         {
            msg = _("You cannot drop messages here.");
         }

         wxLogStatus(GetFrame(), msg);
      }
      else
      {
         SafeDecRef(folder);
      }

      return def;
   }

   virtual void OnLeave()
   {
      SafeDecRef(m_folderLast);
      m_folderLast = NULL;

      MMessagesDropTarget::OnLeave();
   }

   virtual bool OnDrop(wxCoord x, wxCoord y)
   {
      SafeDecRef(m_folderLast);
      m_folderLast = NULL;

      return MMessagesDropTarget::OnDrop(x, y);
   }

   virtual ~TreeDropTarget() { SafeDecRef(m_folderLast); }

private:
   MFolder *m_folderLast;

   DECLARE_NO_COPY_CLASS(TreeDropTarget)
};

#endif // wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

// tree events
BEGIN_EVENT_TABLE(wxFolderTreeImpl, wxPTreeCtrl)
   // don't specify the control id - we shouldn't get any other tree events
   // (except our owns) anyhow
   EVT_TREE_SEL_CHANGED(-1,      wxFolderTreeImpl::OnTreeSelect)
   EVT_TREE_ITEM_EXPANDING(-1,   wxFolderTreeImpl::OnTreeExpanding)
   EVT_TREE_ITEM_ACTIVATED(-1, wxFolderTreeImpl::OnTreeActivate)
   EVT_TREE_BEGIN_LABEL_EDIT(-1, wxFolderTreeImpl::OnBeginLabelEdit)
   EVT_TREE_END_LABEL_EDIT(-1,   wxFolderTreeImpl::OnEndLabelEdit)

   EVT_TREE_BEGIN_DRAG(-1, wxFolderTreeImpl::OnTreeBeginDrag)
   EVT_TREE_END_DRAG(-1, wxFolderTreeImpl::OnTreeEndDrag)

   EVT_RIGHT_DOWN(wxFolderTreeImpl::OnRightDown)
#ifdef USE_MIDDLE_CLICK_HACK
   EVT_MIDDLE_DOWN(wxFolderTreeImpl::OnMiddleDown)
#endif // USE_MIDDLE_CLICK_HACK

   EVT_TREE_KEY_DOWN(-1, wxFolderTreeImpl::OnKeyDown)
   EVT_CHAR(wxFolderTreeImpl::OnChar)

   EVT_MENU(-1, wxFolderTreeImpl::OnMenuCommand)

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   EVT_MOTION (wxFolderTreeImpl::OnMouseMove)
#endif

   EVT_IDLE(wxFolderTreeImpl::OnIdle)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFolderTree
// ----------------------------------------------------------------------------

void
wxFolderTree::Init(wxWindow *parent, wxWindowID id,
                   const wxPoint& pos, const wxSize& size)
{
   m_tree = new wxFolderTreeImpl(this, parent, id, pos, size);
}

wxFolderTree::~wxFolderTree()
{
   delete m_tree;
}

bool wxFolderTree::SelectFolder(MFolder *folder)
{
   wxTreeItemId item = m_tree->GetTreeItemFromName(folder->GetFullName());
   if ( item.IsOk() )
   {
      // the item which is returned should correspond to the folder we started
      // with, otherwise there is something wrong
      ASSERT_MSG(
                  m_tree->GetFolderTreeNode(item)->GetFolder() == folder,
                  _T("GetTreeItemFromName() is buggy")
                );

      // select the item and also scroll to it
      m_tree->GoToItem(item);

      return true;
   }
   else
   {
      return false;
   }
}

MFolder *wxFolderTree::FindNextUnreadFolder(bool next)
{
   CHECK( m_tree, NULL, _T("you didn't call Init()") );

   wxFolderTreeNode *node = m_tree->GetSelection();
   if ( !node )
      return NULL;

   wxTreeItemId id = m_tree->FindNextUnreadFolder(node->GetId(), next);
   if ( !id.IsOk() )
      return NULL;

   MFolder *folder = m_tree->GetFolderTreeNode(id)->GetFolder();
   folder->IncRef();
   return folder;
}

void wxFolderTree::ProcessMenuCommand(int id)
{
   CHECK_RET( m_tree, _T("you didn't call Init()") );

   m_tree->ProcessMenuCommand(id);
}

// this function is used to update the state (enabled/disabled) of menu items
// both for the popup menu we use ourselves and for the "Folder" menu in the
// main frame menu bar, so the menu passed here may have more items than our
// popup menu - handle it properly by disabling all items which don't make sense
// for the current selection!
void wxFolderTree::UpdateMenu(wxMenu *menu, const MFolder *folder)
{
   CHECK_RET( menu, _T("NULL menu in wxFolderTree::UpdateMenu") );

   int folderFlags = folder->GetFlags();
   MFolderType folderType = folder->GetType();

   bool isRoot = folderType == MF_ROOT;

   // root is a group, too
   bool isGroup = isRoot || (folderType == MF_GROUP);

   // TODO we should allow "renaming" the root folder, i.e. changing the
   //      default 'All folders' label for it, but for now we don't
   menu->Enable(WXMENU_FOLDER_RENAME, !isRoot && folderType != MF_INBOX);

   if ( menu->FindItem(WXMENU_FOLDER_FILTERS) )
   {
      // can't associate filters to a group folder - there are never any
      // messages in it anyhow
      menu->Enable(WXMENU_FOLDER_FILTERS, !isGroup);
   }

   if ( menu->FindItem(WXMENU_FOLDER_REMOVE) )
   {
      // root folder can't be removed
      menu->Enable(WXMENU_FOLDER_REMOVE, !isRoot);

      // NB: if Remove is there, Delete and Close are too and vice versa, so we
      //     don't call FindItem() to check for this, but we should if it ever
      //     changes

      menu->Enable(WXMENU_FOLDER_DELETE, CanDeleteFolderOfType(folderType));

      // TODO should check that it is really opened
      menu->Enable(WXMENU_FOLDER_CLOSE, !isGroup);
   }

   if ( menu->FindItem(WXMENU_FOLDER_CLEAR) )
   {
      menu->Enable(WXMENU_FOLDER_CLEAR, !isGroup);
   }

   // the only type of folder for which updating status doesn't make sense is a
   // group without children
   if ( menu->FindItem(WXMENU_FOLDER_UPDATE) )
   {
      menu->Enable(WXMENU_FOLDER_UPDATE,
                     !isGroup || folder->GetSubfolderCount());
   }

   // browsing subfolders only makes sense if we have any and not for the
   // simple groups which can contain anything - so browsing is impossible
   bool mayHaveSubfolders =
      !isGroup && CanHaveSubfolders(folderType, folderFlags);

   menu->Enable(WXMENU_FOLDER_BROWSESUB, mayHaveSubfolders);
}

MFolder *wxFolderTree::GetSelection() const
{
   CHECK( m_tree, NULL, _T("you didn't call Init()") );

   // get it from the tree
   wxFolderTreeNode *node = m_tree->GetSelection();
   if ( node == NULL )
      return NULL;

   MFolder *folder = node->GetFolder();
   folder->IncRef();

   return folder;
}

wxWindow *wxFolderTree::GetWindow() const
{
   ASSERT_MSG( m_tree, _T("you didn't call Init()") );

   return m_tree;
}

// open this folder in the current frame's folder view
void wxFolderTree::OnSelectionChange(MFolder * /* oldsel */, MFolder *newsel)
{
   // MP_OPEN_ON_CLICK profile setting tells us if we should open the folder
   // in the main frame when it's just clicked - otherwise it must be double
   // clicked to be opened. This option might be set for the situations when
   // opening the folder takes too much time.
   if ( READ_APPCONFIG(MP_OPEN_ON_CLICK) )
   {
      // don't even try to open the folders which can't be opened
      if ( CanOpen(newsel) )
      {
         newsel->IncRef(); // before returning it to the outside world
         OnOpenHere(newsel);
      }
      else
      {
         // remove the folder view
         OnOpenHere(NULL);
      }
   }
}

// open a folder view in the same frame - must be overriden to do something
void wxFolderTree::OnOpenHere(MFolder *folder)
{
   if ( folder )
   {
      m_tree->SetOpenFolderName(folder->GetFullName());

      folder->DecRef();
   }
   else
   {
      m_tree->SetOpenFolderName(_T(""));
   }
}

// view folder (always in the same view for now)
void wxFolderTree::OnView(MFolder *folder)
{
   // just update the folder name
   wxFolderTree::OnOpenHere(folder);
}

// open a new folder view on this folder
void wxFolderTree::OnOpen(MFolder *folder)
{
   if ( CanOpen(folder) )
   {
      OpenFolderViewFrame(folder, GetFrame(m_tree));
   }
   else
   {
      FAIL_MSG(_T("OnOpen() called for folder which can't be opened"));
   }

   folder->DecRef();
}

// browse subfolders of this folder
void wxFolderTree::OnBrowseSubfolders(MFolder *folder)
{
   if ( folder->GetType() == MF_ROOT )
   {
      RunImportFoldersWizard();
   }
   else
   {
      (void)ShowFolderSubfoldersDialog(folder, m_tree);
   }

   folder->DecRef();
}

// bring up the properties dialog for this profile
void wxFolderTree::OnProperties(MFolder *folder)
{
   if ( folder->GetType() == MF_ROOT )
   {
      // show the program preferences dialog for the root folder
      ShowOptionsDialog(GetFrame(m_tree));
   }
   else // normal folder, show properties dialog for it
   {
      (void)ShowFolderPropertiesDialog(folder, m_tree);
   }

   folder->DecRef();
}

MFolder *wxFolderTree::OnCreate(MFolder *parent)
{
   wxWindow *winTop = ((wxMApp *)mApplication)->GetTopWindow();

   bool wantsDialog;
   MFolder *newfolder = RunCreateFolderWizard(&wantsDialog, parent, winTop);
   if ( wantsDialog )
   {
      // users wants to use the dialog directly instead of the wizard
      newfolder = ShowFolderCreateDialog(winTop, FolderCreatePage_Default, parent);
   }

   if ( parent )
      parent->DecRef();

   return newfolder;
}

bool wxFolderTree::OnDelete(MFolder *folder, bool removeOnly)
{
   CHECK( folder, false, _T("can't delete NULL folder") );

   if ( folder->GetFlags() & MF_FLAGS_DONTDELETE )
   {
      wxLogError(_("The folder '%s' is used by Mahogany and cannot be deleted"),
                 folder->GetFullName().c_str());
      return false;
   }

   if ( folder->GetType() == MF_ROOT )
   {
      wxLogError(_("The root folder can not be deleted."));
      return false;
   }

   // by default, don't allow to suppress this question as deleting a whole
   // subtree of folders is something which should require confirmation
   const MPersMsgBox *msgbox = NULL;
   wxString msg;
   if ( folder->GetSubfolderCount() > 0 )
   {
      if ( removeOnly )
      {
         msg.Printf(_("Do you really want to remove folder '%s' and all of its\n"
                      "subfolders? You will permanently lose all the settings\n"
                      "for the removed folders!"), folder->GetName().c_str());
      }
      else // remove and delete
      {
         msg.Printf(_("Do you really want to delete folder '%s' and all of its\n"
                      "subfolders? All messages contained in them will be "
                      "permanently lost!"), folder->GetFullName().c_str());
      }
   }
   else
   {
      if ( removeOnly )
      {
         // in this case the question can be suppressed as you don't lose much
         // by inadvertently answering "Yes" to it
         msgbox = M_MSGBOX_CONFIRM_FOLDER_DELETE;

         msg.Printf(_("Do you really want to remove folder '%s'?"),
                    folder->GetFullName().c_str());
      }
      else // remove and delete
      {
         msg.Printf(_("Do you really want to delete folder '%s' with\n"
                      "all the messages contained in it?"),
                    folder->GetFullName().c_str());
      }
   }

   bool ok = MDialog_YesNoDialog(msg,
                                 m_tree->wxWindow::GetParent(),
                                 MDIALOG_YESNOTITLE,
                                 M_DLG_NO_DEFAULT,
                                 msgbox);
   if ( ok )
   {
      // close the folder first
      ok = OnClose(folder);
      if ( !ok )
      {
         wxLogError(_("The folder must be closed before it can be deleted."));
      }

      if ( ok && !removeOnly )
      {
         ok = MailFolder::DeleteFolder(folder);
         if ( !ok )
         {
            wxLogError(_("Failed to physically delete folder '%s'."),
                       folder->GetFullName().c_str());
         }
      }

      if ( ok )
      {
         // delete it from the folder tree
         folder->Delete();
      }
   }

   return ok;
}

bool wxFolderTree::OnRename(MFolder *folder,
                            const String& folderNewName,
                            const String& mboxNewName)
{
   CHECK( folder, false, _T("can't rename NULL folder") );

   bool rc = true;
   if ( !mboxNewName.empty() )
   {
      rc = MailFolder::Rename(folder, mboxNewName);
      if ( rc )
      {
         folder->SetPath(mboxNewName);
      }
   }

   if ( rc && !folderNewName.empty() )
   {
      rc &= folder->Rename(folderNewName);
   }

   if ( rc )
   {
      wxLogStatus(GetFrame(m_tree->wxWindow::GetParent()),
                  _("Successfully renamed folder '%s'."),
                  folder->GetFullName().c_str());
   }
   else
   {
      wxLogError(_("Failed to rename the folder or mailbox."));
   }

   return rc;
}

bool wxFolderTree::OnMove(MFolder *folder,
                          MFolder *newParent)
{
   CHECK( folder, false, _T("can't move NULL folder") );
   CHECK( newParent, false, _T("can't move folder to NULL parent") );

   bool rc = true;

   String fullPath = folder->GetFullName();
   CHECK( !fullPath.empty(), false, _T("can't move the root pseudo-folder") );

   String oldPath = fullPath.BeforeLast('/'),
          name    = fullPath.AfterLast('/');

   String newPath = newParent->GetFullName();

   // close the folder first
   rc = OnClose(folder);
   if ( !rc )
   {
      wxLogError(_("The folder must be closed before it can be moved."));
      return false;
   }

   rc = folder->Move(newParent);
   if ( rc )
   {
      wxLogStatus(GetFrame(m_tree->wxWindow::GetParent()),
                  _("Successfully moved folder '%s' to '%s'."),
                  folder->GetFullName().c_str(), newParent->GetFullName().c_str());
   }
   else
   {
      wxLogError(_("Failed to move the folder '%s' from '%s' to '%s'."),
                 name.c_str(), oldPath.c_str(), newPath.c_str());
   }
   return false;
}

void wxFolderTree::OnClear(MFolder *folder)
{
   wxString fullname = folder->GetFullName();

   wxString msg;
   msg.Printf(_("Are you sure you want to delete all messages from the\n"
                "folder '%s'?\n"
                "\n"
                "Warning: it will be impossible to undelete them!"),
                fullname.c_str());

   wxWindow *parent = m_tree->wxWindow::GetParent();

   // this dialog is not disablable!
   if ( !MDialog_YesNoDialog
         (
            msg,
            parent,
            _("Please confirm"),
            M_DLG_NO_DEFAULT
         ) )
   {
      wxLogStatus(GetFrame(parent), _("No messages were deleted."));
   }
   else
   {
      long n = MailFolder::ClearFolder(folder);
      if ( n < 0 )
      {
         wxLogError(_("Failed to delete messages from folder '%s'."),
                    fullname.c_str());
      }
      else
      {
         wxLogStatus(GetFrame(parent),
                     _("%lu messages were deleted from folder '%s'."),
                     (unsigned long)n, fullname.c_str());
      }
   }
}

void wxFolderTree::OnUpdate(MFolder *folder)
{
   // for the folders which can't be opened but have subfolders we should
   // update their subfolders
   if ( folder->CanOpen() || !folder->GetSubfolderCount() )
   {
      if ( !MailFolder::CheckFolder(folder) )
      {
         wxLogError(_("Failed to update the status of the folder '%s'."),
                    folder->GetFullName().c_str());
      }
      else
      {
         wxLogStatus(GetFrame(m_tree->wxWindow::GetParent()),
                     _("Updated status of the folder '%s'"),
                     folder->GetFullName().c_str());
      }
   }
   else // update subfolders
   {
      // this is from wxMainFrame.cpp (TODO: move somewhere else)
      extern int UpdateFoldersSubtree(const MFolder& folder, wxWindow *parent);

      int nUpdated = UpdateFoldersSubtree(*folder, m_tree);
      if ( nUpdated < 0 )
      {
         wxLogError(_("Failed to update the status"));
      }
      else
      {
         wxLogStatus(GetFrame(m_tree->wxWindow::GetParent()),
                     _("Updated status of %d folders."),
                     nUpdated);
      }
   }
}

bool wxFolderTree::OnClose(MFolder *folder)
{
   // if the user chose to manually close the folder, we close the network
   // connection as well ("don't linger")
   (void)MailFolder::CloseFolder(folder, false /* don't linger */);

   wxLogStatus(GetFrame(m_tree->wxWindow::GetParent()),
               _("Folder '%s' closed."),
               folder->GetFullName().c_str());

   m_tree->SetOpenFolderName(_T(""));

   return true;
}

bool wxFolderTree::OnDoubleClick()
{
   MFolder *sel = GetSelection();
   CHECK( sel, false, _T("no folder to open") );

   if ( !CanOpen(sel) )
   {
      wxLogStatus(GetFrame(m_tree), _("Cannot open this folder."));

      sel->DecRef();

      return false;
   }

   if ( READ_APPCONFIG(MP_OPEN_ON_CLICK) )
   {
      // then double click opens in a separate view
      OnOpen(sel);
   }
   else
   {
      // double click is needed to open it in this view (== "here")
      OnOpenHere(sel);
   }

   return true;
}

// ----------------------------------------------------------------------------
// wxFolderTreeNode ctor
// ----------------------------------------------------------------------------

wxFolderTreeNode::wxFolderTreeNode(wxTreeCtrl *tree,
                                   MFolder *folder,
                                   wxFolderTreeNode *parent,
                                   int index)
{
   // init member vars
   m_parent = parent;
   m_folder = folder;
   m_wasExpanded = false;

   m_status =
   m_statusOwn = Folder_Normal;

   m_nNewChildren =
   m_nUnseenChildren =
   m_nFlaggedChildren = 0;

   String fullname = folder->GetFullName();

   MailFolderStatus status;
   bool hasStatus = MfStatusCache::Get()->GetStatus(fullname, &status);

   int image = GetFolderIconForDisplay(folder);

   // add this item to the tree
   int id;
   if ( folder->GetType() == MF_ROOT )
   {
      id = tree->AddRoot(_("All folders"), image, image, this);
   }
   else // not root
   {
      String label = GetName();
      if ( hasStatus )
      {
         // show the number of messages in the tree
         label += GetLabelSuffix(status);
      }
      //else: no status, don't show anything

      wxTreeItemId idParent = GetParent()->GetId();
      if ( index == -1 )
         id = tree->AppendItem(idParent, label, image, image, this);
      else
         id = tree->InsertItem(idParent, index, label, image, image, this);
   }

   SetId(id);

   // allow the user to expand us if we have any children
   if ( folder )
   {
      tree->SetItemHasChildren(GetId(), folder->GetSubfolderCount() != 0);
   }

   // restore cached status
   if ( hasStatus )
   {
      SetStatus(tree, status);
   }
}

// ----------------------------------------------------------------------------
// wxFolderTreeNode status code
// ----------------------------------------------------------------------------

String wxFolderTreeNode::GetLabelSuffix(const MailFolderStatus& mfStatus) const
{
   Profile_obj profile(GetFolder()->GetProfile());

   String fmt = READ_CONFIG(profile, MP_FTREE_FORMAT);
   if ( fmt.empty() )
   {
      // don't bother calling FormatFolderStatusString
      return fmt;
   }

   return FormatFolderStatusString
          (
            fmt,
            GetFolder()->GetFullName(),     // name
            (MailFolderStatus *)&mfStatus,  // const_cast is harmless
            NULL                            // don't use mail folder
          );
}

/* static */
wxFolderTreeNode::Status
wxFolderTreeNode::GetTreeStatusFromMf(const MailFolderStatus& mfStatus)
{
   if ( mfStatus.newmsgs > 0 )
   {
      return wxFolderTreeNode::Folder_New;
   }
   else if ( mfStatus.unread > 0 )
   {
      return wxFolderTreeNode::Folder_Unseen;
   }
   else if ( mfStatus.flagged > 0 )
   {
      return wxFolderTreeNode::Folder_Flagged;
   }
   else // no unseen, no new, no flagged
   {
      return wxFolderTreeNode::Folder_Normal;
   }
}

wxFolderTreeNode::Status wxFolderTreeNode::GetChildrenStatus() const
{
   if ( m_nNewChildren )
      return Folder_New;

   if ( m_nUnseenChildren )
      return Folder_Unseen;

   if ( m_nFlaggedChildren )
      return Folder_Flagged;

   return Folder_Normal;
}

void wxFolderTreeNode::OnChildStatusChange(wxTreeCtrl *tree,
                                           Status statusOld,
                                           Status statusNew)
{
   if ( !GetParent() )
   {
      // never change the status of the root folder

      // but update the main frame icon depending on whether we have any new
      // messages or not
      if ( statusOld == Folder_New ||
               statusNew == Folder_New )
      {
         wxMFrame *frame = mApplication->TopLevelFrame();
         if ( frame )
         {
            wxIcon icon(ICON(statusNew == Folder_New ? _T("MainFrameNewMail")
                                                     : _T("MainFrame")));
            frame->SetIcon(icon);
         }
         //else: we can get this during startup, before the frame exists yet
      }

      return;
   }

   Status statusShownBefore = GetShownStatus();

   // first account for the fact that the child lost its special status
   switch ( statusOld )
   {
      case Folder_New:
         Dec(m_nNewChildren);
         break;

      case Folder_Unseen:
         Dec(m_nUnseenChildren);
         break;

      case Folder_Flagged:
         Dec(m_nFlaggedChildren);
         break;

      default:
         FAIL_MSG(_T("unexpected folder status in OnChildStatusChange"));

      case Folder_Normal:
         // nothing to do
         ;
   }

   // then remember that one of our children got some special status
   switch ( statusNew )
   {
      case Folder_New:
         m_nNewChildren++;
         break;

      case Folder_Unseen:
         m_nUnseenChildren++;
         break;

      case Folder_Flagged:
         m_nFlaggedChildren++;
         break;

      default:
         FAIL_MSG(_T("unexpected folder status in OnChildStatusChange"));

      case Folder_Normal:
         // nothing to do
         ;
   }

   // update our own status
   UpdateShownStatus(tree, statusShownBefore);
}

void wxFolderTreeNode::SetStatus(wxTreeCtrl *tree,
                                 const MailFolderStatus& mfStatus)
{
   Status status = GetTreeStatusFromMf(mfStatus);

   if ( status != m_status )
   {
      Status statusShownBefore = GetShownStatus();

      m_status = status;

      UpdateShownStatus(tree, statusShownBefore);
   }
   //else: status (i.e. colour) didn't change

   // change label if we show the number of messages in it
   String suffix = GetLabelSuffix(mfStatus);
   if ( !suffix.empty() )
   {
      wxString textOld = tree->GetItemText(GetId()),
               textNew = GetName() + suffix;

      // avoid flicker: don't update the item text unless it really changed
      if ( textNew != textOld )
      {
         tree->SetItemText(GetId(), textNew);
      }
   }
}

void wxFolderTreeNode::UpdateShownStatus(wxTreeCtrl *tree,
                                         Status statusShownBefore)
{
   // do we need to update the item on screen?
   Status statusShown = GetShownStatus();
   if ( statusShown != statusShownBefore )
   {
      Profile_obj profile(GetFolder()->GetProfile());

      const MOption *opt;
      switch ( statusShown )
      {
         default:
            FAIL_MSG( _T("unexpected folder status value") );
            // fall through

         case Folder_Normal:
            opt = &MP_FVIEW_FGCOLOUR;
            break;

         case Folder_Flagged:
            opt = &MP_FVIEW_FLAGGEDCOLOUR;
            break;

         case Folder_Unseen:
            opt = &MP_FVIEW_UNREADCOLOUR;
            break;

         case Folder_New:
            opt = &MP_FVIEW_NEWCOLOUR;
            break;

      }

      Profile::ReadResult found;
      wxString colorName = profile->readEntry(GetOptionName(*opt),
                                              GetStringDefault(*opt),
                                              &found);

      // use the default tree foreground colour for the folders not explicitly
      // configured to use another colour
      wxColour col;
      if ( found || (statusShown != Folder_Normal) )
      {
         if ( !ParseColourString(colorName, &col) )
         {
            wxLogDebug(_T("Invalid colour string '%s'."), colorName.c_str());
            col = *wxBLACK;
         }
      }
      else // use default foreground
      {
         // NB: we still need to set the colour even if it is now going to be
         //     the default one simply because it could have been a different
         //     one before
         col = tree->GetForegroundColour();
      }

      tree->SetItemTextColour(GetId(), col);

      // propagate the change to the parent: when the child status changes,
      // the parent status may change as well (if we just got new messages,
      // parent should show them, for example)

      // however this may be disabled for some particular folders (the reason
      // for having this option is that I really don't want to have my IMAP
      // server highlighted as new if there are new messages in the trash
      // folder only)
      if ( READ_CONFIG(profile, MP_FTREE_PROPAGATE) )
      {
         if ( GetParent() )
            GetParent()->OnChildStatusChange(tree, statusShownBefore, statusShown);
      }
   }
}

// ----------------------------------------------------------------------------
// wxFolderTreeNode UTF8 support
// ----------------------------------------------------------------------------

#ifdef USE_UTF8

// Folder names (IMAP, but can be used for local mailboxes also) may contain
// characters encoded in "modified-UTF7" (RFC 2060)

wxString wxFolderTreeNode::GetName() const
{
   const wxString nameOrig = m_folder->GetName();
   wxString name;

   bool isValid = true;

   const size_t len = nameOrig.Length();

   name.reserve(len);
   for ( size_t i = 0; i < len; i++ )
   {
      // make sure valid name
      wxChar s = nameOrig[i];
      if (s & 0x80)
      {
         // reserved for future use with UTF-8
         wxLogDebug(_T("mailbox name with 8-bit character"));
         isValid = false;
         break;
      }

      // validate IMAP modified UTF-7
      if (s == _T('&'))
      {
         String nameutf7 = _T('+');
         size_t j = i;
         while ( ((s = nameOrig[++j]) != '-') && isValid )
         {
            switch (s)
            {
               case '\0':
                  wxLogDebug(_T("unterminated modified UTF-7 name"));
                  isValid = false;
                  break;

#if 0
               case '+':   /* valid modified BASE64 */
                  // but we don't support it yet  FIXME
                  isValid = false;
                  break;
               case ',':  /* all OK so far */
                  // but we don't support it yet; should be changed to "/"  FIXME
                  isValid = false;
                  break;
#endif // 0

               default:    /* must be alphanumeric */
                  if (!wxIsalnum (s))
                  {
                     wxLogDebug(_T("invalid modified UTF-7 name"));
                     isValid = false;
                     break;
                  }
                  else
                  {
                     nameutf7 << s;
                  }
            }
         }

         if ( !isValid )
            break;

         // valid IMAP modified UTF-7 mailbox name, converting to
         // environment's default encoding for now (FIXME)

         //Convert UTF-7 to UTF-8. Instead of this we could just use
         //wxString(nameutf7.wc_str(wxConvUTF7), wxConvLocal);
         //but wxWindows does not support UTF-7 yet, so we first convert
         //UTF-7 to UTF-8 using c-client function and then convert
         //UTF-8 to current environment's encoding.

         nameutf7 << _T("-");

         SIZEDTEXT *text7 = new SIZEDTEXT;
         SIZEDTEXT *text8 = new SIZEDTEXT;
         text7->data = (unsigned char *) nameutf7.c_str();
         text7->size = nameutf7.Length();

         utf8_text_utf7 (text7, text8);

         //we cannot use "nameutf8 << text8->data" here as utf8_text_utf7()
         //returns text8->data which is longer than text8->size:
         String nameutf8;
         nameutf8.reserve(text8->size);
         for ( unsigned long k = 0; k < text8->size; k++ )
         {
            nameutf8 << wxChar(text8->data[k]);
         }
         // convert nameutf8 from UTF-8 to current environment's encoding:
         nameutf8 = wxString(nameutf8.wc_str(wxConvUTF8), wxConvLocal);
         name << nameutf8;
         i = j;
         free(text7);
         free(text8);
      }
      else // s != '&'
      {
         name << s;
      }
   }

   return isValid ? name : nameOrig;
}

#endif // USE_UTF8

// ----------------------------------------------------------------------------
// wxFolderTreeImpl
// ----------------------------------------------------------------------------

wxFolderTreeImpl::wxFolderTreeImpl(wxFolderTree *sink,
                                   wxWindow *parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size)
                : wxPTreeCtrl(_T("FolderTree"), parent, id, pos, size,
                              wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS)
{
   // init member vars
   m_current = NULL;
   m_sink = sink;
   m_menu =
   m_menuRoot = NULL;
   m_suppressSelectionChange = false;
   m_previousFolder = NULL;

   m_showHidden = ShowHiddenFolders();
   m_curIsHidden = false;

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   m_FocusFollowMode = READ_APPCONFIG_BOOL(MP_FOCUS_FOLLOWSMOUSE);
#endif

   // create an image list and associate it with this control
   size_t nIcons = GetNumberOfFolderIcons();

   wxImageList *imageList = NULL;

   wxIconManager *iconManager = mApplication->GetIconManager();
   for ( size_t n = 0; n < nIcons; n++ )
   {
      wxBitmap bmp = iconManager->GetBitmap(GetFolderIconName(n));

      // create thei mage list once we know the size of the images in it
      if ( !imageList )
      {
         imageList = new wxImageList(bmp.GetWidth(), bmp.GetHeight(),
                                     false, nIcons);
      }

      imageList->Add(bmp);
   }

   SetImageList(imageList);

   UpdateColours();

#if wxUSE_DRAG_AND_DROP
   // create our drop target
   new TreeDropTarget(new TreeMessagesDropWhere(this), this);
#endif // wxUSE_DRAG_AND_DROP

   // create the root item
   MFolder *folderRoot = MFolder::Get(_T(""));
   m_current = new wxFolderTreeNode(this, folderRoot);

   // register with the event manager
   if ( !MEventManager::RegisterAll
         (
            this,
            MEventId_FolderTreeChange, &m_eventFolderChange,
            MEventId_OptionsChange, &m_eventOptionsChange,
            MEventId_FolderStatus, &m_eventFolderStatus,
            MEventId_FolderClosed, &m_eventFolderClose,
            MEventId_Null
         ) )
    {
        FAIL_MSG( _T("Failed to register folder tree with event manager") );
    }

   // preselect the home folder: this is probably better then returning the
   // selection to the last selected item although we could still do this if
   // there is no home folder...
   GoToHomeFolderIfAny();
}

void wxFolderTreeImpl::UpdateColours()
{
   wxString colName = READ_APPCONFIG(MP_FTREE_BGCOLOUR);
   if ( colName != m_colBgName )
   {
      if ( !colName.empty() )
      {
         wxColour col;
         if ( ParseColourString(colName, &col) )
         {
            SetBackgroundColour(col);
         }
      }

      m_colBgName = colName;
   }

   colName = READ_APPCONFIG_TEXT(MP_FTREE_FGCOLOUR);
   if ( colName != m_colFgName )
   {
      if ( !colName.empty() )
      {
         wxColour col;
         if ( ParseColourString(colName, &col) )
         {
            SetForegroundColour(col);
         }
      }

      m_colFgName = colName;
   }
}

void wxFolderTreeImpl::SetOpenFolderName(const String& name)
{
   m_openFolderName = name;

   // in any case, the previous opened folder is not opened here any more, stop
   // showing it in bold
   if ( m_idOpenedHere.IsOk() )
   {
      SetItemBold(m_idOpenedHere, false);
      m_idOpenedHere.Unset();
   }

   // do have an opened folder?
   if ( !m_openFolderName.empty() )
   {
      // yes, visually emphasize it
      m_idOpenedHere = wxTreeCtrl::GetSelection();
      if ( READ_APPCONFIG(MP_FTREE_SHOWOPENED) )
      {
         SetItemBold(m_idOpenedHere, true);
      }
   }
}

void wxFolderTreeImpl::DoPopupMenu(const wxPoint& pos)
{
   wxFolderTreeNode *cur = GetSelection();
   if ( cur != NULL )
   {
      MFolder *folder = cur->GetFolder();

      // what kind of folder do we have?
      wxString title(folder->GetName());
      if ( !title )
      {
         title = _("All folders");
      }

      MFolderType folderType = folder->GetType();
      bool isRoot = folderType == MF_ROOT;

      FolderMenu **menu = isRoot ? &m_menuRoot : &m_menu;

      // create our popup menu if not done yet
      if ( !*menu )
      {
         *menu = new FolderMenu(isRoot);
      }

      (*menu)->SetTitle(wxString::Format(_("Folder '%s'"), title.c_str()));

      // some items (all WXMENU_FOLDEX_XXX ones) are taken care of there already
      m_sink->UpdateMenu(*menu, folder);

      if ( isRoot )
      {
         // init the menu
         (*menu)->Check(FolderMenu::ShowHidden, ShowHiddenFolders());
      }
      else
      {
         int folderFlags = folder->GetFlags();

         // disable the items which don't make sense for some kinds of folders:
         // groups can't be opened
         bool canOpen = CanOpenFolder(folderType, folderFlags);
         (*menu)->Enable(FolderMenu::Open, canOpen);
         (*menu)->Enable(FolderMenu::View, canOpen);

         // these items only make sense when a folder can, in principle, have
         // inferiors (and browsing doesn't make sense for "simple" groups - what
         // would we browse for?)
         bool mayHaveSubfolders = CanHaveSubfolders(folderType, folderFlags);
         (*menu)->Enable(FolderMenu::New, mayHaveSubfolders);
      }

      PopupMenu(*menu, pos.x, pos.y);
   }
   //else: no selection
}

// this functions accepts a slash delimited (but without leading slash) full
// path to the folder in the folder tree and returns its tree item id
wxTreeItemId
wxFolderTreeImpl::GetTreeItemFromName(const String& fullname)
{
   // tokenize the fullname in components treating subsequent slashes as one
   // delimiter
   wxTreeItemId current = GetRootItem();

   // empty name means root
   if ( !fullname )
      return current;

   wxStringTokenizer tk(fullname, _T("/"), wxTOKEN_STRTOK);

   // we do breadth-first search in the tree
   while ( tk.HasMoreTokens() )
   {
      // find the child with the given name
      wxString name = tk.GetNextToken();

      ASSERT_MSG( !!name, _T("token can't be empty here") );

      if ( ItemHasChildren(current) && !IsExpanded(current) )
      {
         // do expand it or we wouldn't find any children under it
         Expand(current);
      }

#if wxCHECK_VERSION(2, 5, 0)
      wxTreeItemIdValue cookie;
#else
      long cookie;
#endif
      wxTreeItemId child = GetFirstChild(current, cookie);
      while ( child.IsOk() )
      {
         // we can't use GetItemText() any more here because the tree item
         // label can now have user-configured suffix showing the number of
         // messages
         if ( GetFolderTreeNode(child)->GetName() == name )
            break;

         child = GetNextChild(current, cookie);
      }

      current = child;

      if ( !current.IsOk() )
      {
         // didn't find the folder
         break;
      }
   }

   return current;
}

void wxFolderTreeImpl::DoFolderOpen()
{
   m_sink->OnOpen(m_sink->GetSelection());
}

void wxFolderTreeImpl::DoFolderView()
{
   m_sink->OnView(m_sink->GetSelection());
}

void wxFolderTreeImpl::DoFolderCreate()
{
   MFolder *folderNew = m_sink->OnCreate(m_sink->GetSelection());
   if ( folderNew != NULL )
   {
      // now done in OnMEvent()
#if 0
     wxTreeItemId idCurrent = wxTreeCtrl::GetSelection();
     wxFolderTreeNode *parent = GetFolderTreeNode(idCurrent);

     wxASSERT_MSG( parent, _T("can't get the parent of current tree item") );

     (void)new wxFolderTreeNode(this, folderNew, parent);
#endif // 0

     folderNew->DecRef();
   }
   //else: cancelled by user
}

bool wxFolderTreeImpl::CanRenameFolder(const MFolder *folder) const
{
   if ( folder->GetType() == MF_INBOX )
   {
      wxLogError(_("INBOX folder is a special folder used by the mail "
                   "system and can not be renamed."));
   }
   else if ( folder->GetType() == MF_ROOT )
   {
      wxLogError(_("The root folder can not be renamed."));
   }
   else if ( folder->GetFlags() & MF_FLAGS_DONTDELETE )
   {
      wxLogError(_("The folder '%s' is used by Mahogany and cannot be renamed."),
                 folder->GetName().c_str());
   }
   else
   {
      // passed all tests
      return true;
   }

   return false;
}

void wxFolderTreeImpl::DoFolderRename()
{
   MFolder *folder = m_sink->GetSelection();
   if ( !folder )
   {
      wxLogError(_("Please select the folder to rename first."));
   }
   else // have a folder
   {
      if ( CanRenameFolder(folder) )
      {
         wxFrame *frame = GetFrame(this);

         String folderName, mboxName;
         if ( !ShowFolderRenameDialog(folder, &folderName, &mboxName, frame) )
         {
            wxLogStatus(frame, _("Folder renaming cancelled."));
         }
         else // we have got the new name
         {
            // try to rename (the text will be changed in MEvent handler
            // if OnRename() succeeds)
            (void)m_sink->OnRename(folder, folderName, mboxName);
         }
      }
      //else: can't rename this folder

      folder->DecRef();
   }
}

bool wxFolderTreeImpl::CanMoveFolder(const MFolder *folder) const
{
   return CanRenameFolder(folder);
}

void wxFolderTreeImpl::DoFolderMove()
{
   MFolder_obj folder(m_sink->GetSelection());
   if ( !folder )
   {
      wxLogError(_("Please select the folder to move first."));
   }
   else // have a folder
   {
      if ( CanMoveFolder(folder) )
      {
         wxFrame *frame = GetFrame(this);
         MFolder_obj newParent(MDialog_FolderChoose(frame, folder, MDlg_Folder_NoFiles));
         if ( !newParent )
         {
            wxLogStatus(frame, _("Folder moving cancelled."));
         }
         else if ( newParent == folder )
         {
            wxLogError(_("You chose the folder itself as its new parent !"));
         }
         else if ( newParent == folder->GetParent() )
         {
            wxLogError(_("You chose the current parent as new parent !"));
         }
         else if ( folder->GetSubfolderCount() > 0 )
         {
            wxLogError(_("Sorry, moving a hierarchy of folders is not implemented yet !"));
         }
         else
         {
            (void)m_sink->OnMove(folder, newParent);
         }
      }
      else
      {
         wxLogError(_("This folder can't be moved !"));
      }
   }
}

void wxFolderTreeImpl::DoFolderDelete(bool removeOnly)
{
   MFolder *folder = m_sink->GetSelection();
   if ( !folder )
   {
      wxLogError(_("Please select the folder to delete first."));

      return;
   }

   // don't try to delete folders which can't be deleted
   if ( !removeOnly && !CanDeleteFolderOfType(folder->GetType()) )
   {
      removeOnly = true;
   }

   if ( m_sink->OnDelete(folder, removeOnly) )
   {
      if ( folder == m_current->GetFolder() )
      {
         // don't leave invalid selection
         m_current = NULL;
      }

      wxLogStatus(GetFrame(this), _("Folder '%s' %s"),
                  folder->GetName().c_str(),
                  removeOnly ? _("removed from the tree")
                             : _("deleted"));
   }

   folder->DecRef();
}

void wxFolderTreeImpl::DoFolderClear()
{
   MFolder_obj folder(m_sink->GetSelection());
   if ( !folder )
   {
      wxLogError(_("Please select the folder to clear first."));

      return;
   }

   m_sink->OnClear(folder);
}

void wxFolderTreeImpl::DoFolderUpdate()
{
   MFolder_obj folder(m_sink->GetSelection());
   if ( !folder.IsOk() )
   {
      wxLogError(_("Please select the folder to update first."));

      return;
   }

   (void)m_sink->OnUpdate(folder);
}

void wxFolderTreeImpl::DoFolderClose()
{
   MFolder_obj folder(m_sink->GetSelection());
   if ( !folder.IsOk() )
   {
      wxLogError(_("Please select the folder to close first."));

      return;
   }

   (void)m_sink->OnClose(folder);
}

void wxFolderTreeImpl::DoBrowseSubfolders()
{
   m_sink->OnBrowseSubfolders(m_sink->GetSelection());
}

void wxFolderTreeImpl::DoFolderProperties()
{
   m_sink->OnProperties(m_sink->GetSelection());
}

void wxFolderTreeImpl::DoToggleHidden()
{
   m_showHidden = !ShowHiddenFolders();

   mApplication->GetProfile()->writeEntry(MP_SHOW_HIDDEN_FOLDERS, m_showHidden);

   ReopenBranch(GetRootItem());
}

// ----------------------------------------------------------------------------
// tree traversal and looking for unread folders
// ----------------------------------------------------------------------------

wxTreeItemId wxFolderTreeImpl::GetNextItem(wxTreeItemId id, bool next) const
{
   // garbage in, garbage out
   CHECK( id.IsOk(), id, _T("invalid tree item in GetNextItem") );

#if wxCHECK_VERSION(2, 5, 0)
   wxTreeItemIdValue cookie;
#else
   long cookie;
#endif
   wxTreeItemId idNext;

   if ( next )
   {
      // first try our child
      idNext = GetFirstChild(id, cookie);

      if ( !idNext.IsOk() )
      {
         wxTreeItemId idParent = id;
         while ( idParent.IsOk() )
         {
            // then try sibling just below
            idNext = GetNextSibling(idParent);

            // still doesn't work? try the next sibling of our parent
            if ( idNext.IsOk() )
               break;

            idParent = GetItemParent(idParent);
         }
      }
   }
   else // previous
   {
      idNext = GetPrevSibling(id);
      if ( !idNext.IsOk() )
      {
         // simple case: our previous item is our parent
         idNext = GetItemParent(id);
      }
      else // find the last child of our previous sibling now
      {
         for ( ;; )
         {
            wxTreeItemId idChild = GetLastChild(idNext);
            if ( !idChild.IsOk() )
               break;

            idNext = idChild;
         }
      }
   }

   // avoid infinite loops in the caller
   ASSERT_MSG( idNext != id, _T("logic error in GetNextItem") );

   return idNext;
}

wxTreeItemId
wxFolderTreeImpl::FindNextUnreadFolder(wxTreeItemId id, bool next) const
{
   // we want to find the next item, not the same one as we have now
   MfStatusCache *mfStatus = MfStatusCache::Get();
   if ( !mfStatus )
      return wxTreeItemId();

   id = GetNextItem(id, next);
   if ( !id.IsOk() )
   {
      return id;
   }

   // check first this item, then the next one(s)
   do
   {
      MFolder *folder = GetFolderTreeNode(id)->GetFolder();
      wxString name = folder->GetFullName();
      Profile_obj profile(folder->GetProfile());

      // this folder may be explicitly excluded from this search (it makes
      // sense for folders such as Trash...), check for it
      if ( !READ_CONFIG(profile, MP_FTREE_NEVER_UNREAD) )
      {
         // does it have any unread messages?
         MailFolderStatus status;
         if ( mfStatus->GetStatus(name, &status) && status.unread )
         {
            return id;
         }
      }

      id = GetNextItem(id, next);
   }
   while ( id.IsOk() );

   return id;
}

bool wxFolderTreeImpl::GoToNextUnreadFolder(bool next)
{
   // start from the beginning if no current node
   wxFolderTreeNode *node = GetSelection();
   wxTreeItemId id; // NB: gcc 2.91 dies if we use operator ?: here 
   if ( node )
      id = node->GetId();
   else
      id = GetRootItem();

   id = FindNextUnreadFolder(id, next);
   if ( !id.IsOk() )
   {
      wxLogStatus(GetFrame(this), _("No more folders with unread messages."));

      return false;
   }

   GoToItem(id);

   return true;
}

bool wxFolderTreeImpl::GoToHomeFolderIfAny()
{
   String folderHome = READ_APPCONFIG_TEXT(MP_FTREE_HOME);
   if ( !folderHome.empty() )
   {
      wxTreeItemId id = GetTreeItemFromName(folderHome);
      if ( id.IsOk() )
      {
         GoToItem(id);

         return true;
      }
   }

   return false;
}

void wxFolderTreeImpl::GoToHomeFolder()
{
   if ( !GoToHomeFolderIfAny() )
   {
      wxLogStatus(GetFrame(this), _("No home folder configured."));
   }
}

// ----------------------------------------------------------------------------
// label in place editing stuff
// ----------------------------------------------------------------------------

void wxFolderTreeImpl::OnBeginLabelEdit(wxTreeEvent& event)
{
   // someone clicked the tree, so the user must be back
   mApplication->UpdateAwayMode();

   MFolder *folder = m_sink->GetSelection();

   if ( !folder )
   {
      return;
   }

   // should we allow renaming this item?
   bool allow;
   {
      wxLogNull nolog;
      allow = CanRenameFolder(folder);
   }

   if ( folder )
      folder->DecRef();

   if ( !allow )
   {
      // can't rename this one
      event.Veto();
   }
   else
   {
      // leave only the label itself, get rid of suffix showing the number of
      // messages
      long item = event.GetItem();
      wxFolderTreeNode *node = GetFolderTreeNode(item);

      wxString label = GetItemText(item);
      wxString name = node->GetName();

      // remember m_suffix (containing the number of messages in the folder &c)
      // so that we can tackle it back on to the new name later
      if ( label.StartsWith(name, &m_suffix) )
      {
         SetItemText(item, name);
      }
      else
      {
         // what's going on? label is normally composed of name and suffix
         FAIL_MSG( _T("unexpected tree item label") );
      }

      // this is checked in OnIdle() to detect cancelling the label edit
      m_idEditedInPlace = item;
   }
}

void wxFolderTreeImpl::OnEndLabelEdit(wxTreeEvent& event)
{
   MFolder_obj folder(m_sink->GetSelection());
   CHECK_RET( folder, _T("how can we edit a label without folder?") );

   wxString label = event.GetLabel();

   if ( !label.empty() )
   {
      if ( label != folder->GetName() )
      {
         if ( !m_sink->OnRename(folder, label) )
         {
            wxTreeItemId id = event.GetItem();
            label = GetItemText(id);

            event.Veto();
         }
         //else: renamed ok, leave m_idEditedInPlace non null because
         //      we can't restore the suffix part of the label right now as it
         //      is going to be overwritten after we return (at least under
         //      MSW), and this will be done in OnIdle() later if
         //      m_idEditedInPlace != 0
      }
      //else: don't rename, the name didn't change
   }
   else // empty new label or renaming cancelled, don't do anything
   {
      event.Veto();
   }
}

void wxFolderTreeImpl::OnIdle(wxIdleEvent& event)
{
   if ( m_idEditedInPlace )
   {
      // check if the user finished editing the label
      if ( !IsEditingInPlace() )
      {
         String label = GetItemText(m_idEditedInPlace);

         if ( !m_suffix.empty() )
         {
            label += m_suffix;
            m_suffix.clear();
         }

         SetItemText(m_idEditedInPlace, label);

         m_idEditedInPlace = 0l;
      }
   }

   event.Skip();
}

bool wxFolderTreeImpl::IsEditingInPlace() const
{
   // FIXME: wxGTK doesn't have GetEditControl() and it doesn't work in wxMSW,
   //        need to fix wxTreeCtrl instead of this ugliness!
#ifdef __WXMSW__
   // argh, hack in a hack: we can't use SendMessage() because it's a name of
   // our class
   #define SendMessage SendMessageA

   if ( TreeView_GetEditControl((HWND)GetHWND()) )
      return true;

   #undef SendMessage
#endif // MSW

   return false;
}

// ----------------------------------------------------------------------------
// dragging items in the tree
// ----------------------------------------------------------------------------

// this function sets tree positions for all the siblings of the given folder
// up to and including it and returns the new index of idThis
int wxFolderTreeImpl::SetIndicesBefore(wxTreeItemId idThis) const
{
   static const int idxInvalid = GetNumericDefault(MP_FOLDER_TREEINDEX);

   const wxTreeItemId idParent = GetItemParent(idThis);

   int idxThis = 0;

   wxTreeItemIdValue cookie;
   wxTreeItemId id = GetFirstChild(idParent, cookie);
   while ( id.IsOk() )
   {
      MFolder * const folder = GetFolderFromTreeItem(id);
      if ( !folder )
         return idxInvalid;

      int idx = folder->GetTreeIndex();
      if ( idx != idxInvalid )
      {
         // so far, so good: remember the highest previous index
         ASSERT_MSG( idx >= idxThis, _T("inconsistent tree index values!") );

         idxThis = idx;
      }
      else // item without defined position in the tree
      {
         // set it now
         folder->SetTreeIndex(++idxThis);
      }

      if ( id == idThis )
         break;

      id = GetNextChild(idParent, cookie);
   }

   return idxThis;
}

// this functions moves up the tree positions of all the folders starting
// (inclusively) from the given one and returns the new index of idThis
int wxFolderTreeImpl::OffsetIndicesAfter(wxTreeItemId idThis) const
{
   static const int idxInvalid = GetNumericDefault(MP_FOLDER_TREEINDEX);

   const wxTreeItemId idParent = GetItemParent(idThis);

   wxTreeItemId id = GetLastChild(idParent);
   while ( id.IsOk() )
   {
      MFolder * const folder = GetFolderFromTreeItem(id);
      if ( !folder )
         return idxInvalid;

      int idx = folder->GetTreeIndex();
      if ( idx != idxInvalid )
      {
         // bump the index up
         folder->SetTreeIndex(idx + 1);
      }
      //else: nothing to do, leave it at the end of all the other items

      if ( id == idThis )
      {
         // done, alll indices from this item up to the end are offset
         ASSERT_MSG( idx != idxInvalid, _T("logic error") );

         return idx + 1;
      }

      id = GetPrevSibling(id);
   }

   return idxInvalid;
}

void
wxFolderTreeImpl::MoveFolderBetween(wxTreeItemId id,
                                    wxTreeItemId idBefore,
                                    wxTreeItemId idAfter) const
{
   static const int idxInvalid = GetNumericDefault(MP_FOLDER_TREEINDEX);

   // the (new) index of the folder, must be in ]idxBefore, idxAfter[
   int idx = idxInvalid;

   const MFolder * const folderAfter = GetFolderFromTreeItem(idAfter);
   if ( !folderAfter )
      return;

   int idxAfter = folderAfter->GetTreeIndex();
   if ( idxAfter == idxInvalid )
   {
      if ( idBefore.IsOk() )
      {
         const MFolder * const folderBefore = GetFolderFromTreeItem(idBefore);
         if ( !folderBefore )
            return;

         int idxBefore = folderBefore->GetTreeIndex();
         if ( idxBefore == idxInvalid )
         {
            // we have to fix the indices of all the items before this one so
            // make it position stick, otherwise it would bubble up to the top
            idx = SetIndicesBefore(idBefore);
            if ( idx == idxInvalid )
               return;

            idx++;
         }
         else // position just after idxBefore
         {
            idx = idxBefore + 1;
         }
      }
      else // make folder the first child
      {
         idx = 0;
      }
   }
   else // we must select an index less than idxAfter
   {
      if ( idBefore )
      {
         const MFolder * const folderBefore = GetFolderFromTreeItem(idBefore);
         if ( !folderBefore )
            return;

         int idxBefore = folderBefore->GetTreeIndex();

         // it can't possibly happen that an item without index is positioned
         // before another item with index
         ASSERT_MSG( idxBefore != idxInvalid, _T("confusion with tree indices!") );

         // this might not be the final value, it will be tested below
         idx = idxBefore + 1;
      }
      else // position just before idxAfter
      {
         // same as above: this value might be invalid
         idx = idxAfter - 1;
      }

      // did we choose a valid value above?
      if ( idx < 0 || idx >= idxAfter )
      {
         // we have to offset the indices of all subsequent items...
         idx = OffsetIndicesAfter(idAfter);
         if ( idx == idxInvalid )
            return;

         idx--;
      }
   }

   // do change the folder position
   ASSERT_MSG( idx != idxInvalid, _T("logic error in MoveFolderBetween()") );

   MFolder * const folder = GetFolderFromTreeItem(id);
   if ( !folder )
      return;

   folder->SetTreeIndex(idx);
}

void wxFolderTreeImpl::OnTreeBeginDrag(wxTreeEvent& event)
{
   // do we have something basically valid?
   const wxTreeItemId idDragged = event.GetItem();
   const MFolder * const folder = GetFolderFromTreeItem(idDragged);
   if ( !folder )
      return;

   // determine if we can drag this item at all?
   if ( folder->GetType() == MF_ROOT ||
         (folder->GetFlags() & MF_FLAGS_DONTDELETE) )
   {
      wxLogStatus(GetFrame(this),
                 _("The folder \"%s\" cannot be moved."),
                 folder->GetFullName().c_str());
      return;
   }

   // ok, allow the user to drag it
   ASSERT_MSG( !m_idDragged.IsOk(), _T("didn't finish the previous drag?") );

   m_idDragged = idDragged;
   event.Allow();
}

void wxFolderTreeImpl::OnTreeEndDrag(wxTreeEvent& event)
{
   CHECK_RET( m_idDragged.IsOk(), _T("end drag without begin drag?") );

   // get the source and destination folders
   const wxTreeItemId idDst = event.GetItem();
   const wxTreeItemId idSrc = m_idDragged;
   m_idDragged.Unset();

   if ( !idDst.IsOk() )
   {
      // happens if the user dragged the mouse completely outside the window,
      // for example
      return;
   }

   MFolder * const folderDst = GetFolderFromTreeItem(idDst);
   MFolder * const folderSrc = GetFolderFromTreeItem(idSrc);

   if ( !folderDst || !folderSrc )
   {
      // this is not supposed to happen (debug error message already given)
      return;
   }

   // do we want to just change the item position among its siblings or move it?
   if ( folderDst->GetParent() == folderSrc->GetParent() )
   {
      // just changing the order: put the src folder just before the dst one 
      // (not just after: this would make it impossible to reposition a folder
      // to be the first one among its siblings, as dropping it on the parent
      // would move it one level up)
      MoveFolderBetween(idSrc, GetPrevSibling(idDst), idDst);

      // reflect the changes in the tree
      ReopenBranch(GetItemParent(idDst));
   }
   else // really moving
   {
      if ( !folderSrc->Move(folderDst) )
      {
         wxLogError(_("Failed to move the folder \"%s\" to \"%s\"."),
                    folderSrc->GetFullName().c_str(),
                    folderDst->GetFullName().c_str());
      }
      else // moved ok
      {
         wxLogStatus(GetFrame(this),
                     _("Successfully moved the folder \"%s\" to \"%s\"."),
                     folderSrc->GetFullName().c_str(),
                     folderDst->GetFullName().c_str());
      }
   }
}

// ----------------------------------------------------------------------------
// tree notifications
// ----------------------------------------------------------------------------

// add all subfolders of the folder being expanded
void wxFolderTreeImpl::OnTreeExpanding(wxTreeEvent& event)
{
   // someone clicked the tree, so the user must be back
   mApplication->UpdateAwayMode();

   ASSERT_MSG( event.GetEventObject() == this, _T("got other treectrls event?") );

   wxTreeItemId itemId = event.GetItem();
   wxFolderTreeNode *parent = GetFolderTreeNode(itemId);

   if ( parent->WasExpanded() )
   {
      // only add the subfolders the _first_ time!
      return;
   }

   // now we are
   parent->SetExpandedFlag();

   MFolder *folder = parent->GetFolder();
   size_t nSubfolders = folder->GetSubfolderCount();

   // if there are no subfolders, indicate it to user by removing the [+]
   // button from this node
   if ( nSubfolders == 0 )
   {
      SetItemHasChildren(itemId, false);
   }
   else // we do have children
   {
      // we first collect all subfolders we should display in this array
      // because we need to sort them before really creating the tree nodes
      wxArrayFolder subfolders;

      // but we keep a flag telling us whether we really need to sort them: if
      // no positions are configured, we don't which is all the better
      bool shouldSort = false;

      size_t n;
      for ( n = 0; n < nSubfolders; n++ )
      {
         MFolder *subfolder = folder->GetSubfolder(n);
         if ( !subfolder )
         {
            // this is not expected to happen
            FAIL_MSG( _T("no subfolder?") );

            continue;
         }

         // if the folder is marked as being "hidden", we don't show it in the
         // tree (but still use for all other purposes), this is useful for
         // "system" folders like INBOX
         if ( !m_showHidden && (subfolder->GetFlags() & MF_FLAGS_HIDDEN) )
         {
            subfolder->DecRef();

            continue;
         }

         // do we have to sort folders at all? only do it if at least one of
         // them has a non default index
         if ( !shouldSort )
         {
            shouldSort = subfolder->GetTreeIndex() !=
               GetNumericDefault(MP_FOLDER_TREEINDEX);
         }

         // remember this one
         subfolders.Add(subfolder);
      }

      if ( shouldSort )
      {
         // sort the array by tree item position
         subfolders.Sort(CompareFoldersByTreePos);
      }

      // now do fill the tree
      nSubfolders = subfolders.GetCount();
      for ( n = 0; n < nSubfolders; n++ )
      {
         MFolder *subfolder = subfolders[n];

         // note that we give subfolder to wxFolderTreeNode: it will DecRef()
         // it later
         (void)new wxFolderTreeNode(this, subfolder, parent);
      }
   }
}

void wxFolderTreeImpl::OnTreeSelect(wxTreeEvent& event)
{
   // someone clicked the tree, so the user must be back
   mApplication->UpdateAwayMode();

   ASSERT_MSG( event.GetEventObject() == this, _T("got other treectrls event?") );

   wxTreeItemId itemId = event.GetItem();
   wxFolderTreeNode *newCurrent = itemId.IsOk() ? GetFolderTreeNode(itemId)
                                                : NULL;

   MFolder *oldsel = m_current ? m_current->GetFolder() : NULL;

   // do it now because due to use of wxYield() elsewhere, the other handlers
   // might be called _before_ this function returns and the tree selection is
   // updated
   m_current = newCurrent;

   m_curIsHidden = IsHidden(m_current);

   if ( !m_suppressSelectionChange )
   {
      MFolder *newsel = newCurrent ? newCurrent->GetFolder() : NULL;

      // send the event right now
      m_sink->OnSelectionChange(oldsel, newsel);
   }
   else
   {
      // it's not an error: happens when right click happens while popup menu
      // is still shown
      SafeDecRef(m_previousFolder);

      // store this to send it later
      m_previousFolder = oldsel;
      SafeIncRef(m_previousFolder);
   }

   m_selectedFolderName = m_current ? m_current->GetFolder()->GetFullName()
                                    : String();
}

bool wxFolderTreeImpl::OnDoubleClick()
{
   return

#ifdef USE_TREE_ACTIVATE_BUGFIX
   m_openedFolderOnDblClick =
#endif // USE_TREE_ACTIVATE_BUGFIX

   m_sink->OnDoubleClick();
}

void wxFolderTreeImpl::OnRightDown(wxMouseEvent& event)
{
   // for now... see comments near m_suppressSelectionChange declaration
   m_suppressSelectionChange = true;

   wxPoint pt = event.GetPosition();
   wxTreeItemId item = HitTest(pt);
   if ( item.IsOk() )
   {
      SelectItem(item);
   }
   else
   {
      item = wxTreeCtrl::GetSelection();
   }

#if 0
   // try to popup the menu in some reasonable position
   if ( item.IsOk() )
   {
      wxRect rect;
      GetBoundingRect(item, rect);
      pt.x = (rect.GetX() + rect.GetWidth())/2;
      pt.y = (rect.GetY() + rect.GetHeight())/2;
   }
#endif

   // show menu in any case
   DoPopupMenu(pt);

   // now send the selection change event
   MFolder *newsel = m_current ? m_current->GetFolder() : NULL;
   m_sink->OnSelectionChange(m_previousFolder, newsel);

   SafeDecRef(m_previousFolder); // matches IncRef() in OnTreeSelect()
   m_previousFolder = NULL;

   m_suppressSelectionChange = false;
}

#ifdef USE_MIDDLE_CLICK_HACK

void wxFolderTreeImpl::OnMiddleDown(wxMouseEvent& event)
{
   // translate middle mouse click into left double click
   wxMouseEvent event2 = event;
   event2.SetEventType(wxEVT_LEFT_DCLICK);
   wxTreeCtrl::OnMouse(event2);

   if ( event2.GetSkipped() )
      event.Skip();
}

#endif // USE_MIDDLE_CLICK_HACK

void wxFolderTreeImpl::OnMenuCommand(wxCommandEvent& event)
{
   // someone clicked in the tree, so the user must be back
   mApplication->UpdateAwayMode();

   if ( !ProcessMenuCommand(event.GetId()) )
      event.Skip();
}

bool wxFolderTreeImpl::ProcessMenuCommand(int id)
{
   switch ( id )
   {
      case FolderMenu::Open:
         DoFolderOpen();
         break;

      case FolderMenu::View:
         DoFolderView();
         break;

      case FolderMenu::New:
         DoFolderCreate();
         break;

      case FolderMenu::Delete:
         DoFolderDelete(false);
         break;

      case WXMENU_FOLDER_CLEAR:
         DoFolderClear();
         break;

      case FolderMenu::Remove:
         DoFolderDelete();
         break;

      case FolderMenu::Rename:
         DoFolderRename();
         break;

      case FolderMenu::Move:
         DoFolderMove();
         break;

      case FolderMenu::Close:
         DoFolderClose();
         break;

      case FolderMenu::Update:
         DoFolderUpdate();
         break;

      case FolderMenu::BrowseSub:
         DoBrowseSubfolders();
         break;

      case FolderMenu::Properties:
         DoFolderProperties();
         break;

      case FolderMenu::ShowHidden:
         DoToggleHidden();
         break;

      default:
         // we're now called from wxManFrame msg processing code so it's
         // possible for us to be called for another event - ignore silently
         //
         //FAIL_MSG(_T("unexpected menu command in wxFolderTree"));
         return false;
   }

   return true;
}

void wxFolderTreeImpl::OnKeyDown(wxTreeEvent& event)
{
   // we want to make Ctrl-Up/Down to go to the next folder with unread
   // messages in it
   const wxKeyEvent& evtKey = event.GetKeyEvent();
   if ( evtKey.ControlDown() )
   {
      int keycode = evtKey.GetKeyCode();
      switch ( keycode )
      {
         case WXK_DOWN:
         case WXK_UP:
            // do go to next or previous folder
            (void)GoToNextUnreadFolder(keycode == WXK_DOWN);
            break;

         case WXK_HOME:
            GoToHomeFolder();
            break;

         default:
            event.Skip();
      }
   }
   else
   {
      event.Skip();
   }
}

void wxFolderTreeImpl::OnChar(wxKeyEvent& event)
{
  // someone typed into the tree, so the user must be back
  mApplication->UpdateAwayMode();

  switch ( event.GetKeyCode() ) {
    case WXK_DELETE:
      DoFolderDelete(event.ShiftDown());
      break;

    case WXK_INSERT:
      DoFolderCreate();
      break;

    case WXK_RETURN:
      if ( event.AltDown() )
      {
         // with Alt => show properties (Windows standard...)
         DoFolderProperties();
         break;
      }
      //else: fall through to let it generate the ACTIVATED event

    default:
      event.Skip();
  }
}

void wxFolderTreeImpl::ReopenBranch(wxTreeItemId parent)
{
   wxString currentName;
   if ( m_current )
   {
      currentName = m_current->GetFolder()->GetFullName();
   }

   wxString expState = SaveExpandedBranches(parent);

   Collapse(parent);
   DeleteChildren(parent);
   GetFolderTreeNode(parent)->ResetExpandedFlag();
   SetItemHasChildren(parent, true);

   RestoreExpandedBranches(parent, expState);

   if ( !currentName.empty() )
   {
      wxTreeItemId id = GetTreeItemFromName(currentName);
      if ( id.IsOk() )
      {
         m_current = GetFolderTreeNode(id);
      }
      else
      {
         // this may happen if the previously selected folder became hidden and
         // is not shown in the tree any more
         m_current = NULL;
      }
   }
}

// ----------------------------------------------------------------------------
// event processing
// ----------------------------------------------------------------------------

bool wxFolderTreeImpl::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_FolderTreeChange )
   {
      MEventFolderTreeChangeData& event = (MEventFolderTreeChangeData &)ev;

      ProcessFolderTreeChange(event);
   }
   else if ( ev.GetId() == MEventId_OptionsChange )
   {
#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
      m_FocusFollowMode = READ_APPCONFIG_BOOL(MP_FOCUS_FOLLOWSMOUSE);
#endif // wxGTK || EXPERIMENTAL_FOCUS_FOLLOWS

      // reread the bg colour setting
      UpdateColours();

      MEventOptionsChangeData& event = (MEventOptionsChangeData &)ev;

      Profile *profileChanged = event.GetProfile();
      if ( !profileChanged )
      {
         // it's some profile which has nothing to do with us
         return true;
      }

      // find the folder which has been changed
      wxTreeItemId item = GetTreeItemFromName(profileChanged->GetFolderName());
      if ( item.IsOk() )
      {
         // the icon might have changed too
         switch ( event.GetChangeKind() )
         {
            case MEventOptionsChangeData::Apply:
               UpdateIcon(item, true /* temporary change, may be undone */);
               break;

            case MEventOptionsChangeData::Ok:
               UpdateIcon(item, false /* definitive change, no undo */);
               break;

            case MEventOptionsChangeData::Cancel:
               // restore the old values
               RestoreIcon(item);
               break;

            default:
               FAIL_MSG(_T("unknown options change event"));
         }

         // important: after calling ReopenBranch() we can't use item any more
         //            because it is invalidated by this call, so change the
         //            icon first and reexpand the tree after this!

         // hidden folder flags might have changed (either for the entire tree
         // or just for this folder)
         wxTreeItemId idToReopen;
         if ( ShowHiddenFolders() != m_showHidden )
         {
            // reexpend the entire tree
            idToReopen = GetRootItem();
         }
         else if ( !m_showHidden )  // if we show all, nothing can change!
         {
            if ( IsHidden(GetFolderTreeNode(item)) != m_curIsHidden )
            {
               // reexpand just the parent of this folder
               idToReopen = GetItemParent(item);

               m_curIsHidden = !m_curIsHidden;
            }
            //else: nothing changed, do nothing
         }

         if ( idToReopen.IsOk() )
         {
            ReopenBranch(idToReopen);
         }
      }
      //else: folder which changed is not in the tree
   }
   else if ( ev.GetId() == MEventId_FolderStatus )
   {
      MEventFolderStatusData& event = (MEventFolderStatusData &)ev;

      ProcessMsgNumberChange(event.GetFolderName());
   }
   else if ( ev.GetId() == MEventId_FolderClosed )
   {
      MEventWithFolderData& event = (MEventWithFolderData &)ev;
      if ( m_openFolderName == event.GetFolder()->GetName() )
      {
         SetOpenFolderName(_T(""));
      }
   }

   return true;
}

void
wxFolderTreeImpl::
ProcessFolderTreeChange(const MEventFolderTreeChangeData& event)
{
   wxString folderName = event.GetFolderFullName();
   MEventFolderTreeChangeData::ChangeKind kind = event.GetChangeKind();

   // find the item in the tree
   switch ( kind )
   {
      case MEventFolderTreeChangeData::Rename:
         {
            // just rename the item in the tree - if it's not already done as
            // it would be in the case of in-place label editing
            wxString folderNewName = event.GetNewFolderName();
            wxTreeItemId item = GetTreeItemFromName(folderNewName);
            if ( item.IsOk() )
            {
               // notice that the event carries the full folder name and we
               // only want to show the name, i.e. the part after '/' in the
               // tree
               wxString nameNew = folderNewName.AfterLast('/');

               // keep the old suffix, if any (renaming doesn't normally
               // change the number of messages in the folder)
               wxString label = GetItemText(item);
               wxString name = folderName.AfterLast('/');

               wxString suffix;
               if ( label.StartsWith(name, &suffix) )
               {
                  nameNew += suffix;
               }

               SetItemText(item, nameNew);
            }
         }
         break;

      case MEventFolderTreeChangeData::Create:
         {
            // if parentName is empty, the root will be returned which is ok
            wxString parentName = folderName.BeforeLast('/');
            wxTreeItemId parent = GetTreeItemFromName(parentName);
            CHECK_RET( parent.IsOk(), _T("no such item in the tree??") );

            wxFolderTreeNode *nodeParent = GetFolderTreeNode(parent);
            if ( nodeParent->WasExpanded() )
            {
               // the folder is given to wxFolderTreeNode, hence don't DecRef()
               // it (and don't use MFolder_obj)
               MFolder *folder = MFolder::Get(folderName);

               CHECK_RET( folder, _T("just created folder doesn't exist?") );

               // insert the new folder at the right place
               int pos = 0;
#if wxCHECK_VERSION(2, 5, 0)
               wxTreeItemIdValue cookie;
#else
               long cookie;
#endif
               wxTreeItemId item = GetFirstChild(parent, cookie);
               while ( item.IsOk() )
               {
                  MFolder *folder2 = GetFolderTreeNode(item)->GetFolder();
                  int rc = CompareFoldersByTreePos(&folder, &folder2);
                  if ( rc == 0 )
                  {
                     // CompareFoldersByTreePos() doesn't look at the folder
                     // names as they are always sorted when it is called from
                     // OnTreeExpanding() but we should do it specially here
                     rc = wxStrcmp(folder->GetName(), folder2->GetName());
                  }

                  if ( rc < 0 )
                  {
                     // we should insert it here
                     break;
                  }

                  pos++;

                  item = GetNextChild(parent, cookie);
               }

               // create the entry as the tree won't do it for us any more
               (void)new wxFolderTreeNode(this, folder, nodeParent, pos);
            }
            else // parent has never been expanded
            {
               // no need to add the item to the tree, Expand() below will do
               // it for us

               // if the parent hadn't had any children before, it would be
               // impossible to open it now if we don't do this
               SetItemHasChildren(parent, true);

               // force creation of the new item
               Expand(parent);
            }

            // expand/scroll if necessary to show the newly created item
            EnsureVisible(GetTreeItemFromName(folderName));
         }
         break;

      case MEventFolderTreeChangeData::CreateUnder:
         {
            wxTreeItemId parent = GetTreeItemFromName(folderName);
            CHECK_RET( parent.IsOk(), _T("no such item in the tree??") );

            if ( IsExpanded(parent) )
            {
               // refresh the branch of the tree with the parent of the folder
               // which changed for all usual events or this folder itself for
               // CreateUnder events (which are sent when (possibly) multiple
               // folders were created under the common parent)
               ReopenBranch(parent);
            }
            else // wasn't expanded yet, no need to reopen - just open
            {
               SetItemHasChildren(parent, true);

               // expand the branch even if it wasn't expanded before - we want
               // to show the newly created folders
               Expand(parent);
            }

            EnsureVisible(parent);
         }
         break;

      case MEventFolderTreeChangeData::Delete:
         {
            wxTreeItemId item = GetTreeItemFromName(folderName);
            CHECK_RET( item.IsOk(), _T("no such item in the tree??") );

            wxTreeItemId parent = GetItemParent(item);

            // if the deleted folder was either the tree ctrl selection or was
            // opened (these 2 folders may be different), refresh
            if ( folderName == m_openFolderName ||
                 folderName == m_selectedFolderName )
            {
               SelectItem(parent);
            }

            Delete(item);

            // remove "+" button if no children left
            if ( GetChildrenCount(parent, false /* not recursively */) == 0 )
            {
               SetItemHasChildren(parent, false);
            }
         }
         break;
   }
}

// ----------------------------------------------------------------------------
// folder status stuff
// ----------------------------------------------------------------------------

void wxFolderTreeImpl::ProcessMsgNumberChange(const wxString& folderName)
{
   wxLogTrace(M_TRACE_MFSTATUS, _T("Folder tree: status changed for '%s'."),
              folderName.c_str());

   // check if we need to react to this event at all
   // ----------------------------------------------

   // first find out which folder does it concern
   MFolder_obj folder(folderName);

   if ( !folder )
   {
      // it's not an error: MTempFolder objects are not in the tree, yet they
      // generate MEventId_FolderStatus events as well
      //
      // just ignore them as we don't show their status anyhow
      wxLogTrace(M_TRACE_MFSTATUS,
                 _T("Folder tree: ignoring it because the folder is not in tree"));

      return;
   }

   // don't bother counting the messages - it may be time consuming - if
   // we don't need to update this item
   //
   // NB: we do update the status for the hidden folders because otherwise
   //     we'd have to rescan the tree completely each time the user does "show
   //     hidden folders in tree"
   Profile_obj profile(folder->GetProfile());
   if ( READ_CONFIG_TEXT(profile, MP_FTREE_FORMAT).empty() )
   {
      wxLogTrace(M_TRACE_MFSTATUS,
                 _T("Folder tree: ignoring it because we don't show status"));

      return;
   }

   // ok, do update the item
   // ----------------------

   // first get the status of the folder
   MailFolderStatus status;
   if ( !MfStatusCache::Get()->GetStatus(folderName, &status) )
   {
      wxLogTrace(M_TRACE_MFSTATUS,
                 _T("Folder tree: no cached status, calling CountAllMessages()"));

      // we don't have the status right now, count the messages (the folder
      // should be already opened, we don't want to open it just for this)
      MailFolder_obj mf(MailFolder::GetOpenedFolderFor(folder));
      if ( !mf )
      {
         wxLogDebug(_T("Failed to update status for '%s'"), folderName.c_str());
      }
      else
      {
         MailFolderStatus status;
         (void)mf->CountAllMessages(&status);
      }

      return;
   }

   // next find the folder in the tree
   wxTreeItemId item = GetTreeItemFromName(folderName);
   if ( !item.IsOk() )
   {
      // it can happen it the folder is hidden but otherwise we should have it
      ASSERT_MSG( folder->GetFlags() & MF_FLAGS_HIDDEN,
                  _T("update folder not in the tree?") );

      return;
   }

   // do update the status of the item in the tree
   wxFolderTreeNode *node = GetFolderTreeNode(item);
   node->SetStatus(this, status);

   wxLogTrace(M_TRACE_MFSTATUS, _T("Folder tree: updated status for '%s'"),
              folderName.c_str());
}

// ----------------------------------------------------------------------------
// folder icons
// ----------------------------------------------------------------------------

// update the icon of the selected folder: called with tmp == false when [Ok]
// button is pressed or true if it was the [Apply] button
void wxFolderTreeImpl::UpdateIcon(const wxTreeItemId item, bool tmp)
{
   wxFolderTreeNode *node = GetFolderTreeNode(item);
   CHECK_RET( node, _T("can't update icon of non existing node") );

   // first remember the old icon to be able to restore it
   if ( tmp )
   {
      int imageOld = GetItemImage(item);
      m_itemsWithChangedIcons.Add(new ItemWithChangedIcon(item, imageOld));
   }
   else
   {
      // some cleanup: if we "Applied" this change before, now we can remove
      // the undo information because we can't undo anymore after "Ok"
      size_t n;
      if ( FindItemWithChangedIcon(item, &n) )
      {
         m_itemsWithChangedIcons.RemoveAt(n);
      }
   }

   // and now set the new one
   MFolder *folder = node->GetFolder();
   int image = GetFolderIconForDisplay(folder);
   SetItemImage(item, image);
}

// restore the icon previously changed by UpdateIcon(false)
void wxFolderTreeImpl::RestoreIcon(const wxTreeItemId item)
{
   size_t n;
   if ( FindItemWithChangedIcon(item, &n) )
   {
      // restore icon
      SetItemImage(item, m_itemsWithChangedIcons[n].image);

      // and delete the record - can't cancel the same action more than once
      m_itemsWithChangedIcons.RemoveAt(n);
   }
   //else: the icon wasn't changed, so nothing to do
}

// find the item in the list of items whose icons were changed, return true if
// found or false otherwise
bool wxFolderTreeImpl::FindItemWithChangedIcon(const wxTreeItemId& item,
                                               size_t *index)
{
   size_t nCount = m_itemsWithChangedIcons.GetCount();
   for ( size_t n = 0; n < nCount; n++ )
   {
      if ( m_itemsWithChangedIcons[n].item == item )
      {
         *index = n;

         return true;
      }
   }

   return false;
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

#ifdef USE_TREE_ACTIVATE_BUGFIX

// this is a bug fix around MSW quirk: activating an item with the mouse will
// always toggle the branch, although we don't want to do it if we open the
// folder on activate
bool wxFolderTreeImpl::MSWOnNotify(int idCtrl,
                                   WXLPARAM lParam,
                                   WXLPARAM *result)
{
    NMHDR *hdr = (NMHDR *)lParam;

    bool isDblClk = hdr->code == (UINT)NM_DBLCLK; // cast for cygwin
    if ( isDblClk )
    {
       m_openedFolderOnDblClick = false;
    }

    bool rc = wxPTreeCtrl::MSWOnNotify(idCtrl, lParam, result);

    if ( isDblClk )
    {
       if ( m_openedFolderOnDblClick )
       {
          // prevent default processing from taking place
          *result = true;
       }
    }

    return rc;
}

#endif // USE_TREE_ACTIVATE_BUGFIX

wxFolderTreeImpl::~wxFolderTreeImpl()
{
   MEventManager::DeregisterAll(&m_eventFolderChange,
                                &m_eventOptionsChange,
                                &m_eventFolderStatus,
                                &m_eventFolderClose,
                                NULL);

   delete GetImageList();

   delete m_menuRoot;
   delete m_menu;
}

// ----------------------------------------------------------------------------
// global functions from include/FolderType.h implemented here
// ----------------------------------------------------------------------------

size_t GetNumberOfFolderIcons()
{
   return wxFolderTree::iconFolderMax;
}

String GetFolderIconName(size_t n)
{
   static const wxChar *aszImages[] =
   {
      // should be in sync with the corresponding enum (FolderIcon)!
      _T("folder_inbox"),
      _T("folder_file"),
      _T("folder_file"),
      _T("folder_pop"),
      _T("folder_imap"),
      _T("folder_nntp"),
      _T("folder_news"),
      _T("folder_root"),
      _T("folder_group"),
      _T("folder_nntp"),
      _T("folder_imap"),
      _T("folder_newmail"),
      _T("folder_sentmail"),
      _T("folder_palmpilot"),
      _T("folder_outbox"),
      _T("folder_trash")
   };

   ASSERT_MSG( wxFolderTree::iconFolderMax == WXSIZEOF(aszImages),
               _T("bad number of icon names") );

   CHECK( n < WXSIZEOF(aszImages), _T(""), _T("invalid icon index") );

   return aszImages[n];
}

int GetFolderIconForDisplay(const MFolder* folder)
{
   // first of all, try to ask the folder itself - this will return something
   // if this folder has its own "special" icon
   int image = folder->GetIcon();

   if ( image == -1 )
   {
      // if there is no special icon for this folder, use a default one for its
      // type
      image = GetDefaultFolderTypeIcon(folder->GetType());
   }

   return image;
}

int GetDefaultFolderTypeIcon(MFolderType folderType)
{
   // each folder type has its own icon
   static const struct
   {
      wxFolderTree::FolderIcon icon;
      MFolderType type;
   } FolderIcons[] =
     {
      { wxFolderTree::iconInbox,         MF_INBOX       },
      { wxFolderTree::iconFile,          MF_FILE        },
      { wxFolderTree::iconMH,            MF_MH          },
      { wxFolderTree::iconPOP,           MF_POP         },
      { wxFolderTree::iconIMAP,          MF_IMAP        },
      { wxFolderTree::iconNNTP,          MF_NNTP        },
      { wxFolderTree::iconNews,          MF_NEWS        },
      { wxFolderTree::iconRoot,          MF_ROOT        },
      { wxFolderTree::iconGroup,         MF_GROUP       },
      { wxFolderTree::iconFile,          MF_MFILE       },
      { wxFolderTree::iconMH,            MF_MDIR        },
     };

   int image = -1;
   size_t n;
   for ( n = 0; (image == -1) && (n < WXSIZEOF(FolderIcons)); n++ )
   {
      if ( folderType == FolderIcons[n].type )
      {
         image = FolderIcons[n].icon;
      }
   }

   ASSERT_MSG( image != -1, _T("no icon for this folder type") );

   return image;
}
