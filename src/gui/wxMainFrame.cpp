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
#include   "Mcommon.h"

#ifndef   USE_PCH
#   include   "guidef.h"
#   include   "gui/wxMDialogs.h"
#endif

#include   "MFrame.h"
#include   "Mdefaults.h"
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
BEGIN_EVENT_TABLE(wxMainTreeCtrl, wxTreeCtrl)
/*
  EVT_TREE_BEGIN_DRAG(TREE_CTRL, MyTreeCtrl::OnBeginDrag)
  EVT_TREE_BEGIN_RDRAG(TREE_CTRL, MyTreeCtrl::OnBeginRDrag)
  EVT_TREE_BEGIN_LABEL_EDIT(TREE_CTRL, MyTreeCtrl::OnBeginLabelEdit)
  EVT_TREE_END_LABEL_EDIT(TREE_CTRL, MyTreeCtrl::OnEndLabelEdit)
  EVT_TREE_DELETE_ITEM(TREE_CTRL, MyTreeCtrl::OnDeleteItem)
  EVT_TREE_GET_INFO(TREE_CTRL, MyTreeCtrl::OnGetInfo)
  EVT_TREE_SET_INFO(TREE_CTRL, MyTreeCtrl::OnSetInfo)
  EVT_TREE_ITEM_EXPANDED(TREE_CTRL, MyTreeCtrl::OnItemExpanded)
  EVT_TREE_ITEM_EXPANDING(TREE_CTRL, MyTreeCtrl::OnItemExpanding)
  EVT_TREE_SEL_CHANGED(TREE_CTRL, MyTreeCtrl::OnSelChanged)
  EVT_TREE_SEL_CHANGING(TREE_CTRL, MyTreeCtrl::OnSelChanging)
  EVT_TREE_KEY_DOWN(TREE_CTRL, MyTreeCtrl::OnKeyDown)
*/
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxMainFrame, wxMFrame)
   EVT_MENU(WXMENU_FILE_OPEN,    wxMFrame::OnOpen)
   EVT_MENU(WXMENU_FILE_ADBEDIT, wxMFrame::OnAdbEdit)
   EVT_MENU(WXMENU_FILE_CLOSE,   wxMFrame::OnMenuClose)
   EVT_MENU(WXMENU_FILE_COMPOSE, wxMFrame::OnCompose)
   EVT_MENU(WXMENU_FILE_EXIT,    wxMFrame::OnExit)
   EVT_MENU(WXMENU_HELP_ABOUT,   wxMainFrame::OnAbout)
END_EVENT_TABLE()
#endif // wxWin2

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
  SetIcon(GLOBAL_NEW wxIcon(MainFrame_xpm));

  AddFileMenu();
  AddHelpMenu();
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
