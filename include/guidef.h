/*-*- c++ -*-********************************************************
 * guidef.h define the GUI implementation                           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:13  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef GUIDEF_H
#define GUIDEF_H

#include	<Mcommon.h>

#if	USE_WXWINDOWS

///	define the class to use for implementing MFrame objects
#	define	MFrame		wxMFrame
#	define	FolderView 	wxFolderView
#	define	MainFrame	wxMainFrame
#	define	MDialogs	wxMDialogs


#	define	Uses_wxFrame
#	define	Uses_wxMenu
#	define	Uses_wxString
#	define	Uses_wxCanvas
#	define	Uses_wxTextWindow
#	define	Uses_wxFont
#	define	Uses_wxMenu
#	define	Uses_wxMenuBar
#	define	Uses_wxListBox
#	define	Uses_wxPostScriptDC
#	define	Uses_wxPrintSetup
#	define	Uses_wxTextWindow
#	define	Uses_wxTimer
#	define		Uses_wxMessage
#	include	<wx/wx.h>
#	include	<wxMenuDefs.h>
/// how much space to leave in frame around other items
#	define	WXFRAME_WIDTH_DELTA	16
/// how much space to leave in frame around other items
#	define	WXFRAME_HEIGHT_DELTA	64

#endif

#endif
