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
#endif

#include   "gui/wxMainFrame.h"

#ifdef    OS_WIN
#   define   MainFrame_xpm          "MainFrame"
#   define   tb_exit                "tb_exit"
#   define   tb_help                "tb_help"
#   define   tb_open                "tb_open"
#   define   tb_mail_compose        "tb_mail_compose"
#   define   tb_book_open           "tb_book_open"
#   define   tb_preferences         "tb_preferences"
#else   //real XPMs
#   include   "../src/icons/MainFrame.xpm"
#   include   "../src/icons/tb_exit.xpm"
#   include   "../src/icons/tb_help.xpm"
#   include   "../src/icons/tb_open.xpm"
#   include   "../src/icons/tb_mail_compose.xpm"
#   include   "../src/icons/tb_book_open.xpm"
#   include   "../src/icons/tb_preferences.xpm"
#endif  //Win/Unix

#ifdef   USE_WXWINDOWS2
// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
  EVT_SIZE    (wxMFrame::OnSize)
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

#if 0
   int w,h;
   GetClientSize(&w,&h);
   splitter = new wxSplitterWindow(this, -1, wxPoint(0, 0), wxSize(w, h));
   treeCtrl = new wxMainTreeCtrl(this,-1, wxPoint(0,0),
                                 wxSize(200,200),wxTR_HAS_BUTTONS|wxSUNKEN_BORDER); 
//     splitter->Initialize(treeCtrl);
   wxTreeItem item;
   item.m_mask = wxTREE_MASK_TEXT | wxTREE_MASK_CHILDREN | wxTREE_MASK_DATA;
   item.m_text = "root.";
   item.m_children = 1;
   long root_id = treeCtrl->InsertItem( 0, item );
#endif // 0
}

void
wxMainFrame::OnMenuCommand(int id)
{
   wxMFrame::OnMenuCommand(id);
}
