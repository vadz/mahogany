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

/*
   TODO

   1. Renaming
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "MApplication.h"
#  include "Profile.h"

#  include <wx/confbase.h>
#endif // USE_PCH

#include "MDialogs.h"

#include "MFolder.h"
#include "MFolderDialogs.h"

#include <wx/treectrl.h>

#include "gui/wxFolderTree.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum FolderIcon
{
   iconInbox,
   iconFile,
   iconMH,
   iconPOP,
   iconIMAP,
   iconNNTP,
   iconNews,
   iconRoot,
   iconGroup,
   iconNewsHierarchy,   // also these types are not used any more, do keep them
   iconImapDirectory,   // to avoid changing values of others (compatibility!)
   iconNewMail,
   iconSentMail,
   iconPalmPilot,
   iconTrash,
   iconFolderMax
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return TRUE if we're showing all folders, even hidden ones, in the tree
static bool ShowHiddenFolders()
{
   return READ_APPCONFIG(MP_SHOW_HIDDEN_FOLDERS) != 0;
}

// can this folder be opened?
static bool CanOpen(const MFolder *folder)
{
  return folder &&
         folder->GetType() != MF_ROOT &&
         folder->GetType() != MF_GROUP &&
         !(folder->GetFlags() & MF_FLAGS_GROUP);
}

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
   // the ctor creates the element and inserts it in the tree
   wxFolderTreeNode(wxTreeCtrl *tree,
                    MFolder *folder,
                    wxFolderTreeNode *parent = NULL);

   // dtor
   //
   // NB: the folder passed to the ctor will be released by this class, i.e.
   //     it takes ownership of it, the caller should not release it!
   virtual ~wxFolderTreeNode() { SafeDecRef(m_folder); }

   // accessors
      // has the side effect of setting a flag which tells us that we were
      // expanded - the overall result is this it only returns false once,
      // subsequent calls will always return true.
   bool WasExpanded()
   {
      bool wasExpanded = m_wasExpanded;
      m_wasExpanded = true;   // because we're called from OnExpand()

      return wasExpanded;
   }
      // reset the "was expanded" flag (useful when the subtree must be
      // recreated)
   void ResetExpandedFlag() { m_wasExpanded = false; }

      // NB: DecRef() shouldn't be called on the pointer returned by this
      //     function
   MFolder          *GetFolder() const { return m_folder; }
   wxFolderTreeNode *GetParent() const { return m_parent; }

private:
   // not implemented
   wxFolderTreeNode(const wxFolderTreeNode&);
   wxFolderTreeNode& operator=(const wxFolderTreeNode&);

   MFolder *m_folder;            // the folder we represent

   wxFolderTreeNode *m_parent;   // the parent node (may be NULL)

   bool m_wasExpanded;
};

// the tree itself
class wxFolderTreeImpl : public wxTreeCtrl, public MEventReceiver
{
public:
   // ctor
   wxFolderTreeImpl(wxFolderTree *sink, wxWindow *parent, wxWindowID id,
                const wxPoint& pos, const wxSize& size);
   // and dtor
   ~wxFolderTreeImpl();

   // accessors
   wxFolderTreeNode *GetSelection() const { return m_current; }

   // helpers
      // find the tree item by full name (as this function changes the state
      // of the tree - it may expand it - it is not `const')
   wxTreeItemId GetTreeItemFromName(const String& fullname);

      // change the currently opened (in the main frame) folder name: we need
      // it to notify the main frame if this folder is deleted
   void SetOpenFolderName(const String& name) { m_openFolderName = name; }

   // callbacks
   void OnChar(wxKeyEvent&);

   void OnDoubleClickHandler(wxMouseEvent&) { OnDoubleClick(); }
   void OnRightDown(wxMouseEvent& event);

   void OnMenuCommand(wxCommandEvent&);

   void OnTreeExpanding(wxTreeEvent&);
   void OnTreeSelect(wxTreeEvent&);

   // event processing function
   virtual bool OnMEvent(MEventData& event);

#ifdef __WXGTK__
   void OnMouseMove(wxMouseEvent &event)
   {
      if(m_FocusFollowMode && FindFocus() != this) SetFocus();
   }
#endif // wxGTK

   // get the folder tree node object from item id
   wxFolderTreeNode *GetFolderTreeNode(const wxTreeItemId& item)
   {
      return (wxFolderTreeNode *)GetItemData(item);
   }

protected:
   // return TRUE if the current folder may be opened
   bool CanOpen() const
   {
      // is the root item chosen?
      if ( wxTreeCtrl::GetSelection() == GetRootItem() )
         return FALSE;

      wxFolderTreeNode *node = GetSelection();

      CHECK( node, FALSE, "shouldn't be called if no selection" );

      MFolder *folder = node->GetFolder();

      return (folder->GetType() != MF_GROUP) &&
            !(folder->GetFlags() & MF_FLAGS_GROUP);
   }

   // this is the real handler for double-click and enter events
   void OnDoubleClick();

   // always opens the folder in a separate view
   void DoFolderOpen();

   void DoFolderCreate();
   void DoFolderDelete();
   void DoFolderRename();

   void DoBrowseSubfolders();

   void DoFolderProperties();
   void DoToggleHidden();

   void DoPopupMenu(const wxPoint& pos);

   // reexpand branch - called when something in the tree changes
   void ReopenBranch(wxTreeItemId parent);

   // returns TRUE if the given node has hidden flag set in the profile
   bool IsHidden(wxFolderTreeNode *node)
   {
      return node ? (node->GetFolder()->GetFlags() & MF_FLAGS_HIDDEN) != 0
                  : FALSE;
   }

private:
   class FolderMenu : public wxMenu
   {
   public:
      // menu items ids
      enum
      {
         Open,
         New,
         Delete,
         Rename,
         BrowseSub,
         Properties,
         ShowHidden
      };

      FolderMenu(bool isRoot)
      {
         if ( !isRoot )
         {
            Append(Open, _("&Open"));

            AppendSeparator();
         }

         Append(New, _("Create &new folder..."));
         if ( !isRoot )
         {
            Append(Delete, _("&Delete folder"));
         }
         Append(Rename, _("&Rename folder..."));

         AppendSeparator();

         Append(BrowseSub, _("Browse &subfolders..."));

         AppendSeparator();

         if ( isRoot )
         {
            Append(ShowHidden, _("Show &hidden folders"), "", TRUE);
         }
         else
         {
            Append(Properties, _("&Properties"));
         }
      }
   } *m_menuRoot,                 // popup menu for the root folder
     *m_menu;                     // popup menu for all folders

   wxFolderTree      *m_sink;     // for sending events
   wxFolderTreeNode  *m_current;  // current selection (NULL if none)

   // event registration handles
   void *m_eventFolderChange;    // for folder creatio/destruction
   void *m_eventOptionsChange;   // options change (update icons)

   // the full names of the folder currently opened in the main frame and
   // of the current selection in the tree ctrl (empty if none)
   String m_openFolderName,
          m_selectedFolderName;

   // icon management
      // update the icon for the selected folder, if "tmp" is TRUE remember the
      // old value to be able to restore it later with RestoreIcon()
   void UpdateIcon(const wxTreeItemId item, bool tmp = FALSE);

      // restore the icon previously changed by UpdateIcon(FALSE)
   void RestoreIcon(const wxTreeItemId item);

      // find item in m_itemsWithChangedIcons and return its index in the out
      // parameter and TRUE if found (otherwise return FALSE)
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

#ifdef __WXGTK__
   // give focus to the tree when mouse enters it [used under unix only]
   bool m_FocusFollowMode;
#endif // wxGTK

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

// tree events
BEGIN_EVENT_TABLE(wxFolderTreeImpl, wxTreeCtrl)
   // don't specify the control id - we shouldn't get any other tree events
   // (except our owns) anyhow
   EVT_TREE_SEL_CHANGED(-1,    wxFolderTreeImpl::OnTreeSelect)
   EVT_TREE_ITEM_EXPANDING(-1, wxFolderTreeImpl::OnTreeExpanding)

   EVT_LEFT_DCLICK(wxFolderTreeImpl::OnDoubleClickHandler)
   EVT_RIGHT_DOWN(wxFolderTreeImpl::OnRightDown)

   EVT_CHAR(wxFolderTreeImpl::OnChar)

   EVT_MENU(-1, wxFolderTreeImpl::OnMenuCommand)

#ifdef __WXGTK__
   EVT_MOTION (wxFolderTreeImpl::OnMouseMove)
#endif // wxGTK
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

   if( READ_APPCONFIG(MP_EXPAND_TREECTRL) )
      m_tree->Expand(m_tree->GetRootItem());
}

wxFolderTree::~wxFolderTree()
{
   ProfilePathChanger p(mApplication->GetProfile(), M_SETTINGS_CONFIG_SECTION);

   mApplication->GetProfile()->writeEntry(MP_EXPAND_TREECTRL,
      m_tree->IsExpanded(m_tree->GetRootItem()));
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
                  "GetTreeItemFromName() is buggy"
                );

      m_tree->SelectItem(item);

      return true;
   }
   else
   {
      return false;
   }
}

MFolder *wxFolderTree::GetSelection() const
{
   CHECK( m_tree, NULL, "you didn't call Init()" );

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
   ASSERT_MSG( m_tree, "you didn't call Init()" );

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
      // don't even try to open the root folder
      // don't try to open groups either
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
      m_tree->SetOpenFolderName("");
   }
}

// open a new folder view on this folder
void wxFolderTree::OnOpen(MFolder *folder)
{
   if ( CanOpen(folder) )
   {
      (void)wxFolderViewFrame::Create(
         // we need to pass relative names (profile names) into
         // the MailFolder class, or it will be interpreted as an
         // absolute path on the filesystem
         folder->GetFullName(),
         mApplication->TopLevelFrame());
   }
   else
   {
      FAIL_MSG("OnOpen() called for folder which can't be opened");
   }

   folder->DecRef();
}

// browse subfolders of this folder
void wxFolderTree::OnBrowseSubfolders(MFolder *folder)
{
   (void)ShowFolderSubfoldersDialog(folder, m_tree);

   folder->DecRef();
}

// bring up the properties dialog for this profile
void wxFolderTree::OnProperties(MFolder *folder)
{
   (void)ShowFolderPropertiesDialog(folder, m_tree);

   folder->DecRef();
}

MFolder *wxFolderTree::OnCreate(MFolder *parent)
{
   return ShowFolderCreateDialog(NULL, FolderCreatePage_Default, parent);
}

bool wxFolderTree::OnDelete(MFolder *folder)
{
   CHECK( folder, FALSE, "can't delete NULL folder" );

   if ( folder->GetFlags() & MF_FLAGS_DONTDELETE )
   {
      wxLogError(_("The folder '%s' is used by Mahogany and cannot be deleted"),
                 folder->GetName().c_str());
      return FALSE;
   }

   if ( folder->GetType() == MF_ROOT )
   {
      wxLogError(_("The root folder can not be deleted."));
      return FALSE;
   }

   const char *configPath;
   wxString msg;
   if ( folder->GetSubfolderCount() > 0 )
   {
      configPath = NULL; // this question can't be suppressed
      msg.Printf(_("Do you really want to delete folder '%s' and all of its\n"
                   "subfolders? You will permanently lose all the settings\n"
                   "for the deleted folders!"), folder->GetName().c_str());
   }
   else
   {
      configPath = "ConfirmFolderDelete";
      msg.Printf(_("Do you really want to delete folder '%s'?"),
                 folder->GetName().c_str());
   }

   bool ok = MDialog_YesNoDialog(msg,
                                 m_tree->wxWindow::GetParent(),
                                 MDIALOG_YESNOTITLE,
                                 FALSE /* 'no' default */,
                                 configPath);
   if ( ok )
   {
      // do delete it
      folder->Delete();
   }

   return ok;
}

