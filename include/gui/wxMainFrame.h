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

   // callbacks
   void OnCommandEvent(wxCommandEvent &);
   void OnCloseWindow(wxCloseEvent &);
   void OnAbout(wxCommandEvent &) { OnMenuCommand(WXMENU_HELP_ABOUT);}
//   void OnSize( wxSizeEvent &event );

private:
   /// the splitter window holding the treectrl and folder view
   wxSplitterWindow *m_splitter;

   // splitter panes:
      /// the folder tree
   wxFolderTree *m_FolderTree;
      /// the folder view
   wxFolderView *m_FolderView;

   DECLARE_EVENT_TABLE()
};

#endif
