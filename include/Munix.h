/*-*- c++ -*-********************************************************
 * Munix.h - unix specific configuration                            *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MUNIX_H
#define	MUNIX_H

/// separating directories in a pathname
#define	DIR_SEPARATOR	'/'

/// separating directories in a search path
#define	PATH_SEPARATOR	':'

/// under Unix it's quite simple
#define IsAbsPath(path)    (((const wxChar *)path)[0] == '/')

#endif
