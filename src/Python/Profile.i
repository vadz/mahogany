/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
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

class ProfileBase
{
public:
    String readEntry(const char *szKey, const char *szDefault = NULL);
    bool writeEntry(const char *szKey, const char *szValue);
};
