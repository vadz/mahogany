/*-*- c++ -*-********************************************************
 * wxMLogFrame.h : class for window printing log output             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#	pragma implementation "wxMLogFrame.h"
#endif

#include	<wxMLogFrame.h>
#include	<MApplication.h>

wxMLogFrame::wxMLogFrame(void)
   : wxMFrame("LogWindow", mApplication.TopLevelFrame())
{
   SetTitle(_("M activity log"));
   tw = NEW wxTextWindow(this);

   
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

int
wxMLogFrame::OnClose(void)
{
   mApplication.ShowConsole(false);
   return wxMFrame::OnClose();
}
