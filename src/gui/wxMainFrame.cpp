/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/18 17:48:46  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.3  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
 *
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma   implementation "wxMainFrame.h"
#endif

#include   "Mpch.h"
#include   "Mcommon.h"

#if   !USE_PCH
#   include   <guidef.h>
#   include   <gui/wxMDialogs.h>
#endif

#include   "MFrame.h"
#include   "MLogFrame.h"

#include   "Mdefaults.h"

#include   "PathFinder.h"
#include   "MimeList.h"
#include   "MimeTypes.h"
#include   "Profile.h"

#include  "MApplication.h"

#include  "FolderView.h"
#include   "MailFolder.h"
#include   "MailFolderCC.h"

#include   "gui/wxMainFrame.h"
#include   "gui/wxFolderView.h"
#include   "gui/wxComposeView.h"

#ifdef    OS_WIN
#   define MainFrame_xpm   "MainFrame"
#else   //real XPMs
#   include   "../src/icons/MainFrame.xpm"
#endif  //Win/Unix

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
  SetIcon(GLOBAL_NEW wxIcon(MainFrame_xpm));

  AddMenuBar();
  AddFileMenu();
  AddHelpMenu();
  SetMenuBar(menuBar);
  #if  USE_WXWINDOWS2
     CreateStatusBar();
  #else   // wxWin1
     CreateStatusLine(1);
  #endif  // wxWin ver
}

void
wxMainFrame::OnMenuCommand(int id)
{
   wxMFrame::OnMenuCommand(id);
}
