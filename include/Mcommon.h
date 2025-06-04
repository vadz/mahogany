///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   Mcommon.h - the common declarations included by everyone
// Purpose:     contains debugging and logging macros, mainly, which everyone
//              needs
// Author:      Karsten Ballüder
// Modified by:
// Created:     11.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MCOMMON_H
#define MCOMMON_H

#include "Mconfig.h"

// ----------------------------------------------------------------------------
// debugging macros
// ----------------------------------------------------------------------------

#ifdef ASSERT
#   undef ASSERT
#endif
#ifdef DEBUG
#  define   DEBUG_DEF     void Debug(void) const;
#  undef ASSERT
#  ifndef __WXDEBUG__
#     define ASSERT(x)          assert(x)
#     define ASSERT_MSG(x, msg) assert(x)
#  else
#     define ASSERT(x)          wxASSERT(x)
#     define ASSERT_MSG(x, msg) wxASSERT_MSG(x, msg)
#  endif
#else
#  define   DEBUG_DEF
// these macros do nothing in release build
#  define ASSERT(x)
#  define ASSERT_MSG(x, msg)
#endif

#define FAIL           ASSERT(0)
#define FAIL_MSG(msg)  ASSERT_MSG(0, msg)

// these definitions work in both modes (debug and release)
#define VERIFY(x, msg)     if ( !(x) ) { wxFAIL_MSG(msg); }
#define CHECK(x, rc, msg)  wxCHECK_MSG(x, rc, msg)
#define CHECK_RET(x, msg)  wxCHECK_RET(x, msg)

// ----------------------------------------------------------------------------
// i18n
// ----------------------------------------------------------------------------

// override wxWin setting if necessary
#ifndef USE_I18N
#  undef wxUSE_INTL
#  define wxUSE_INTL 0
#endif // USE_I18N

#include <wx/intl.h>

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

/// function which will close the splash screen if it's (still) opened
extern void CloseSplash();

// ----------------------------------------------------------------------------
// message logging macros
// ----------------------------------------------------------------------------

// wxWindows 2 has built in logging capabilities
#  include <wx/log.h>

#  define M_LOG_DEBUG   wxLOG_Debug
#  define M_LOG_VERBOSE wxLOG_Info
#  define M_LOG_WINONLY wxLOG_User        // only log the msg to the log window
#  define M_LOG_DEFAULT wxLOG_Message
#  define M_LOG_INFO    wxLOG_Message
#  define M_LOG_ERROR   wxLOG_Warning
#  define M_LOG_URGENT  wxLOG_Error

#  define ERRORMESSAGE(arg)   CloseSplash(); wxLogError arg
#  define LOGMESSAGE(arg)     wxLogGeneric arg
#  define STATUSMESSAGE(arg)  wxLogStatus arg
#  ifdef DEBUG
#     define   DBGMESSAGE(arg)    wxLogDebug arg
#  else
#     define   DBGMESSAGE(arg)
#  endif

#if defined(OS_UNIX) || defined(__CYGWIN__)
#  include "Munix.h"
#elif   defined(OS_WIN)
#  include "Mwin.h"
#endif

// ----------------------------------------------------------------------------
// misc macros
// ----------------------------------------------------------------------------

/// for convenience, get an icon:
#define   ICON(name) (mApplication->GetIconManager()->GetIcon(name))

/// define a NULL for strings (used in SWIG *.i files)
#define   NULLstring wxGetEmptyString()

/// defines an empty string for argument lists, needed for scandoc
#define   M_EMPTYSTRING   wxEmptyString

// ----------------------------------------------------------------------------
// Python macros
// ----------------------------------------------------------------------------

/**@name Macros for calling callback functions.

  These macros do nothing and return the default returnvalue when
  Python is disabled.
*/
#ifdef   USE_PYTHON

//@{
/** This macro takes three arguments: the callback function name to
    look up in a profile, a default return value, and a profile
    pointer (can be NULL).
    It can only be called from within object member functions and the
    object needs to support the GetClasName() method to get the Python
    class name for the argument.
*/
#   define   PY_CALLBACK(name, def, profile) \
   PythonCallback(name,def,this,this->GetClassName(),profile)

/** This macro takes multiple arguments.
    The last argument is the default return value.
    The first argument is a list of arguments in brackets, it must
    contain the name of the callback to look up in the profiles, the
    object pointer, the object's class name, a profile pointer (or
    NULL) and a format string for one further argument. The additional
    argument can be a tuple, i.e. the format string can be set to
    something like "(iis)" (two ints and a string) and the arguments
    must follow the format string.

*/
#   define   PY_CALLBACKVA(arg, def)            PythonCallback arg
//@}

#else // !USE_PYTHON
#   define   PY_CALLBACK(name, def, profile)   (def)
#   define   PY_CALLBACKVA(arg, def)      (def)
#endif // USE_PYTHON/!USE_PYTHON


// ----------------------------------------------------------------------------
// misc types
// ----------------------------------------------------------------------------

/**
  @name Mail-related typedefs
 */
//@{
/// A Type used for message UIds
typedef unsigned long UIdType;

/// An illegal, never happening UId number:
#define UID_ILLEGAL   ULONG_MAX

/// type for msgno numbers
typedef unsigned long MsgnoType;

/// an illegal msgno value (0 because msgnos are counted from 1)
#define MSGNO_ILLEGAL 0
//@}

/// for compilation with wxWindows versions < 2.3.3
#ifndef WX_DEFINE_ARRAY_INT
   #define WX_DEFINE_ARRAY_INT WX_DEFINE_ARRAY
#endif // !defined(WX_DEFINE_ARRAY_INT)

#endif // MCOMMON_H

