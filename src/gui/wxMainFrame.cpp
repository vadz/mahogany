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
   int width, height;
   
   GetClientSize(&width,&height);
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
   const char *foldername = READ_APPCONFIG(MC_MAINFOLDER);
  //FIXME: insert treectrl here
   if(! strutil_isempty(foldername))
   {
      m_FolderView = new wxFolderView(foldername,m_splitter);
      //m_splitter->SplitVertically(new wxPanel(m_splitter), //FIXME: insert treectrl
      //                            m_FolderView->GetWindow(),x/3);
      // FIXME for now:
      m_splitter->Initialize(m_FolderView->GetWindow());
      AddMessageMenu();
   }
   else
      m_splitter->Initialize(new wxPanel(m_splitter));  //FIXME: insert treectrl

   AddHelpMenu();
   SetMenuBar(m_MenuBar);

#ifdef USE_WXWINDOWS2
   
   m_ToolBar = CreateToolBar();
   m_ToolBar->SetSize( -1, -1, width-2, 30 );
   m_ToolBar->SetMargins( 2, 2 );
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, "tb_open", WXMENU_FILE_OPEN, "Open Folder");
   TB_AddTool(m_ToolBar, "tb_mail_compose", WXMENU_FILE_COMPOSE, "Compose Message");
   TB_AddTool(m_ToolBar, "tb_book_open", WXMENU_EDIT_ADB, "Edit Database");
   TB_AddTool(m_ToolBar, "tb_preferences", WXMENU_EDIT_PREF, "Edit Preferences");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, "tb_help", WXMENU_HELP_ABOUT, "Help");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, "tb_exit", WXMENU_FILE_EXIT, "Exit M");

#	ifdef OS_WIN
		m_ToolBar->CreateTools();
#	endif // Windows
#endif // wxWin 2
   
   m_splitter->SetMinimumPaneSize(0);
   m_splitter->SetFocus();
}

void
wxMainFrame::OnCloseWindow(wxCloseEvent&)
{
   // FIXME: ask user if he really wants to exit...
   delete m_FolderView;

   Destroy();
}

void
wxMainFrame::OnMenuCommand(int id)
{
   wxMFrame::OnMenuCommand(id);
}

void
wxMainFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   if(WXMENU_CONTAINS(MSG,id) || id == WXMENU_LAYOUT_CLICK)
      m_FolderView->OnCommandEvent(event);
   else
      wxMFrame::OnMenuCommand(id);
}

