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

extern "C"
{
#  include <mail.h>
#  include <osdep.h>
#  include <rfc822.h>
#  include <smtp.h>
#  include <nntp.h>

   // windows.h included from osdep.h #defines all these
#  undef   DrawText
#  undef   GetMessage
#  undef   FindWindow
#  undef   GetCharWidth
#  undef   LoadAccelerators
#  undef   GetClassInfo
#  undef   StartDoc
}

#include <wx/config.h>
#include <wx/help.h>

#include "kbList.h"
#include "guidef.h"

#include "strutil.h"
#include "sysutil.h"

#include "Mcommon.h"
#include "Merror.h"

#include "PathFinder.h"

#include "MFrame.h"
#include "gui/wxMFrame.h"

#include "Profile.h"

#include "FolderType.h"

#include "MApplication.h"
#include "gui/wxMApp.h"
#include "gui/wxMFrame.h"
#include "gui/wxIconManager.h"

#include "MHelp.h"
#include "gui/wxMIds.h"

#endif  //USE_PCH

#endif  //MPCH_H
