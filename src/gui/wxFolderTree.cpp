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
#endif // USE_PCH

#include "MDialogs.h"

#include "MFolder.h"
#include "gui/wxFolderTree.h"
#include "gui/wxIconManager.h"
#include "gui/wxFolderView.h"

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

   // operations
      // delete all children (not from dtor because needs param)
   void DeleteChildren(wxTreeCtrl *tree);

   // accessors
   bool WasExpanded()
   {
      bool wasExpanded = m_wasExpanded;
      m_wasExpanded = true;   // because we're called from OnExpand()

      return wasExpanded;
   }

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
class wxFolderTreeImpl : public wxTreeCtrl
{
public:
   // constants
   enum Icon
   {
      iconInbox,
      iconFile,
      iconPOP,
      iconIMAP,
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

   // callbacks
   void OnChar(wxKeyEvent&);

   void OnDoubleClick(wxMouseEvent&)
      { DoFolderOpen(); }
   void OnRightDown(wxMouseEvent& event)
      { DoPopupMenu(event.GetPosition());  }

   void OnMenuCommand(wxCommandEvent&);

   void OnTreeExpanding(wxTreeEvent&);
   void OnTreeSelect(wxTreeEvent&);

protected:
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

   EVT_LEFT_DCLICK(wxFolderTreeImpl::OnDoubleClick)
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
}

wxFolderTree::~wxFolderTree()
{
   delete m_tree;
}

MFolder *wxFolderTree::GetSelection() const
{
   CHECK( m_tree, NULL, "you didn't call Init()" );

   wxFolderTreeNode *node = m_tree->GetSelection();
   return ( node == NULL ) ? NULL : node->GetFolder();
}

wxWindow *wxFolderTree::GetWindow() const
{
   ASSERT_MSG( m_tree, "you didn't call Init()" );

   return m_tree;
}

void wxFolderTree::OnSelectionChange(MFolder *oldsel, MFolder *newsel)
{
   // this function is intentionally left blank
}

// open a new folder view on this folder
void wxFolderTree::OnOpen(MFolder *folder)
{
   (void)wxFolderViewFrame::Create(folder->GetName(),
                                   mApplication->TopLevelFrame());
}

// bring up the properties dialog for this profile
void wxFolderTree::OnProperties(MFolder *folder)
{
   ProfileBase *profile = ProfileBase::CreateProfile(folder->GetName());
   CHECK_RET( profile, "can't create profile" );

   MDialog_FolderProfile(GetWindow(), profile);

   profile->DecRef();
}

MFolder *wxFolderTree::OnCreate(MFolder *parent)
{
   FAIL_MSG("not implemented");

   return NULL;
}

bool wxFolderTree::OnDelete(MFolder *folder)
{
   FAIL_MSG("not implemented");

   return FALSE;
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
      MFolder::Type          type;
   } FolderIcons[] =
   {
      { wxFolderTreeImpl::iconInbox, MFolder::Inbox },
      { wxFolderTreeImpl::iconFile,  MFolder::File  },
      { wxFolderTreeImpl::iconPOP,   MFolder::POP   },
      { wxFolderTreeImpl::iconIMAP,  MFolder::IMAP  },
      { wxFolderTreeImpl::iconNews,  MFolder::News  },
      { wxFolderTreeImpl::iconRoot,  MFolder::Root  }
   };

   MFolder::Type type = folder->GetType();
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
   if ( folder->GetType() == MFolder::Root )
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

void wxFolderTreeNode::DeleteChildren(wxTreeCtrl *tree)
{
   // delete all children
   long cookie;
   wxTreeItemId id = tree->GetFirstChild(GetId(), cookie);
   while ( id.IsOk() )
   {
      wxFolderTreeNode *child = (wxFolderTreeNode *)tree->GetItemData(id);
      child->DeleteChildren(tree);
      delete child;

      id = tree->GetNextChild(GetId(), cookie);
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
   m_menu = NULL;

   // create an image list and associate it with this control
   static const char *aszImages[] =
   {
      // should be in sync with the corresponding enum!
      "folder_inbox",
      "folder_file",
      "folder_pop",
      "folder_imap",
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
   (void)new wxFolderTreeNode(this, GetRootFolder());
}

void wxFolderTreeImpl::DoPopupMenu(const wxPoint& pos)
{
   wxFolderTreeNode *cur = GetSelection();
   if ( cur != NULL )
   {
      MFolder *folder = cur->GetFolder();
      if ( folder->GetType() == MFolder::Root )
      {
         // it's the root pseudo-folder
         MDialog_Message(_("No properties available for this item."));
      }
      else
      {
         // normal folder
         if ( m_menu == NULL )
         {
            // create our popup menu if not done yet
            m_menu = new FolderMenu(folder->GetName());
         }

         PopupMenu(m_menu, pos.x, pos.y);
      }
   }
   //else: no selection
}

void wxFolderTreeImpl::DoFolderCreate()
{
   MFolder *folderNew = m_sink->OnCreate(m_sink->GetSelection());
   if ( folderNew != NULL )
   {
      FAIL_MSG("not implemented");
   }
   //else: cancelled by user
}

void wxFolderTreeImpl::DoFolderDelete()
{
   if ( m_sink->OnDelete(m_sink->GetSelection()) )
   {
      FAIL_MSG("not implemented");
   }
}

void wxFolderTreeImpl::DoFolderRename()
{
   FAIL_MSG("folder renaming not implemented"); // @@@
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
      (void)new wxFolderTreeNode(this, folder->GetSubfolder(n), parent);
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

   MFolder *oldsel = m_current ? m_current->GetFolder() : NULL,
           *newsel = newCurrent ? newCurrent->GetFolder() : NULL;
   m_sink->OnSelectionChange(oldsel, newsel);

   m_current = newCurrent;
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

      case FolderMenu::Properties:
         DoFolderProperties();
         break;

      default:
         FAIL_MSG("unexpected menu command in wxFolderTree");
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
         // without Alt
         DoFolderOpen();
      }
      break;

    default:
      event.Skip();
  }
}

wxFolderTreeImpl::~wxFolderTreeImpl()
{
   long idRoot = GetRootItem();
   wxFolderTreeNode *root = (wxFolderTreeNode *)GetItemData(idRoot);

   root->DeleteChildren(this);
   delete root;

   delete GetImageList();

   delete m_menu;
}
