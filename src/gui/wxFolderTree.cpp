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

 + 1. Check this new code for memory leaks
   2. Implement OnCreate/Delete
 + 3. Add popup menu (on right click) with all operations
   4. Renaming
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

#include "wx/treectrl.h"

#include "gui/wxFolderTree.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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
   // constants
   enum Icon
   {
      iconInbox,
      iconFile,
      iconPOP,
      iconIMAP,
      iconNNTP,
      iconNews,
      iconRoot,
      iconMax
   };

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

protected:
   // is the root item chosen?
   bool IsRootChosen() const
   {
      return wxTreeCtrl::GetSelection() == GetRootItem();
   }

   // this is the real handler for double-click and enter events
   void OnDoubleClick();

   // always opens the folder in a separate view
   void DoFolderOpen() { m_sink->OnOpen(m_sink->GetSelection()); }

   void DoFolderCreate();
   void DoFolderDelete();
   void DoFolderRename();

   void DoFolderProperties();

   void DoPopupMenu(const wxPoint& pos);

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
         Properties
      };

      FolderMenu(const String& folderName)
      {
         String title;
         title << _("Folder '") << folderName << '\'';
         SetTitle(title);

         Append(Open, _("&Open"));

         AppendSeparator();

         Append(New, _("Create &new folder..."));
         Append(Delete, _("&Delete folder"));
         Append(Rename, _("&Rename folder..."));

         AppendSeparator();

         Append(Properties, _("&Properties"));
      }
   } *m_menu;                     // popup menu

   wxFolderTree      *m_sink;     // for sending events
   wxFolderTreeNode  *m_current;  // current selection (NULL if none)

   void              *m_eventReg; // event registration handle

   // the full names of the folder currently opened in the main frame and
   // of the current selection in the tree ctrl (empty if none)
   String m_openFolderName,
          m_selectedFolderName;

   // an ugly hack to prevent sending of the "selection changed" notification
   // when the folder is selected from the program: this is needed because
   // otherwise popup menu would be never shown for a folder which can't be
   // opened (bad password...) in the "click to open mode".
   //
   // MT-note: but this class is always used only by the GUI thread, so
   //          it's not too bad
   bool     m_suppressSelectionChange;
   MFolder *m_previousFolder;

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

   ProfilePathChanger p(mApplication->GetProfile(), M_SETTINGS_CONFIG_SECTION);
   if( READ_CONFIG(mApplication->GetProfile(), MP_EXPAND_TREECTRL) )
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
                  ((wxFolderTreeNode *)m_tree->GetItemData(item))->
                  GetFolder() == folder,
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
      if ( newsel && newsel->GetType() != FolderRoot )
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
   if ( folder->GetType() != FolderRoot )
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
      FAIL_MSG("can't open root pseudo-folder");
   }

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

   switch ( folder->GetType() )
   {
      case Inbox:
         wxLogError(_("You should not delete the INBOX folder:\n"
                      "it is automatically created by Mahogany to store your "
                      "incoming mail."));
         return FALSE;

      case FolderRoot:
         wxLogError(_("The root folder can not be deleted."));
         return FALSE;

      default:
         // no check
         break;
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

   // each folder type has its own icon
   static const struct
   {
      wxFolderTreeImpl::Icon icon;
      FolderType             type;
   } FolderIcons[] =
   {
      { wxFolderTreeImpl::iconInbox, Inbox      },
      { wxFolderTreeImpl::iconFile,  File       },
      { wxFolderTreeImpl::iconPOP,   POP        },
      { wxFolderTreeImpl::iconIMAP,  IMAP       },
      { wxFolderTreeImpl::iconNNTP,  Nntp       },
      { wxFolderTreeImpl::iconNews,  News       },
      { wxFolderTreeImpl::iconRoot,  FolderRoot }
   };

   FolderType type = folder->GetType();
   int image = -1;

   size_t n;
   for ( n = 0; n < WXSIZEOF(FolderIcons); n++ )
   {
      if ( type == FolderIcons[n].type )
      {
         image = FolderIcons[n].icon;
         break;
      }
   }

   ASSERT_MSG( n < WXSIZEOF(FolderIcons), "no icon for this folder type" );

   // add this item to the tree
   if ( folder->GetType() == FolderRoot )
   {
      SetId(tree->AddRoot(wxString(_("All folders")), image, image, this));
   }
   else
   {
      SetId(tree->AppendItem(GetParent()->GetId(), folder->GetName(),
                             image, image, this));
   }

   // allow the user to expand us even though we don't have any children yet
   tree->SetItemHasChildren(GetId());
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
   m_menu = NULL;
   m_suppressSelectionChange = FALSE;
   m_previousFolder = NULL;

   // create an image list and associate it with this control
   static const char *aszImages[] =
   {
      // should be in sync with the corresponding enum!
      "folder_inbox",
      "folder_file",
      "folder_pop",
      "folder_imap",
      "folder_nntp",
      "folder_news",
      "folder_root"
   };

   ASSERT_MSG( iconMax == WXSIZEOF(aszImages), "bad number of icon names" );

   wxImageList *imageList = new wxImageList(16, 16, FALSE, WXSIZEOF(aszImages));

   for ( size_t n = 0; n < WXSIZEOF(aszImages); n++ )
   {
      imageList->Add(mApplication->GetIconManager()->GetBitmap(aszImages[n]));
   }

   SetImageList(imageList);

   // create the root item
   MFolder *folderRoot = MFolder::Get("");
   (void)new wxFolderTreeNode(this, folderRoot);

   // register with the event manager
   m_eventReg = MEventManager::Register(*this, MEventId_FolderTreeChange);
   ASSERT_MSG( m_eventReg, "can't register for folder tree change event" );
}