// ----------------------------------------------------------------------------
// wxFolderTreeNode
// ----------------------------------------------------------------------------

wxFolderTreeNode::wxFolderTreeNode(wxTreeCtrl *tree,
                                   MFolder *folder,
                                   wxFolderTreeNode *parent)
{
   // init member vars
   m_parent = parent;
   m_folder = folder;
   m_wasExpanded = false;

   int image = GetFolderIconForDisplay(folder);

   // add this item to the tree
   if ( folder->GetType() == MF_ROOT )
   {
      SetId(tree->AddRoot(wxString(_("All folders")), image, image, this));
   }
   else
   {
      SetId(tree->AppendItem(GetParent()->GetId(), folder->GetName(),
                             image, image, this));
   }

   // allow the user to expand us if we have any children
   if ( folder )
   {
      tree->SetItemHasChildren(GetId(), folder->GetSubfolderCount() != 0);
   }
}

// ----------------------------------------------------------------------------
// wxFolderTreeImpl
// ----------------------------------------------------------------------------

wxFolderTreeImpl::wxFolderTreeImpl(wxFolderTree *sink,
                                   wxWindow *parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size)
                : wxTreeCtrl(parent, id, pos, size)
{
   // init member vars
   m_current = NULL;
   m_sink = sink;
   m_menu =
   m_menuRoot = NULL;
   m_suppressSelectionChange = FALSE;
   m_previousFolder = NULL;

   m_showHidden = ShowHiddenFolders();
   m_curIsHidden = FALSE;

#ifdef __WXGTK__
   m_FocusFollowMode = READ_APPCONFIG(MP_FOCUS_FOLLOWSMOUSE) != 0;
#endif // wxGTK

   // create an image list and associate it with this control
   size_t nIcons = GetNumberOfFolderIcons();

   wxImageList *imageList = new wxImageList(16, 16, FALSE, nIcons);

   wxIconManager *iconManager = mApplication->GetIconManager();
   for ( size_t n = 0; n < nIcons; n++ )
   {
      imageList->Add(iconManager->GetBitmap(GetFolderIconName(n)));
   }

   SetImageList(imageList);

   // create the root item
   MFolder *folderRoot = MFolder::Get("");
   m_current = new wxFolderTreeNode(this, folderRoot);

   // register with the event manager
   m_eventFolderChange = MEventManager::Register(*this,
                                                 MEventId_FolderTreeChange);
   m_eventOptionsChange = MEventManager::Register(*this,
                                                  MEventId_OptionsChange);

   ASSERT_MSG( m_eventFolderChange && m_eventOptionsChange,
               "folder tree failed to register with event manager" );
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

      FolderType folderType = folder->GetType();
      bool isRoot = folderType == MF_ROOT,
           isGroup = folderType == MF_GROUP;

      FolderMenu **menu = isRoot ? &m_menuRoot : &m_menu;

      // create our popup menu if not done yet
      if ( !*menu )
      {
         *menu = new FolderMenu(isRoot);
      }

      (*menu)->SetTitle(wxString::Format(_("Folder '%s'"), title.c_str()));

      if ( isRoot )
      {
         // init the menu
         (*menu)->Check(FolderMenu::ShowHidden, ShowHiddenFolders());
      }
      else
      {
         // disable the items which don't make sense for some kinds of folders:
         // groups can't be opened
         (*menu)->Enable(FolderMenu::Open, !isGroup);

         // these items only make sense when a folder can, in principle, have
         // inferiors (and browsing doesn't make sense for "simple" groups - what
         // would we browse for?)
         bool mayHaveSubfolders = CanHaveSubfolders(folderType, folder->GetFlags());
         (*menu)->Enable(FolderMenu::BrowseSub, mayHaveSubfolders && !isGroup);
         (*menu)->Enable(FolderMenu::New, mayHaveSubfolders);
      }

      PopupMenu(*menu, pos.x, pos.y);
   }
   //else: no selection
}

