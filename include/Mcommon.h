/*-*- c++ -*-********************************************************
 * Mcommon.h: common typedefs etc for M                             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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
  
// @@@ wxGTK alpha 10 doesn't have validators (yet)
#  ifdef  USE_WXGTK
#     define DEFAULT_VALIDATOR
#  else
#     define DEFAULT_VALIDATOR wxDefaultValidator, 
#  endif //GTK
    
   // @@@ is this really the same thing
#  define wxMessage   wxStaticText
#  define wxCanvas    wxWindow
#  define wxItem      wxControl
#  define wxDialogBox wxDialog

   // the same function sometimes have different return types in wxWin1 and 2
#  define ON_CLOSE_TYPE    bool
#  define SHOW_TYPE        bool

#  define PanelNewLine(panel)

#  define CreatePanel(parent, x, y, w, h, name)                               \
   GLOBAL_NEW wxPanel(parent, -1, wxPoint(x, y), wxSize(w, h),                \
                      wxTAB_TRAVERSAL, name)

#  define CreateLabel(parent, title)                                          \
   GLOBAL_NEW wxStaticText(parent, -1, _(title))

#  define CreateButton(parent, title, name, id)                               \
   GLOBAL_NEW wxButton(parent, id, _(title), wxDefaultPosition,               \
                        wxDefaultSize, 0, DEFAULT_VALIDATOR name)

#  define CreateText(parent, x, y, w, h, name)                                \
   GLOBAL_NEW wxTextCtrl(parent, -1, M_EMPTYSTRING, wxPoint(x, y), wxSize(w, h),         \
                         0, DEFAULT_VALIDATOR name)

#  define CreateListBox(parent, x, y, w, h)                                   \
   GLOBAL_NEW wxListBox(parent, -1, wxPoint(x, y), wxSize(w, h),              \
                        0, NULL, wxLB_SINGLE | wxLB_ALWAYS_SB)

#  define CreateFrame(parent, title, x, y, w, h)                              \
   Create(parent, -1, title, wxPoint(x, y), wxSize(w, h))

// ----------------------------------------------------------------------------
// debugging macros
// ----------------------------------------------------------------------------
#ifdef DEBUG
#  define   DEBUG_DEF     void Debug(void) const;
#  ifdef USE_WXWINDOWS2
#     define ASSERT(x)          wxASSERT(x)
#     define ASSERT_MSG(x, msg) wxASSERT_MSG(x, msg)
#  else  // !wxWin2
#     include <assert.h>

#     define ASSERT(x)          assert(x)
#     define ASSERT_MSG(x, msg) DBGMESSAGE((msg)); assert(x)
#  endif // wxWin2
#else
#  define   DEBUG_DEF
// these macros do nothing in release build
#  define ASSERT(x)
#  define ASSERT_MSG(x, msg)
#endif

#define FAIL           ASSERT(0)
#define FAIL_MSG(msg)  ASSERT_MSG(0, msg)

// these definitions work in both modes (debug and release)
#define CHECK(x, rc, msg)  wxCHECK_MSG(x, rc, msg)
#define CHECK_RET(x, msg)  wxCHECK_RET(x, msg)

// ----------------------------------------------------------------------------
// message logging macros
// ----------------------------------------------------------------------------

// LOG_INFO defined in c-client/.h
#undef  LOG_INFO
    
// wxWindows 2 has built in logging capabilities
#  include <wx/intl.h>
#  include <wx/log.h>

#  define M_LOG_DEBUG   wxLOG_Debug
#  define M_LOG_NOISE   wxLOG_Verbose
#  define M_LOG_DEFAULT wxLOG_Message
#  define M_LOG_INFO    wxLOG_Message
#  define M_LOG_ERROR   wxLOG_Warning 
#  define M_LOG_URGENT  wxLOG_Error
    
#  define ERRORMESSAGE(arg)   wxLogError arg
#  define SYSERRMESSAGE(arg)  wxLogSysError arg
#  define FATALERROR(arg)     wxLogFatalError arg
#  define INFOMESSAGE(arg)    wxLogInfo arg
#  define LOGMESSAGE(arg)     wxLogGeneric arg

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

#define  ABOUTMESSAGE \
"M Copyright (C) 1998 by \n"\
"  Karsten Ballüder\n"\
"  11 (2F3) Yeaman Place\n" \
"  Edinburgh EH11 1BR, Scotland\n" \
"  (Ballueder@usa.net)\n"\
"\n"\
"For the latest news on M see\n"\
"http://Ballueder.home.ml.org/M/\n"\
"M has been written using the cross-platform\n"\
"class library wxWindows which is freely\n"\
"available under the GNU Library Public\n"\
"License. Plase see\n"\
"http://web.ukonline.co.uk/julian.smart/wxwin/\n"\
"for more information."\
"\n"

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

#endif	// MCOMMON_H
