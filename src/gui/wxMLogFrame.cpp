/*-*- c++ -*-********************************************************
 * wxMLogFrame.h : class for window printing log output             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/18 17:48:45  KB
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
#   pragma implementation "wxMLogFrame.h"
#endif

#include   "Mpch.h"
#include   "Mcommon.h"

#include   "MFrame.h"
#include   "MLogFrame.h"
#include   "MApplication.h"
#include   "gui/wxMLogFrame.h"

wxMLogFrame::wxMLogFrame(void)
   : wxMFrame("LogWindow", mApplication.TopLevelFrame())
{
   SetTitle(_("M activity log"));

   #if  USE_WXWINDOWS2
      tw = GLOBAL_NEW wxTextWindow(this, -1);
   #else  // wxWin 1
      tw = GLOBAL_NEW wxTextWindow(this);
   #endif // wxWin ver

   AddMenuBar();
   AddFileMenu();
   AddHelpMenu();
   // Associate the menu bar with the frame
   SetMenuBar(menuBar);

   wxFrame::Show(TRUE);
}

wxMLogFrame::~wxMLogFrame()
{
}

void
wxMLogFrame::Write(String const &str)
{
   Write(str.c_str());
}

void
wxMLogFrame::Write(const char *str)
{
   tw->WriteText((char *) str);
}

void
wxMLogFrame::OnMenuCommand(int id)
{
   switch(id)
   {
   case WXMENU_FILE_CLOSE:
      mApplication.ShowConsole(false);
   default:
      wxMFrame::OnMenuCommand(id);
   }
}

ON_CLOSE_TYPE
wxMLogFrame::OnClose(void)
{
   mApplication.ShowConsole(false);
   return wxMFrame::OnClose();
}
