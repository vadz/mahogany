/*-*- c++ -*-********************************************************
 * MDialogs.h : GUI independent dialog boxes                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MDIALOGS_H
#define MDIALOGS_H

#ifdef __GNUG__
#pragma interface "MDialogs.h"
#endif

#include	<MFrame.h>
#include	<MApplication.h>
#include	<guidef.h>

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

#if USE_WXWINDOWS
#	include	<wxMDialogs.h>
#endif

#endif
