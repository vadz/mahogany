/*-*- c++ -*-********************************************************
 * Mconfig.h: configuration for M                                   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef MCONFIG_H
#define	MCONFIG_H

#include	<config.h>

#ifdef unix
#	define	OS_UNIX		1
#	define	OS_TYPE		"unix"
#endif
#if defined(__WIN__) || defined (__WIN32__)
#	define	OS_WIN		1
#	define	OS_TYPE		"windows"
#endif


/// use one common base class
#define	USE_COMMONBASE		1

/// do some memory allocation debugging
#define	USE_MEMDEBUG		1

/// derive common base from wxObject
#define	USE_WXOBJECT		0

/// debug allocator
#define	USE_DEBUGNEW		0

#if USE_DEBUGNEW
#	define	NEW	WXDEBUG_NEW
#	define	DELETE	delete
#else
#	define	NEW	new
#	define	DELETE	delete
#endif

/// use simple dynamic class information
#define	USE_CLASSINFO		1

/// name of the application
#define	M_APPLICATIONNAME	"M"


#if	USE_BASECLASS
#	define	BASECLASS	CommonBase
#endif

#define	M_STRBUFLEN		1024

#endif
