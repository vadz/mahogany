/*-*- c++ -*-********************************************************
 * Mconfig.h: configuration for M                                   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/
#ifndef MCONFIG_H
#define  MCONFIG_H

#include  "config.h"

#undef   OS_SOLARIS
#undef   OS_LINUX
#undef   OS_UNIX
#undef   OS_WIN
#undef   OS_TYPE
#undef   OS_SUBTYPE
#undef   CC_GCC
#undef   CC_MSC
#undef   CC_TYPE

#define   OS_SUBTYPE   "unknown"

#ifdef unix
#  define  OS_UNIX    1
#  define  OS_TYPE    "unix"
#   ifdef linux
#      define   OS_LINUX
#      undef    OS_SUBTYPE
#      define   OS_SUBTYPE   "linux-gnu"
#   endif
#   ifdef sun
#      define   OS_SOLARIS
#      undef    OS_SUBTYPE
#      define   OS_SUBTYPE   "solaris"
#   endif
#elif defined(__WIN__) || defined(__WINDOWS__) || defined(_WIN32)
#  define  OS_WIN    1
#  define  OS_TYPE    "windows"
#   ifndef  __WINDOWS__
#       define  __WINDOWS__     // for wxWindows 2.x
#   endif
#else
  // this reminder is important, it won't compile without it anyhow...
# error   "Unknown platform (forgot to #define unix?)"
#endif

// Are we using GCC?
#ifdef  __GNUG__
#  undef    CC_GCC  // might already be defined thanks to configure
#  define   CC_GCC
#  define   CC_TYPE "gcc"

   /// gcc does not support precompiled headers
#  ifdef USE_PCH
#     if USE_PCH
#        undef  USE_PCH
#        define   USE_PCH      // use the Mpch.h anyway
#     else
#        undef   USE_PCH
#     endif
#  endif

   // gcc gives an annoying warning
   // "class 'Foo' only defines a private destructor and has no friends"
   // even when it's a ref counted class which deletes itself.
   //
   // suppress it for all MObjectRC derived classes with this macro
#  define GCC_DTOR_WARN_OFF()    friend class MObjectRC
#else
#  define GCC_DTOR_WARN_OFF()
#endif

// Are we using Microsoft Visual C++ ?
#ifdef   _MSC_VER
#   define   CC_MSC   1
#   define   CC_TYPE  "Visual C++"
/// are we using precompiled headers?
#   ifndef USE_PCH
#      define USE_PCH        1
#   endif
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

/// name of the application
#define   M_APPLICATIONNAME   "M"
/// name of the vendor
#define   M_VENDORNAME   "M-Team"

/// name of global config file
#define   M_GLOBAL_CONFIG_NAME   "M.conf"

/// path for etc directories
#define   M_ETC_PATH   "/etc:/usr/etc:/usr/local/etc:/opt/etc:/usr/share/etc:/usr/local/share/etc:/opt/share/etc:/usr/local/stow/etc"

/// basic M installation directory
#ifdef OS_UNIX
#   define   M_BASEDIR M_PREFIX"share/M"
#else
#   define   M_PREFIX   "" 
#   define   M_BASEDIR M_PREFIX"\\M"
#endif
#ifdef   HAVE_COMPFACE_H
#   define   HAVE_XFACES
#endif

#ifdef   HAVE_COMPFACE_H
#   define   HAVE_XFACES
#endif

#define   M_STRBUFLEN      1024

#ifndef   USE_WXSTRING
#   define   USE_STD_STRING
#else
#   undef    USE_STD_STRING
#endif

#ifdef USE_STD_STRING
#   include   <string>
    typedef   std::string String;
#   define    Str(str)((str).c_str())
#else
#   include   <wx/string.h>
    typedef   wxString String;
#   define    Str(str) str
#   ifndef USE_WXWINDOWS2
#      define  c_str() GetData()
#      define  length() Length()       //FIXME dangerous!
#   endif
#endif

/// make sure NULL is define properly
#undef  NULL
#define NULL    0


// you can't mix iostream.h and iostream, the former doesn't compile
// with "using namespace std", the latter doesn't compile with wxWin
// make your choice...
#ifndef USE_IOSTREAMH
# define USE_IOSTREAMH   1
#endif

// Microsoft Visual C++
#ifdef  CC_MSC
   // suppress the warning "identifier was truncated to 255 characters
   // in the debug information"
#  pragma warning(disable: 4786)

  // <string> includes <istream> (Grrr...)
#   ifdef USE_IOSTREAMH
#      undef  USE_WXSTRING
#      define USE_WXSTRING    1
#   endif
#endif  // VC++

#ifdef           USE_IOSTREAMH
#       include <iostream.h>
#       include <fstream.h>
#else
#       include <iostream>
#       include <fstream>
#endif

#ifdef           USE_IOSTREAMH
  // can't use namespace std because old iostream doesn't compile with it
  // and can't use std::list because it's a template class
#else
  using namespace std;
#endif

#ifdef  USE_WXWINDOWS
# ifdef   USE_WXWINDOWS2
# define  WXCPTR      /**/
# define  WXSTR(str)  str
#else
# define  WXCPTR      (char *)
# define  WXSTR(str)  ((char *)str.c_str())
#endif

#endif

// set the proper STL class names
#ifdef  CC_MSC
# define  STL_LIST  std::list
#else //!Visual C++
# define  STL_LIST  list
#endif

#define Bool    int

// use builtin wxConfig by default in wxWin2 and appconf otherwise
#if !defined(USE_APPCONF) && !defined(USE_WXCONFIG)
#  if defined(USE_WXWINDOWS2)
#     define  USE_WXCONFIG 1
#  else
#     define  USE_APPCONF  1
#  endif
#endif

#endif
