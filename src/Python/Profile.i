/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/08/26 00:32:05  VZ
 * my final Windows rel. 0.01 sources merged with the recent changes (no warranty
 * about their compilability under Unix, sorry...)
 *
 * Revision 1.2  1998/08/22 23:19:27  VZ
 *
 * many, many changes (I finally had to correct all the small things I was delaying
 * before). Among more or less major changes:
 *  1) as splash screen now looks more or less decently, show it on startup
 *  2) we have an options dialog (not functional yet, though)
 *  3) MApplication has OnShutDown() function which is the same to OnStartup() as
 *     OnExit() to OnInit()
 *  4) confirmation before closing compose window
 * &c...
 *
 * Revision 1.1  1998/04/29 19:55:43  KB
 * some more scripting and configure changes for python
 *
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/
%module MProfile

%{
#include  "Mcommon.h"
#include  "kbList.h"
#include  "Profile.h"
#include  "strutil.h"

#include  "MFrame.h"
#include  "guidef.h"
#include  "gui/wxMFrame.h"

#include  "Mdefaults.h"

#include  "PathFinder.h"
#include  "MimeList.h"
#include  "MimeTypes.h"
#include  "Profile.h"

#include  "MApplication.h"

// we don't want to export our functions as we don't build a shared library
#undef SWIGEXPORT
#define SWIGEXPORT(a,b) a b
%}

class Profile
{
public:
    String readEntry(const char *szKey, const char *szDefault = NULL);
    bool writeEntry(const char *szKey, const char *szValue);
};
