/*-*- c++ -*-********************************************************
 * wxMainFrame.h: a basic window class                              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$       *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/05/18 17:48:26  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMAINFRAME_H
#define WXMAINFRAME_H

#include	<MMainFrame.h>
#include	<gui/wxMenuDefs.h>
#include	<gui/wxMFrame.h>

#ifdef __GNUG__
#pragma interface "wxMainFrame.h"
#endif

class wxMainFrame : public wxMFrame
{
public:
   /// Constructor
   wxMainFrame(const String &iname = String("wxMainFrame"),
	       wxFrame *parent = NULL);
   
   void OnMenuCommand(int id);
};

#endif
