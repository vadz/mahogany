/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma   implementation "wxMainFrame.h"
#endif

#include   "Mpch.h"

#ifndef   USE_PCH
#   include   "Mcommon.h"
#   include   "guidef.h"
#   include   "MFrame.h"
#   include   "Mdefaults.h"
#   include   "strutil.h"
#endif

#include   "gui/wxMainFrame.h"
#include   "gui/wxMApp.h"
#include   "gui/wxIconManager.h"
#include   "gui/wxFolderView.h"

#include   "MDialogs.h"

#ifdef   USE_WXWINDOWS2
// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
  EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
  EVT_TOOL(-1,    wxMainFrame::OnCommandEvent)
  EVT_CLOSE(wxMainFrame::OnCloseWindow)
END_EVENT_TABLE()
#endif // wxWin2

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
   SetIcon(ICON("MainFrame"));

   AddFileMenu();
   AddEditMenu();

#ifndef  USE_WXWINDOWS2
   CreateStatusLine(1);
   return;
#endif
   CreateStatusBar();

   int x,y;
   GetClientSize(&x, &y);

   m_FolderView = NULL;
   m_splitter = new wxSplitterWindow(this,-1,wxPoint(1,31),wxSize(x-1,y-31),wxSP_3D|wxSP_BORDER);

   String foldername = READ_APPCONFIG(MC_MAINFOLDER);

   //FIXME: insert treectrl here
   if(! strutil_isempty(foldername))
   {
      m_FolderView = new wxFolderView(foldername, m_splitter);
   }

   if ( m_FolderView && m_FolderView->IsInitialised() )
   {
      //m_splitter->SplitVertically(new wxPanel(m_splitter), //FIXME: insert treectrl
      //                            m_FolderView->GetWindow(),x/3);
      // FIXME for now:
      m_splitter->Initialize(m_FolderView->GetWindow());
      AddMessageMenu();
   }
   else {
      m_splitter->Initialize(new wxPanel(m_splitter));  //FIXME: insert treectrl

      delete m_FolderView; // may be NULL
   }

   AddHelpMenu();
   SetMenuBar(m_MenuBar);

   m_ToolBar = CreateToolBar();
   AddToolbarButtons(m_ToolBar, WXFRAME_MAIN);
   
   m_splitter->SetMinimumPaneSize(0);
   m_splitter->SetFocus();
}

void
wxMainFrame::OnCloseWindow(wxCloseEvent&)
{
   // ask the user unless disabled
   if ( READ_APPCONFIG(MC_CONFIRMEXIT) == 0 || 
        MDialog_YesNoDialog(_("Really exit M?")) ) {
      delete m_FolderView;

      Destroy();
   }
}

void
wxMainFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   
   if(m_FolderView)
   {
      if( WXMENU_CONTAINS(MSG, id) || WXMENU_CONTAINS(LAYOUT, id) )
         m_FolderView->OnCommandEvent(event);
#if 0 // VZ: why? (FIXME)
      else if(id == WXMENU_EDIT_PREF)
         MDialog_FolderProfile(this, m_FolderView->GetProfile());
#endif
      else
         wxMFrame::OnMenuCommand(id);
   }
   else
      wxMFrame::OnMenuCommand(id);
}