wxTreeItemId
wxFolderTreeImpl::GetTreeItemFromName(const String& fullname)
{
   wxArrayString components;
   wxSplitPath(components, fullname);

   wxTreeItemId current = GetRootItem();
   wxString currentPath;

   size_t count = components.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      // find the child of the current item which corresponds to (the [grand]
      // parent of) our folder
      if ( !!currentPath )
      {
         currentPath << '/';
      }
      currentPath << components[n];

      long cookie;
      wxTreeItemId child = GetFirstChild(current, cookie);
      while ( child.IsOk() )
      {
         // expand it first
         Expand(child);

         wxFolderTreeNode *node = GetFolderTreeNode(child);
         CHECK( node, false, "tree folder node without folder?" );

         if ( node->GetFolder()->GetFullName() == currentPath )
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

void wxFolderTreeImpl::DoFolderCreate()
{
   MFolder *folderNew = m_sink->OnCreate(m_sink->GetSelection());
   if ( folderNew != NULL )
   {
      // now done in OnMEvent()
#if 0
     wxTreeItemId idCurrent = wxTreeCtrl::GetSelection();
     wxFolderTreeNode *parent = GetFolderTreeNode(idCurrent);

     wxASSERT_MSG( parent, "can't get the parent of current tree item" );

     (void)new wxFolderTreeNode(this, folderNew, parent);
#endif // 0

     folderNew->DecRef();
   }
   //else: cancelled by user
}

void wxFolderTreeImpl::DoFolderDelete()
{
   MFolder *folder = m_sink->GetSelection();
   if ( !folder )
   {
      wxLogError(_("Please select the folder to delete first."));

      return;
   }

   if ( m_sink->OnDelete(folder) )
   {
      if ( folder == m_current->GetFolder() )
      {
         // don't leave invalid selection
         m_current = NULL;
      }

      wxLogStatus(_("Folder '%s' deleted"), folder->GetName().c_str());
   }

   folder->DecRef();
}

void wxFolderTreeImpl::DoFolderRename()
{
   FAIL_MSG("folder renaming not implemented"); // TODO
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

// add all subfolders of the folder being expanded
void wxFolderTreeImpl::OnTreeExpanding(wxTreeEvent& event)
{
   ASSERT_MSG( event.GetEventObject() == this, "got other treectrls event?" );

   wxTreeItemId itemId = event.GetItem();
   wxFolderTreeNode *parent = GetFolderTreeNode(itemId);

   if ( parent->WasExpanded() )
   {
      // only add the subfolders the _first_ time!
      return;
   }
   //else: calling WasExpanded() sets the expanded flag

   MFolder *folder = parent->GetFolder();
   size_t nSubfolders = folder->GetSubfolderCount();
   for ( size_t n = 0; n < nSubfolders; n++ )
   {
      MFolder *subfolder = folder->GetSubfolder(n);
      if ( !subfolder )
      {
         FAIL_MSG( "no subfolder?" );

         continue;
      }

      // if the folder is marked as being "hidden", we don't show it in the
      // tree (but still use for all other purposes), this is useful for
      // "system" folders like INBOX
      if ( !ShowHiddenFolders() && (subfolder->GetFlags() & MF_FLAGS_HIDDEN) )
      {
         subfolder->DecRef();

         continue;
      }

      // ok, create the new tree item
      (void)new wxFolderTreeNode(this, subfolder, parent);
   }

   // if there are no subfolders, indicate it to user by removing the [+]
   // button from this node
   if ( nSubfolders == 0 )
   {
      SetItemHasChildren(itemId, FALSE);
   }
}

void wxFolderTreeImpl::OnTreeSelect(wxTreeEvent& event)
{
   ASSERT_MSG( event.GetEventObject() == this, "got other treectrls event?" );

   wxTreeItemId itemId = event.GetItem();
   wxFolderTreeNode *newCurrent = GetFolderTreeNode(itemId);

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
                                    : wxString("");
}

void wxFolderTreeImpl::OnDoubleClick()
{
   if ( !CanOpen() )
   {
      wxLogStatus(GetFrame(this), _("Cannot open this folder."));

      return;
   }

   if ( READ_APPCONFIG(MP_OPEN_ON_CLICK) )
   {
      // then double click opens in a separate view
      DoFolderOpen();
   }
   else
   {
      // double click is needed to open it here
      m_sink->OnOpenHere(m_sink->GetSelection());
   }
}

void wxFolderTreeImpl::OnRightDown(wxMouseEvent& event)
{
   // for now... see comments near m_suppressSelectionChange declaration
   m_suppressSelectionChange = TRUE;

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

   m_suppressSelectionChange = FALSE;
}

void wxFolderTreeImpl::OnMenuCommand(wxCommandEvent& event)
{
   switch ( event.GetId() )
   {
      case FolderMenu::Open:
         DoFolderOpen();
         break;

      case FolderMenu::New:
         DoFolderCreate();
         break;

      case FolderMenu::Delete:
         DoFolderDelete();
         break;

      case FolderMenu::Rename:
         FAIL_MSG("renaming folders not yet implemented");
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
         //FAIL_MSG("unexpected menu command in wxFolderTree");
         event.Skip();
   }
}

void wxFolderTreeImpl::OnChar(wxKeyEvent& event)
{
  switch ( event.KeyCode() ) {
    case WXK_DELETE:
      DoFolderDelete();
      break;

    case WXK_INSERT:
      DoFolderCreate();
      break;

    case WXK_RETURN:
      if ( event.AltDown() )
      {
         // with Alt => show properties (Windows standard...)
         DoFolderProperties();
      }
      else
      {
         // without Alt it's the same as double click
         OnDoubleClick();
      }
      break;

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

   Collapse(parent);
   DeleteChildren(parent);
   GetFolderTreeNode(parent)->ResetExpandedFlag();
   SetItemHasChildren(parent, TRUE);
   Expand(parent);

   if ( !!currentName )
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

bool wxFolderTreeImpl::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_FolderTreeChange )
   {
      MEventFolderTreeChangeData& event = (MEventFolderTreeChangeData &)ev;

      String folderName = event.GetFolderFullName();

      // refresh the branch of the tree with the parent of the folder which
      // changed
      String parentName = folderName.BeforeLast('/');

      // recreate the branch
      wxTreeItemId parent = GetTreeItemFromName(parentName);

      CHECK(parent.IsOk(), TRUE, "no such item in the tree??");

      ReopenBranch(parent);

      if ( event.GetChangeKind() == MEventFolderTreeChangeData::Delete )
      {
         // if the deleted folder was either the tree ctrl selection or was
         // opened (these 2 folders may be different), refresh
         if ( folderName == m_openFolderName ||
              folderName == m_selectedFolderName )
         {
            SelectItem(GetRootItem());
         }
      }
   }
   else if ( ev.GetId() == MEventId_OptionsChange )
   {
#ifdef __WXGTK__
      m_FocusFollowMode = READ_APPCONFIG(MP_FOCUS_FOLLOWSMOUSE) != 0;
#endif // wxGTK

      MEventOptionsChangeData& event = (MEventOptionsChangeData &)ev;

      ProfileBase *profileChanged = event.GetProfile();
      if ( !profileChanged )
      {
         // it's some profile which has nothing to do with us
         return true;
      }

      // find the folder which has been changed
      wxTreeItemId item;

      wxString profileName = profileChanged->GetName();
      int pos = profileName.Find(M_PROFILE_CONFIG_SECTION);

      // don't know how to get folder name...
      CHECK( pos == 0, TRUE, "weird profile path" )

      // skip the M_PROFILE_CONFIG_SECTION prefix
      wxString folderName = profileName.c_str() +
                            strlen(M_PROFILE_CONFIG_SECTION);

      item = GetTreeItemFromName(folderName);

      if ( item.IsOk() )
      {
         // the icon might have changed too
         switch ( event.GetChangeKind() )
         {
            case MEventOptionsChangeData::Apply:
               UpdateIcon(item, TRUE /* temporary change, may be undone */);
               break;

            case MEventOptionsChangeData::Ok:
               UpdateIcon(item, FALSE /* definitive change, no undo */);
               break;

            case MEventOptionsChangeData::Cancel:
               // restore the old values
               RestoreIcon(item);
               break;

            default:
               FAIL_MSG("unknown options change event");
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
               idToReopen = GetParent(item);

               m_curIsHidden = !m_curIsHidden;
            }
            //else: nothing changed, do nothing
         }

         if ( idToReopen.IsOk() )
         {
            ReopenBranch(idToReopen);
         }
      }
   }

   return true;
}

// update the icon of the selected folder: called with tmp == FALSE when [Ok]
// button is pressed or TRUE if it was the [Apply] button
void wxFolderTreeImpl::UpdateIcon(const wxTreeItemId item, bool tmp)
{
   wxFolderTreeNode *node = GetFolderTreeNode(item);
   CHECK_RET( node, "can't update icon of non existing node" );

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
         m_itemsWithChangedIcons.Remove(n);
      }
   }

   // and now set the new one
   MFolder *folder = node->GetFolder();
   int image = GetFolderIconForDisplay(folder);
   SetItemImage(item, image);
}

