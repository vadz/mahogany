/*-*- c++ -*-********************************************************
 * wxMainFrame.h: a basic window class                              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$            *
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

   // wxWindows callbacks
   void OnCommandEvent(wxCommandEvent &);
   void OnAbout(wxCommandEvent &) { OnMenuCommand(WXMENU_HELP_ABOUT);}
protected:
   /// the splitter window holding the treectrl and folder view
   wxSplitterWindow *m_splitter;

   // splitter panes:
      /// the folder tree
   wxFolderTree *m_FolderTree;
      /// the folder view
   wxFolderView *m_FolderView;
   /// the popup menu for modules
   wxMenu *m_ModulePopup;
   
   /// the name of the currently opened folder (empty if none)
   String m_folderName;

private:
   DECLARE_EVENT_TABLE()
};

#endif
