/*-*- c++ -*-********************************************************
 * Mconfig.h: configuration for M                                   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/
#ifndef MCONFIG_H
#define	MCONFIG_H

#include	"config.h"

#undef	OS_UNIX
#undef	OS_WIN
#undef	OS_TYPE
#undef	CC_GCC
#undef	CC_MSC

#ifdef unix
#	define	OS_UNIX		1
#	define	OS_TYPE		"unix"
#elif defined(__WIN__) || defined (__WIN32__)
#	define	OS_WIN		1
#	define	OS_TYPE		"windows"
# 	ifndef  __WINDOWS__
#   		define  __WINDOWS__     // for wxWindows 2.x
# 	endif
#else
  // this reminder is important, it won't compile without it anyhow...
# error   "Unknown platform (forgot to #define unix?)"
#endif

#ifdef	__WINDOWS__
#error windows
#endif

// Are we using GCC?
#ifdef	__GNUG__
#	undef	CC_GCC	// might already be defined thanks to configure
#	define	CC_GCC	1
        /// gcc does not support precompiled headers
#	ifdef USE_PCH
#		if USE_PCH
#			undef	USE_PCH
#			define	USE_PCH	1	// use the Mpch.h anyway
#		else
#			undef	USE_PCH
#		endif
#	endif
#endif

// Are we using Microsoft Visual C++ ?
#ifdef	_MSC_VER 
#		define	CC_MSC	1
                /// are we using precompiled headers?
#		ifndef USE_PCH
# 			define USE_PCH        1
#		else
#			undef	USE_PCH
#		endif
#endif

#ifdef  USE_WXWINDOWS2
#	define wxTextWindow  wxTextCtrl
#	define wxText        wxTextCtrl
#endif  // wxWin 2

/// use one common base class
#define	USE_COMMONBASE		1

/// do some memory allocation debugging
#define	USE_MEMDEBUG		1

/// derive common base from wxObject
#define	USE_WXOBJECT		0

/// debug allocator
#define	USE_DEBUGNEW		0


#if USE_DEBUGNEW
#	define	GLOBAL_NEW	  WXDEBUG_NEW
#	define	GLOBAL_DELETE	delete
#else
#	define	GLOBAL_NEW	  new
#	define	GLOBAL_DELETE	delete
#endif

/// use simple dynamic class information
#define	USE_CLASSINFO		1

/// name of the application
#define	M_APPLICATIONNAME	"M"


#if	USE_BASECLASS
#	define	BASECLASS	CommonBase
#endif

#ifdef	HAVE_COMPFACE_H
#	define	HAVE_XFACES
#endif

#ifdef	HAVE_COMPFACE_H
#	define	HAVE_XFACES
#endif

#define	M_STRBUFLEN		1024

#if         USE_WXSTRING
# if        !USE_WXWINDOWS2
#               define  c_str() GetData()
#               define  length() Length()       //FIXME dangerous!
#       endif
#       define  String      wxString
#	include	<wx/string.h>
#else
#       include <string>
  typedef std::string String;
#endif


/// make sure NULL is define properly
#undef  NULL
#define NULL    0


// Microsoft Visual C++
#ifdef  CC_MSC
  // suppress the warning "identifier was truncated to 255 characters 
  // in the debug information"
#	pragma warning(disable: 4786)

  // you can't mix iostream.h and iostream, the former doesn't compile
  // with "using namespace std", the latter doesn't compile with wxWin
  // make your choice...
#	ifndef USE_IOSTREAMH
#		define USE_IOSTREAMH   1
#	endif

  // <string> includes <istream> (Grrr...)
#	if     USE_IOSTREAMH
#		undef  USE_WXSTRING
#		define USE_WXSTRING    1
#	endif
#endif  // VC++

#if           USE_IOSTREAMH
#       include <iostream.h>
#       include <fstream.h>
#else
#       include <iostream>
#       include <fstream>
#endif

#include        <list>
#include        <map>

#if           USE_IOSTREAMH
  // can't use namespace std because old iostream doesn't compile with it
  // and can't use std::list because it's a template class
#else
  using namespace std;
#endif

#ifdef	USE_WXWINDOWS
#       ifdef        USE_WXWINDOWS2
#               define  WXCPTR  	/**/
#		define	WXSTR(str)	str
#       else
#               define  WXCPTR  	(char *)
#    		define	WXSTR(str)	((char *)str.c_str())
#       endif
#endif


// set the proper STL class names
#if	CC_MSC
#	define	STL_LIST	std::list
#else
#	define	STL_LIST	list
#endif


#define Bool    int

#endif
