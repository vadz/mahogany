/*-*- c++ -*-********************************************************
 * wxMainFrame.h: a basic window class                              *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef  WXMAINFRAME_H
#define  WXMAINFRAME_H

#include "MMainFrame.h"
#include "gui/wxMenuDefs.h"
#include "gui/wxMFrame.h"

#ifdef __GNUG__
#   pragma interface "wxMainFrame.h"
#endif

class MFolder;
class wxFolderView;
class wxFolderTree;
class wxSplitterWindow;

class wxMainFrame : public wxMFrame
{
public:
   /// constructor & dtor
   wxMainFrame(const String &iname = String("wxMainFrame"),
               wxFrame *parent = NULL);

   virtual ~wxMainFrame();

   // ask user whether he really wants to exit
   virtual bool CanClose() const;

   // open the given folder in the integrated folder view (may be called
   // multiple times)
   void OpenFolder(MFolder *folder);

   // close the given folder if it is opened
   void CloseFolder(MFolder *folder);

   // add the folder menu to the menu bar
   void AddFolderMenu(void);

   // wxWindows callbacks
   void OnCommandEvent(wxCommandEvent &);
   void OnAbout(wxCommandEvent &) { OnMenuCommand(WXMENU_HELP_ABOUT);}

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
protected:
   /// the splitter window holding the treectrl and folder view
   wxSplitterWindow *m_splitter;

   // splitter panes:
      /// the folder tree
   wxFolderTree *m_FolderTree;
      /// the folder view
   wxFolderView *m_FolderView;
   
   /// the name of the currently opened folder (empty if none)
   String m_folderName;

   /// the module extension menu if it is set
   class wxMenu *m_ModulesMenu;
private:
   void MakeModulesMenu(void);
   DECLARE_EVENT_TABLE()
};

#endif
