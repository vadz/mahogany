/*-*- c++ -*-********************************************************
 * guidef.h define the GUI implementation                           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/11 20:29:37  VZ
 * compiles under Windows again + option USE_WXCONFIG added
 *
 * Revision 1.3  1998/04/22 19:54:48  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:37  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:13  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef GUIDEF_H
#define GUIDEF_H

#if	USE_WXWINDOWS

///	define the class to use for implementing MFrame objects
#	define	MFrame		wxMFrame
#	define	FolderView	wxFolderView
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
#	define	Uses_wxMessage
#	define	Uses_wxButton
#	define	Uses_wxDialogBox
#	define	Uses_wxDialog
#	include	<wx/wx.h>
#	include	"gui/wxMenuDefs.h"

#ifdef    __WINDOWS__
  // FIXME would be better change it in wx/wx.h
  #undef  USE_IPC
#endif

/// how much space to leave in frame around other items
#	define	WXFRAME_WIDTH_DELTA	16
/// how much space to leave in frame around other items
#	define	WXFRAME_HEIGHT_DELTA	64

#endif

#endif
