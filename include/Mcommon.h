/*-*- c++ -*-********************************************************
 * Mcommon.h: common typedefs etc for M                             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 *******************************************************************/
#ifndef MCOMMON_H
#define	MCOMMON_H

/// make sure NULL is define properly
#undef	NULL
#define	NULL	0

#include	<iostream.h>

#include	<Mconfig.h>

#ifdef	USE_WXWINDOWS
#	ifdef	WXWINDOWS2
#		define	WXCPTR	/**/
#	else
#		define	WXCPTR	(char *)
#	endif
#endif

#ifdef	USE_WXSTRING
#	include	<wx/wxstring.h>
#	define	String wxString
#	ifndef WXWINDOWS2
#		define	c_str()	GetData()
#		define  length() Length()	//FIXME dangerous!
#	endif
#else
#	include	<string>
        typedef class string String;
#endif

#define	Bool	int

#ifdef	HAVE_LIBINTL_H
#	define 	USE_GETTEXT	1
#	include	<libintl.h>
#else
#	define	USE_GETTEXT	0
#endif

#define	_(string)	mApplication.GetText(string)


#ifndef NDEBUG
/// macro to define debug method
#	define DEBUG_DEF		void Debug(void) const;
#	define	DBGLOG(x)		cerr << x << endl;
#else
#	define	DEBUG_DEF
#	define	DBGLOG(x)		
#endif

#define	LogMsg(x) 	cerr << x << endl;

/// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#define	ERRORMESSAGE(arg) MDialog_ErrorMessage arg
/// variable argument macro to do system error messages, call SYSERRMESSAGE((argument))
#define	SYSERRMESSAGE(arg) MDialog_SystemErrorMessage arg
/// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#define	FATALERROR(arg) MDialog_FatalErrorMessage arg
/// variable argument macro to do information messages, call INFOMESSAGE((argument))
#define	INFOMESSAGE(arg) MDialog_Message arg
/// for logging messages
#define	LOGMESSAGE(arg)	mApplication.Log arg
/// for internal errors, core dumps or gives message for NDBEGU set
#ifdef NDEBUG
#	define	INTERNALERROR(arg) MDialog_FatalErrorMessage arg
#else
#	define	INTERNALERROR(arg) { MDialog_ErrorMessage arg ; abort(); }
#endif
#ifdef OS_UNIX
#	include	<Munix.h>
#endif

/// define log levels
enum { LOG_DEBUG = -2, LOG_NOISE, LOG_DEFAULT = 0, LOG_INFO = 0, LOG_ERROR, LOG_URGENT };

#ifdef OS_WIN
#	include <Mwin.h>
#endif


#define	ABOUTMESSAGE \
"M Copyright (C) 1998 by \n"\
"  Karsten Ballüder\n"\
"  11 (2F3) Yeaman Place\n" \
"  Edinburgh EH11 1BR, Scotland\n" \
"  (Ballueder@usa.net)\n"\
"\n"\
"For the latest news on M see\n"\
"http://Ballueder.home.ml.org/M/\n"\
"M has been written using the cross-platform\n"\
"class library wxWindows which is freely\n"\
"available under the GNU Library Public\n"\
"License. Plase see\n"\
"http://web.ukonline.co.uk/julian.smart/wxwin/\n"\
"for more information."\
"\n"


#if	USE_WXOBJECT
#	include	<guidef.h>
#endif

#endif	// MCOMMON_H
