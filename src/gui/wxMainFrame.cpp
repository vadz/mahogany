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
#   define MainFrame_xpm   "MainFrame"
#else   //real XPMs
#   include   "../src/icons/MainFrame.xpm"
#endif  //Win/Unix

#ifdef   USE_WXWINDOWS2
// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
   EVT_MENU(-1,    wxMainFrame::OnCommandEvent)
END_EVENT_TABLE()
#endif // wxWin2

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
  SetIcon(GLOBAL_NEW wxIcon(MainFrame_xpm));

  AddFileMenu();
  AddHelpMenu();
  SetMenuBar(menuBar);
  
#ifndef  USE_WXWINDOWS2
  CreateStatusLine(1);
  return;
#endif
     CreateStatusBar();
#if 0
     int w,h;
     GetClientSize(&w,&h);
//     splitter = new wxSplitterWindow(this, -1, wxPoint(0, 0), wxSize(w, h));
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
