/*-*- c++ -*-********************************************************
 * Mwin.h - Windows specific configuration                          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MWIN_H
#define	MWIN_H

/// separating directories in a pathname
#define	DIR_SEPARATOR	_T('\\')

/// separating directories in a search path
#define	PATH_SEPARATOR	_T(';')

/// not defined by makefile
#ifndef M_CANONICAL_HOST
#define M_CANONICAL_HOST   _T("")
#endif

#endif