// restore the icon previously changed by UpdateIcon(FALSE)
void wxFolderTreeImpl::RestoreIcon(const wxTreeItemId item)
{
   size_t n;
   if ( FindItemWithChangedIcon(item, &n) )
   {
      // restore icon
      SetItemImage(item, m_itemsWithChangedIcons[n].image);

      // and delete the record - can't cancel the same action more than once
      m_itemsWithChangedIcons.Remove(n);
   }
   //else: the icon wasn't changed, so nothing to do
}

// find the item in the list of items whose icons were changed, return TRUE if
// found or FALSE otherwise
bool wxFolderTreeImpl::FindItemWithChangedIcon(const wxTreeItemId& item,
                                               size_t *index)
{
   size_t nCount = m_itemsWithChangedIcons.GetCount();
   for ( size_t n = 0; n < nCount; n++ )
   {
      if ( m_itemsWithChangedIcons[n].item == item )
      {
         *index = n;

         return TRUE;
      }
   }

   return FALSE;
}

wxFolderTreeImpl::~wxFolderTreeImpl()
{
   MEventManager::Deregister(m_eventFolderChange);
   MEventManager::Deregister(m_eventOptionsChange);

   delete GetImageList();

   delete m_menu;
}

// ----------------------------------------------------------------------------
// global functions from include/FolderType.h implemented here
// ----------------------------------------------------------------------------

