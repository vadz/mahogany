///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   gui/wxMainFrame.h - main frame class declaration
// Author:      Mahogany Team
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 Mahogany Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_MAINFRAME_H_
#define _GUI_MAINFRAME_H_

#ifdef __GNUG__
#   pragma interface "wxMainFrame.h"
#endif

#ifndef USE_PCH
#  include "gui/wxMFrame.h"
#endif // USE_PCH

#include "MMainFrame.h"
#include "gui/wxMenuDefs.h"

#include "MEvent.h"

class MFolder;
class wxFolderView;
class wxFolderTree;

class GlobalSearchData;

class WXDLLEXPORT wxMenu;
class WXDLLEXPORT wxSplitterWindow;

class wxMainFrame : public wxMFrame, public MEventReceiver
{
public:
   /// constructor & dtor
   wxMainFrame(const String &iname = String(_T("wxMainFrame")),
               wxFrame *parent = NULL);

   virtual ~wxMainFrame();

   // ask user whether he really wants to exit
   virtual bool CanClose() const;

   // open the given folder in the integrated folder view (may be called
   // multiple times) in read write (default) or read only mode
   void OpenFolder(MFolder *folder, bool readonly = false);

   // close the given folder if it is opened
   void CloseFolder(MFolder *folder);

   // add the folder menu to the menu bar
   void AddFolderMenu(void);

   // update the UI of the folder menu depending on the current folder
   void UpdateFolderMenuUI(MFolder *folderSelected);

   // wxWindows callbacks
   void OnCommandEvent(wxCommandEvent &event);
   void OnIdentChange(wxCommandEvent &event);
   void OnIdle(wxIdleEvent &event);
   void OnUpdateUIEnableIfHasPreview(wxUpdateUIEvent& event);
   void OnAbout(wxCommandEvent &) { OnMenuCommand(WXMENU_HELP_ABOUT);}

   /// Mahogany event processing
   virtual bool OnMEvent(MEventData& event);

   /// Appends the menu for a module to the menubar
   virtual void AddModulesMenu(const char *name,
                               const char *help,
                               class wxMenu *submenu,
                               int id = -1);

   /// Appends the menu entry for a module to the modules menu
   virtual void AddModulesMenu(const char *name,
                               const char *help,
                               int id);

   /// Returns the name of the currently open folder:
   wxString GetFolderName(void) const { return m_folderName; }

   /// Return the profile to use for the composer started from this frame
   virtual Profile *GetFolderProfile(void) const;

   /// "private" method - for wxMainFolderView use only
   void ClearFolderName() { m_folderName.clear(); }

   /// semi-private accessor: for wxMainFolderView only
   wxFolderTree *GetFolderTree() const { return m_FolderTree; }

protected:
   /// handler for the "Folder|Search" menu command
   void DoFolderSearch();

   /// the splitter window holding the treectrl and folder view
   wxSplitterWindow *m_splitter;

   /// the folder tree
   wxFolderTree *m_FolderTree;

   /// the folder view
   wxFolderView *m_FolderView;

   /// the name of the currently opened folder (empty if none)
   String m_folderName;

   /// the module extension menu if it is set
   wxMenu *m_ModulesMenu;

   /// the async search data
   GlobalSearchData *m_searchData;

   /// the MEventManager cookie for ASFolder events
   void *m_cookieASMf;

private:
   /// create and initialize the modules menu
   void MakeModulesMenu(void);

   /// initialize m_searchData and m_cookieASMf if necessary
   void InitSearchData();

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMainFrame)
};

#endif // _GUI_MAINFRAME_H_

