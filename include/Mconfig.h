/*-*- c++ -*-********************************************************
 * Mconfig.h: configuration for M                                   *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$               *
 *******************************************************************/

#ifndef MCONFIG_H
#define MCONFIG_H

#include  "config.h"

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
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#if defined(__WIN__) || defined(__WINDOWS__) || defined(_WIN32)
#  define  OS_WIN    1
#  define  OS_TYPE    "windows"
#  ifndef  __WINDOWS__
#       define  __WINDOWS__     // for wxWindows 2.x
#  endif
#endif

/// Test for MacOS:
#if defined( __POWERPC__ ) && ( defined( __MWERKS__ ) || defined ( THINK_C ))
#   define   OS_MAC   1
#   define   OS_TYPE      "MacOS"
#   undef    OS_SUBTYPE
#   define   OS_SUBTYPE   ""
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
#endif

/// debug allocator
#undef   USE_DEBUGNEW


#ifdef USE_DEBUGNEW
#   define   GLOBAL_NEW     WXDEBUG_NEW
#   define   GLOBAL_DELETE   delete
#else
#   define   GLOBAL_NEW     new
#   define   GLOBAL_DELETE   delete
#endif

/// use simple dynamic class information
#define   USE_CLASSINFO      1

/// name of the application, has to remain "M" for compatibility reasons
#define   M_APPLICATIONNAME   "M"
/// name of the vendor
#define   M_VENDORNAME   "Mahogany-Team"

/// name of global config file
#define   M_GLOBAL_CONFIG_NAME   "Mahogany.conf"

/// path for etc directories
#define   M_ETC_PATH   "/etc:/usr/etc:/usr/local/etc:/opt/etc:/usr/share/etc:/usr/local/share/etc:/opt/share/etc:/usr/local/stow/etc"

/// basic M installation directory
#ifdef OS_UNIX
#   define   M_BASEDIR M_PREFIX"/share/Mahogany"
#else
#   define   M_BASEDIR M_PREFIX"\\Mahogany"
#endif

#ifdef   HAVE_COMPFACE_H
#   define   HAVE_XFACES
#endif

#define   M_STRBUFLEN      1024

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
// with "using namespace std", the latter doesn't compile with wxWin
// make your choice...
#ifndef USE_IOSTREAMH
#  define USE_IOSTREAMH   1
#endif

#ifdef USE_IOSTREAMH
#  include <iostream.h>
#  include <fstream.h>
#else
#  include <iostream>
#  include <fstream>
#endif

#ifdef USE_IOSTREAMH
  // can't use namespace std because old iostream doesn't compile with it
  // and can't use std::list because it's a template class
#else
  using namespace std;
#endif

// set the proper STL class names
#ifdef  CC_MSC
# define  STL_LIST  std::list
#else //!Visual C++
# define  STL_LIST  list
#endif

#define Bool    int

// macro which marks strings for later extraction with xgettext(1): it
// intentionally does nothing else!
#ifndef gettext_noop
#   define gettext_noop(s)    s
#endif // gettext_noop

#ifdef USE_THREADS
#   if !  wxUSE_THREADS
#      error "Mahogany's thread support requires a wxWindows with threads compiled in!"
#   endif
#endif

// although this is not necessary, including this header allows to have more
// info in the logs
#ifdef USE_DMALLOC
#   include <dmalloc.h>
#endif // USE_DMALLOC

#endif // MCONFIG_H
