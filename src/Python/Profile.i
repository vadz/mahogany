/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/04/29 19:55:43  KB
 * some more scripting and configure changes for python
 *
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/
%module Profile

%{
#include	"Mpch.h"
#include	"Mcommon.h"
#include	"Profile.h"
#if       !USE_PCH
#	include	<strutil.h>
#endif

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

%}

class Profile
{
public:
    const char *readEntry(const char *szKey, const char *szDefault = NULL);
    bool writeEntry(const char *szKey, const char *szValue);
};