void wxFolderTreeImpl::DoPopupMenu(const wxPoint& pos)
{
   wxFolderTreeNode *cur = GetSelection();
   if ( cur != NULL )
   {
      MFolder *folder = cur->GetFolder();

      wxString title(folder->GetName());
      if ( m_menu == NULL )
      {
         // create our popup menu if not done yet
         m_menu = new FolderMenu(title);
      }
      else
      {
         m_menu->SetTitle(title);
      }

      // disable the items which don't make sense for some kinds of folders
      bool isRoot = folder->GetType() == FolderRoot;

      // you can't open nor delete the root folder and it has no properties
      m_menu->Enable(FolderMenu::Open, !isRoot);
      m_menu->Enable(FolderMenu::Delete, !isRoot);
      m_menu->Enable(FolderMenu::Properties, !isRoot);

      PopupMenu(m_menu, pos.x, pos.y);
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
      currentPath << components[n];

      long cookie;
      wxTreeItemId child = GetFirstChild(current, cookie);
      while ( child.IsOk() )
      {
         // expand it first
         Expand(child);

         wxFolderTreeNode *node = (wxFolderTreeNode *)GetItemData(child);
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

void wxFolderTreeImpl::DoFolderCreate()
{
   MFolder *folderNew = m_sink->OnCreate(m_sink->GetSelection());
   if ( folderNew != NULL )
   {
      // now done in OnMEvent()
#if 0
     wxTreeItemId idCurrent = wxTreeCtrl::GetSelection();
     wxFolderTreeNode *parent = (wxFolderTreeNode *)GetItemData(idCurrent);

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
      // now done in OnMEvent()
#if 0
     Delete(wxTreeCtrl::GetSelection());

     SelectItem(GetRootItem());
#endif // 0

     wxLogStatus(_("Folder '%s' deleted"), folder->GetName().c_str());
   }

   folder->DecRef();
}

void wxFolderTreeImpl::DoFolderRename()
{
   FAIL_MSG("folder renaming not implemented"); // TODO
}

void wxFolderTreeImpl::DoFolderProperties()
{
   m_sink->OnProperties(m_sink->GetSelection());
}

// add all subfolders of the folder being expanded
void wxFolderTreeImpl::OnTreeExpanding(wxTreeEvent& event)
{
   ASSERT_MSG( event.GetEventObject() == this, "got other treectrls event?" );

   wxTreeItemId itemId = event.GetItem();
   wxFolderTreeNode *parent = (wxFolderTreeNode *)GetItemData(itemId);

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
      if ( subfolder )
      {
         (void)new wxFolderTreeNode(this, subfolder, parent);
      }
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
   wxFolderTreeNode *newCurrent = (wxFolderTreeNode *)GetItemData(itemId);

   MFolder *oldsel = m_current ? m_current->GetFolder() : NULL;
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

   m_current = newCurrent;
   m_selectedFolderName = m_current ? m_current->GetFolder()->GetFullName()
                                    : wxString("");
}

void wxFolderTreeImpl::OnDoubleClick()
{
   if ( IsRootChosen() )
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

      case FolderMenu::Properties:
         DoFolderProperties();
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

      ASSERT_MSG(parent.IsOk(), "no such item in the tree??");

      Collapse(parent);
      DeleteChildren(parent);
      ((wxFolderTreeNode *)GetItemData(parent))->ResetExpandedFlag();
      SetItemHasChildren(parent, TRUE);
      Expand(parent);

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

   return true;
}

wxFolderTreeImpl::~wxFolderTreeImpl()
{
   MEventManager::Deregister(m_eventReg);

   delete GetImageList();

   delete m_menu;
}