size_t GetNumberOfFolderIcons()
{
   return iconFolderMax;
}

String GetFolderIconName(size_t n)
{
   static const char *aszImages[] =
   {
      // should be in sync with the corresponding enum (FolderIcon)!
      "folder_inbox",
      "folder_file",
      "folder_file",
      "folder_pop",
      "folder_imap",
      "folder_nntp",
      "folder_news",
      "folder_root",
      "folder_group",
      "folder_nntp",
      "folder_imap",
      "folder_newmail",
      "folder_sentmail",
      "folder_palmpilot",
      "folder_trash"
   };

   ASSERT_MSG( iconFolderMax == WXSIZEOF(aszImages),
               "bad number of icon names" );

   CHECK( n < WXSIZEOF(aszImages), "", "invalid icon index" );

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

int GetDefaultFolderTypeIcon(FolderType folderType)
{
   // each folder type has its own icon
   static const struct
   {
      FolderIcon icon;
      FolderType type;
   } FolderIcons[] =
   {
      { iconInbox,         MF_INBOX       },
      { iconFile,          MF_FILE        },
      { iconMH,            MF_MH          },
      { iconPOP,           MF_POP         },
      { iconIMAP,          MF_IMAP        },
      { iconNNTP,          MF_NNTP        },
      { iconNews,          MF_NEWS        },
      { iconRoot,          MF_ROOT        },
      { iconGroup,         MF_GROUP       },
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

   ASSERT_MSG( image != -1, "no icon for this folder type" );

   return image;
}
