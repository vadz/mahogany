/*-*- c++ -*-********************************************************
 * Mpch.h - precompiled header support for M                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *                                                                  *
 * This reduces to Mconfig.h when no  precompiled headers are used. *
 *******************************************************************/

#ifndef   MPCH_H
#define   MPCH_H

#include  "Mconfig.h"

#ifdef    USE_PCH

// includes for c-client library

#include  <stdio.h>
#include  <time.h>

#ifdef   OS_WIN
#  undef USE_IPC  // it's in conflict with a standard Windows constant
#endif   //Windows

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/help.h>

#include "Mcclient.h"

#include "Mcommon.h"

#include "guidef.h"

#include "strutil.h"
#include "sysutil.h"

#include "Merror.h"

#include "Mdefaults.h"

#include "MFrame.h"
#include "gui/wxMFrame.h"

#include "Profile.h"

#include "FolderType.h"
#include "Sorting.h"
#include "Threading.h"

#include "MApplication.h"
#include "gui/wxMApp.h"
#include "gui/wxIconManager.h"

#include "MHelp.h"
#include "gui/wxMIds.h"

#if defined(DEBUG) && defined(_MSC_VER)
   // Redefine global operator new to use the overloaded version which records
   // the allocation location in the source, this makes finding the leaks
   // reported by MSVC CRT much easier.
   //
   // Notice that this must be done after including the standard headers which
   // sometimes redefine operator new themselves and, worse, map and set can't
   // be included after doing this as it redefines some of the identifiers used
   // in them, so include them from here proactively.
   #include <map>
   #include <set>

   #include <wx/msw/msvcrt.h>
#endif

#endif  //USE_PCH

#endif  //MPCH_H
