//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Mconfig.h: common compilation configuration settings
// Purpose:     config.h is edited by the user, this one detects the rest
//              automatically
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1999-2004 by M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MCONFIG_H
#define MCONFIG_H

#include  "config.h"
#include  <wx/defs.h>

// ----------------------------------------------------------------------------
// detect the OS
// ----------------------------------------------------------------------------

#undef   OS_SOLARIS
#undef   OS_LINUX
#undef   OS_UNIX
#undef   OS_WIN
#undef   OS_TYPE
#undef   OS_MAC
#undef   OS_SUBTYPE
#undef   CC_GCC
#undef   CC_MSC
#undef   CC_TYPE

#define   OS_SUBTYPE   "unknown"

/// Test for unix flavours:
#if ( defined(unix) || defined(__unix) || defined(__unix__) ) && ! defined(__CYGWIN__)
#  define  OS_UNIX    1
#  define  OS_TYPE    "unix"
#  undef   OS_SUBTYPE
#  ifdef linux
#     define   OS_LINUX
#     define   OS_SUBTYPE   "linux-gnu"
#  endif
#  ifdef sun
#     define   OS_SOLARIS
#     define   OS_SUBTYPE   "solaris"
#  endif
#  ifdef __osf__
#     define   OS_TRU64
#     define   OS_SUBTYPE   "tru64"
#  endif
#endif

/// Test for MS Windows:
#if defined(__WIN__) || defined(__WINDOWS__) || defined(_WIN32) || defined(__CYGWIN__)
#  define  OS_WIN    1
#  define  OS_TYPE    "windows"
#endif

/// Test for MacOS X
#if defined (__APPLE__) && defined (__MACH__)
    #define OS_UNIX   1
    #define   OS_MAC   1
    #define OS_TYPE   "unix"
    #undef OS_SUBTYPE
    #define OS_SUBTYPE   "mac-osx"
#endif


#if ! defined (OS_TYPE)
    // this reminder is important!
#   error   "Unknown platform (forgot to #define unix?)"
#endif

// ----------------------------------------------------------------------------
// detect the compiler
// ----------------------------------------------------------------------------

// Are we using GCC?
#ifdef  __GNUG__
#  undef    CC_GCC  // might already be defined thanks to configure
#  define   CC_GCC
#  define   CC_TYPE "gcc"

   // gcc gives an annoying warning
   // "class 'Foo' only defines a private destructor and has no friends"
   // even when it's a ref counted class which deletes itself.
   //
   // suppress it for all MObjectRC derived classes with this macro
#  define GCC_DTOR_WARN_OFF    friend class MObjectRC;
#else
#  define GCC_DTOR_WARN_OFF
#endif

// Are we using Microsoft Visual C++ ?
#ifdef   _MSC_VER
#   define   CC_MSC   1
#   define   CC_TYPE  "Visual C++"

    // no reason not to use precompiled headers with VC++
#   ifndef USE_PCH
#       define USE_PCH
#   endif
#endif

// MSVC defines _DEBUG in debug builds, set DEBUG accordingly
#if defined(_DEBUG) && !defined(DEBUG)
   #define DEBUG
#endif

// ----------------------------------------------------------------------------
// wxWindows stuff
// ----------------------------------------------------------------------------

#if !wxCHECK_VERSION(2, 5, 1)
#  error "This version of Mahogany requires wxWindows 2.5.1 or higher."
#endif

#ifdef USE_THREADS
#   if !wxUSE_THREADS
#      error "Mahogany's thread support requires a wxWindows with threads compiled in!"
#   endif
#endif

// make sure that wxWin and M settings are in sync: although in theory it
// should be possible to use mismatching settings, in practice this more often
// doesn't work than does, so please don't do it unless you really know what
// you're doing
#ifdef DEBUG
#   ifndef __WXDEBUG__
#      error "Please use debug version of wxWindows with debug Mahogany build."
#   endif
#else // Release
#   ifdef __WXDEBUG__
#      error "Please don't use debug version of wxWindows with non-debug Mahogany build."
#   endif
#endif // Debug/Release

// ----------------------------------------------------------------------------
// miscellaneous other stuff
// ----------------------------------------------------------------------------

/// name of the application, has to remain "M" for compatibility reasons
#define   M_APPLICATIONNAME   _T("M")
/// name of the vendor
#define   M_VENDORNAME   _T("Mahogany-Team")

/// basic M installation directory
#ifdef OS_UNIX
#   define   M_BASEDIR M_PREFIX _T("/share/Mahogany")
#else
#   define   M_BASEDIR M_PREFIX _T("\\Mahogany")
#endif

#ifdef   HAVE_COMPFACE_H
#   define   HAVE_XFACES
#endif

#ifdef USE_STD_STRING
#   include   <string>
    typedef   std::string String;
#   define    Str(str)((str).c_str())
#else
#   include   <wx/string.h>
    typedef   wxString String;
#   define    Str(str) str
#endif

// you can't mix iostream.h and iostream, the former doesn't compile
// with "using namespace std", the latter doesn't compile with older
// compilers.

#if wxUSE_IOSTREAMH
#  include <iostream.h>
#  include <fstream.h>
#else
#  include <iostream>
#  include <fstream>
   using namespace std;
#endif

// set the proper STL class names
#ifdef  CC_MSC
# define  STL_LIST  std::list
#else //!Visual C++
# define  STL_LIST  list
#endif

// although this is not necessary, including this header allows to have more
// info in the logs
#ifdef USE_DMALLOC
#   include <dmalloc.h>
#endif // USE_DMALLOC

#endif // MCONFIG_H

