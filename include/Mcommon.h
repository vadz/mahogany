/*-*- c++ -*-********************************************************
 * Mcommon.h: common typedefs etc for M                             *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/


#ifndef MCOMMON_H
#define  MCOMMON_H

#include   "Mconfig.h"
#include   "MObject.h"

/// defines an empty string for argument lists, needed for scandoc
#define   M_EMPTYSTRING   ""

/// macro which defines compare operators needed for std::list
#define IMPLEMENT_DUMMY_COMPARE_OPERATORS(classname)                          \
    bool operator==(const classname&) const { assert(0); return false; }      \
    bool operator!=(const classname&) const { assert(0); return false; }      \
    bool operator< (const classname&) const { assert(0); return false; }      \
    bool operator> (const classname&) const { assert(0); return false; }

// screen coordinates type
typedef int coord_t;

// @@: in both wxGTK/wxMSW 'long' and 'int' are used for the same things in
//     different contexts
typedef long int lcoord_t;

// ----------------------------------------------------------------------------
// debugging macros
//
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
#     define ASSERT_RET(x)      {assert(x); if(!(x)) return;}
#  else
#     define ASSERT(x)          wxASSERT(x)
#     define ASSERT_MSG(x, msg) wxASSERT_MSG(x, msg)
#     define ASSERT_RET(x)      {wxASSERT(x); if(!(x)) return;}
#  endif
#else
#  define   DEBUG_DEF
// these macros do nothing in release build
#  define ASSERT(x)
#  define ASSERT_MSG(x, msg)
#  define ASSERT_RET(x)      {if(!(x)) return;}
#endif

#define FAIL           ASSERT(0)
#define FAIL_MSG(msg)  ASSERT_MSG(0, msg)

// these definitions work in both modes (debug and release)
#define VERIFY(x, msg)     if ( !(x) ) { wxFAIL_MSG(msg); }
#define CHECK(x, rc, msg)  wxCHECK_MSG(x, rc, msg)
#define CHECK_RET(x, msg)  wxCHECK_RET(x, msg)

// ----------------------------------------------------------------------------
// message logging macros
// ----------------------------------------------------------------------------

// wxWindows 2 has built in logging capabilities
#  include <wx/intl.h>
#  include <wx/log.h>

#  define M_LOG_DEBUG   wxLOG_Debug
#  define M_LOG_VERBOSE wxLOG_Info
#  define M_LOG_WINONLY wxLOG_User        // only log the msg to the log window
#  define M_LOG_DEFAULT wxLOG_Message
#  define M_LOG_INFO    wxLOG_Message
#  define M_LOG_ERROR   wxLOG_Warning
#  define M_LOG_URGENT  wxLOG_Error

#  define ERRORMESSAGE(arg)   wxLogError arg
#  define SYSERRMESSAGE(arg)  wxLogSysError arg
#  define FATALERROR(arg)     wxLogFatalError arg
#  define INFOMESSAGE(arg)    wxLogInfo arg
#  define LOGMESSAGE(arg)     wxLogGeneric arg
#  define STATUSMESSAGE(arg)  wxLogStatus arg
#  ifdef DEBUG
#     define   DBGMESSAGE(arg)    wxLogDebug arg
#     define   TRACEMESSAGE(arg)  wxLogTrace arg
#     define   DBGLOG(arg)        wxLogTrace(wxString(M_EMPTYSTRING) << arg)
#     define   INTERNALERROR(arg) wxLogFatalError arg    // aborts
#  else
#     define   DBGMESSAGE(arg)
#     define   TRACEMESSAGE(arg)
#     define   DBGLOG(arg)
#     define   INTERNALERROR(arg) wxLogError arg         // just log the error
#  endif

#ifdef   OS_UNIX
#  include  "Munix.h"
#elif   defined(OS_WIN)
#  include "Mwin.h"
#endif

/// for convenience, get an icon:
#define   ICON(name) (mApplication->GetIconManager()->GetIcon(name))

/**@name Macros for calling callback functions.

  These macros do nothing and return the default returnvalue when
  Python is disabled.
*/
//@{
#ifdef   USE_PYTHON
/** This macro takes three arguments: the callback function name to
    look up in a profile, a default return value, and a profile
    pointer (can be NULL).
    It can only be called from within object member functions and the
    object needs to support the GetClasName() method to get the Python
    class name for the argument.
*/
#   define   PY_CALLBACK(name,default,profile)        PythonCallback(name,default,this,this->GetClassName(),profile)
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
#   define   PY_CALLBACKVA(arg,default)            PythonCallback arg
#else
#   define   PY_CALLBACK(name, profile, default)   PythonCallback((int)default)
    inline int PythonCallback(int def) { return def; }
#   define   PY_CALLBACKVA(arg,default)      PythonCallback((int)default)
#endif
//@}

/// define a NULL for strings (FIXME: is this valid for std::string ?)
#define   NULLstring String((const char *)NULL)

#endif // MCOMMON_H

