/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
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
  EVT_SIZE    (wxMFrame::OnSize)
  EVT_MENU(-1,    wxMFrame::OnCommandEvent)
  EVT_TOOL(-1,    wxMFrame::OnCommandEvent)
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
   AddHelpMenu();
   SetMenuBar(menuBar);

#ifdef USE_WXWINDOWS2
   m_ToolBar = new wxMToolBar( this, /*id*/-1, wxPoint(2,60), wxSize(width-4,26) );
   m_ToolBar->SetMargins( 2, 2 );
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_open"), WXMENU_FILE_OPEN, "Open Folder");
   TB_AddTool(m_ToolBar, ICON("tb_mail_compose"), WXMENU_FILE_COMPOSE, "Compose Message");
   TB_AddTool(m_ToolBar, ICON("tb_book_open"), WXMENU_EDIT_ADB, "Edit Database");
   TB_AddTool(m_ToolBar, ICON("tb_preferences"), WXMENU_EDIT_PREFERENCES, "Edit Preferences");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_help"), WXMENU_HELP_ABOUT, "Help");
   m_ToolBar->AddSeparator();
   TB_AddTool(m_ToolBar, ICON("tb_exit"), WXMENU_FILE_EXIT, "Exit M");

#	ifdef OS_WIN
		m_ToolBar->CreateTools();
#	endif // Windows
#endif // wxWin 2
   

#ifndef  USE_WXWINDOWS2
   CreateStatusLine(1);
   return;
#endif
   CreateStatusBar();

   int x,y;
   GetClientSize(&x, &y);

   m_FolderView = NULL;
   m_splitter = new wxSplitterWindow(this,-1,wxPosition(1,31),wxSize(x-1,y-31),wxSP_3D);
   const char *foldername = READ_APPCONFIG(MC_MAINFOLDER);
  //FIXME: insert treectrl here
   if(! strutil_isempty(foldername))
   {
      m_FolderView = new wxFolderView(foldername,this);
      m_splitter->SplitVertically(new wxPanel(this), //FIXME: insert treectrl
                                  m_FolderView->GetWindow(),x/3);
   }
   else
      m_splitter->Initialize(new wxPanel(this));  //FIXME: insert treectrl

   m_splitter->SetMinimumPaneSize(0);
   m_splitter->SetFocus();
}

void
wxMainFrame::OnMenuCommand(int id)
{
   wxMFrame::OnMenuCommand(id);
}

void
wxMainFrame::OnSize( wxSizeEvent &event )
   
{
   int x = 0;
   int y = 0;
   GetClientSize( &x, &y );
   if(m_ToolBar)  m_ToolBar->SetSize( 1, 0, x-2, 30 );
   if(m_splitter) m_splitter->SetSize(0,31,x-2,y);
};
