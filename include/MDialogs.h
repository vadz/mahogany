/*-*- c++ -*-********************************************************
 * MDialogs.h : GUI independent dialog boxes                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/03/26 23:05:36  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.2  1998/03/16 18:22:40  karsten
 * started integration of python, fixed bug in wxFText/word wrapping
 *
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MDIALOGS_H
#define MDIALOGS_H

#ifdef __GNUG__
#pragma interface "MDialogs.h"
#endif

/**@name Strings for default titles */
//@{
/// for error message
#define	MDIALOG_ERRTITLE	_("Error")
/// for error message
#define	MDIALOG_SYSERRTITLE	_("System Error")
/// for error message
#define	MDIALOG_FATALERRTITLE	_("Fatal Error")
/// for error message
#define	MDIALOG_MSGTITLE	_("Information")
/// for error message
#define	MDIALOG_YESNOTITLE	_("Please choose")
//@}

#ifdef USE_WXWINDOWS
#	include	"gui/wxMDialogs.h"
#endif

#endif
