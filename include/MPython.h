/*-*- c++ -*-********************************************************
 * MPython.h - include file for M's python code                     *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MPYTHON_H
#define MPYTHON_H

#include "Mconfig.h"    // for USE_PYTHON

#ifdef USE_PYTHON

// SWIG-generated files don't include Mpch.h themselves
#include "Mpch.h"

// before including Python.h, undef all these macros defined by our config.h
// and redefined by Python's config.h under Windows to avoid the warnings
#ifdef OS_WIN
   #undef HAVE_STRERROR
   #undef HAVE_PROTOTYPES
   #undef HAVE_STDARG_PROTOTYPES
#endif // OS_WIN

// under Linux we get annoying messages about redefinition of these symbols
// which are defined by the standard headers already included above
#ifdef __LINUX__
   #ifdef _POSIX_C_SOURCE
      #undef _POSIX_C_SOURCE
   #endif

   #ifdef _XOPEN_SOURCE
      #undef _XOPEN_SOURCE
   #endif
#endif // __LINUX__

// and under Windows for this symbol which is defined in wx/defs.h
#ifdef HAVE_SSIZE_T
   #define HAVE_SSIZE_T_DEFINED_BY_WX
   #undef HAVE_SSIZE_T
#endif

// we don't want to use Python debug DLL even if we're compiled in debug
// ourselves, so make sure _DEBUG is undefined so that Python.h doesn't
// reference any symbols only available in the debug build of Python
#ifdef _DEBUG
   #define MAHOGANY_DEBUG_WAS_DEFINED
   #undef _DEBUG
#endif

// Disable warnings about "inconsistent dll linkage" for several functions
// declared in Python.h without any declspec as they clash with the
// declarations of these functions when using CRT as a DLL.
#ifdef CC_MSC
   #if _MSC_VER >= 1800
      #pragma warning(push)
      #pragma warning(disable: 4273)
   #endif
#endif

#include <Python.h>

#ifdef CC_MSC
   #if _MSC_VER >= 1800
      #pragma warning(pop)
   #endif
#endif

#ifdef MAHOGANY_DEBUG_WAS_DEFINED
   #undef MAHOGANY_DEBUG_WAS_DEFINED
   #define _DEBUG
#endif

#if !defined(HAVE_SSIZE_T) && defined(HAVE_SSIZE_T_DEFINED_BY_WX)
   #define HAVE_SSIZE_T
#endif

#ifdef __LINUX__
   // Just include a standard header again to get the correct values back.
   #include <features.h>
#endif

#include "PythonHelp.h"

#define  M_PYTHON_MODULE "Minit"

#endif // USE_PYTHON

#endif // MPYTHON_H
