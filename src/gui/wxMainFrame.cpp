/*-*- c++ -*-********************************************************
 * wxMainFrame: the main window class                               *
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
#pragma	implementation "wxMainFrame.h"
#endif


#include	<Mcommon.h>

#include	<guidef.h>
#include	<wxMainFrame.h>
#include	<wxFolderView.h>
#include	<wxComposeView.h>
#include	<wxMDialogs.h>

#include	<MApplication.h>
#include	<MailFolderCC.h>

#include	<MainFrame.xpm>

wxMainFrame::wxMainFrame(const String &iname, wxFrame *parent)
   : wxMFrame(iname,parent)
{
   SetIcon(NEW wxIcon(MainFrame_xpm));

   AddMenuBar();
   AddFileMenu();
   AddHelpMenu();
   SetMenuBar(menuBar);
   CreateStatusLine(1);

}

void
wxMainFrame::OnMenuCommand(int id)
{
   wxMFrame::OnMenuCommand(id);
}
