// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M
// File name:   gui/wxFolderTree.h - a specialized tree control for folders
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.10.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   _WXFOLDERTREE_H
#define   _WXFOLDERTREE_H

#ifndef USE_PCH
#  include <wx/window.h>
#endif // USE_PCH

// fwd declarations
class wxFolderTreeImpl;
class MFolder;
class wxMainFrame;

class WXDLLEXPORT wxMenu;
class WXDLLEXPORT wxPoint;
class WXDLLEXPORT wxSize;

// abstraction of the folder tree control for M usage
class wxFolderTree
{
public:
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
      iconOutbox,
      iconTrash,
      iconFolderMax
   };
   // ctor(s) and dtor
      // Init() must be called if you use default ctor
   wxFolderTree() { m_tree = NULL; }
      // normal ctor
   wxFolderTree(wxWindow *parent, wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize)
      { Init(parent, id, pos, size); }
      // the function which really initializes this object
   void Init(wxWindow *parent, wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize);
      // dtor
   virtual ~wxFolderTree();

   // operations
      // select the tree node specified by the given folder, returns FALSE if
      // the folder is not in the tree
   bool SelectFolder(MFolder *folder);
      // find the next/previous folder with unread messages, return NULL if
      // there are no more
   MFolder *FindNextUnreadFolder(bool next = TRUE);

      // process one of WXMENU_FOLDER_XXX commands
   void ProcessMenuCommand(int id);
      // update the menu items state (NB: doesn't inc/dec ref the pointer)
   void UpdateMenu(wxMenu *menu, const MFolder *folder);

   // high level events (should be implemented in derived classes)
      // parameters are the previously selected folder and the new selection
   virtual void OnSelectionChange(MFolder *oldsel, MFolder *newsel);
      // open folder in the same view
   virtual void OnOpenHere(MFolder *folder);
      // open folder in a new view
   virtual void OnOpen(MFolder *folder);
      // view folder (always in the same view for now)
   virtual void OnView(MFolder *folder);
      // "browse subfolders" selected from the popup menu
   virtual void OnBrowseSubfolders(MFolder *folder);
      // folder properties requested
   virtual void OnProperties(MFolder *folder);
      // user wants to add a new folder under the current one (return the
      // pointer to new folder or NULL to cancel the operation)
   virtual MFolder *OnCreate(MFolder *parent);
      // user wants to delete this folder, return TRUE to allow this
   virtual bool OnDelete(MFolder *folder, bool removeOnly);
      // user wants to rename this folder to folderNewName, return TRUE if ok
   virtual bool OnRename(MFolder *folder,
                         const String& folderNewName,
                         const String& mboxNewName = wxEmptyString);
      // user wants to move this folder to a new parent, return TRUE if ok
   virtual bool OnMove(MFolder *folder,
                       MFolder *newParent);
      // user wants to remove all messages from this folder
   virtual void OnClear(MFolder *folder);
      // the folder must be closed, return FALSE to prevent it from closing
   virtual bool OnClose(MFolder *folder);
      // update the status of the folder shown in the tree
   virtual void OnUpdate(MFolder *folder);

   // low level events (have reasonable default implementation in base class)
      // folder double clicked/enter pressed
   virtual bool OnDoubleClick();

   // accessors
      // associated window object (for showing/hiding/resizing...)
   wxWindow *GetWindow() const;
      // don't froget to call DecRef() on the returned pointer
   MFolder  *GetSelection() const;

private:
   wxFolderTreeImpl *m_tree;
};

#endif  //_WXFOLDERTREE_H
