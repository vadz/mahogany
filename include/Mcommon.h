/*-*- c++ -*-********************************************************
 * Mcommon.h: common typedefs etc for M                             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MCOMMON_H
#define  MCOMMON_H

#include "Mconfig.h"

#ifdef   HAVE_LIBINTL_H
#  define   USE_GETTEXT 1
#  include  <libintl.h>
#else
#  undef    USE_GETTEXT
#endif

/// macro which defines compare operators needed for std::list
#define IMPLEMENT_DUMMY_COMPARE_OPERATORS(classname)                          \
    bool operator==(const classname&) const { assert(0); return false; }      \
    bool operator!=(const classname&) const { assert(0); return false; }      \
    bool operator< (const classname&) const { assert(0); return false; }      \
    bool operator> (const classname&) const { assert(0); return false; }

// wxWindows 2 has/will have native gettext support
#ifndef     USE_WXWINDOWS2
#   define  _(string)   mApplication.GetText(string)
#endif  // wxWin 2

// hide differences between wxWin versions
#ifdef  USE_WXWINDOWS2
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
    
   // @@@ wxFrame::SetIcon doesn't exist in wxGTK
#  ifdef  USE_WXGTK  
#     define SetIcon(x)
#  endif  //GTK
    
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
   GLOBAL_NEW wxTextCtrl(parent, -1, "", wxPoint(x, y), wxSize(w, h),         \
                         0, DEFAULT_VALIDATOR name)

#  define CreateListBox(parent, x, y, w, h)                                   \
   GLOBAL_NEW wxListBox(parent, -1, wxPoint(x, y), wxSize(w, h),              \
                        0, NULL, wxLB_SINGLE | wxLB_ALWAYS_SB)

#  define CreateFrame(parent, title, x, y, w, h)                              \
   Create(parent, -1, title, wxPoint(x, y), wxSize(w, h))
#else   // wxWin 1.xx
  // screen coordinates type
  typedef float coord_t;
  typedef coord_t lcoord_t;
  
#   define ON_CLOSE_TYPE     Bool
#   define SHOW_TYPE         void

#   define PanelNewLine(panel)    panel->NewLine()

#   define CreatePanel(p, x, y, w, h, n)  GLOBAL_NEW wxPanel(p,x,y,w,h,0,n)
#   define CreateLabel(p, t)              GLOBAL_NEW wxMessage(p, _(t))
#   define CreateButton(p, t, n, id)      GLOBAL_NEW wxButton(p, NULL, _(t),   \
                                                          -1, -1, -1, -1,      \
                                                          0, n)
#   define CreateText(p, x, y, w, h, name)   GLOBAL_NEW wxText(p, NULL, NULL,  \
                                                       "", x, y, w, h, 0, name)

#   define CreateListBox(parent, x, y, w, h)                                   \
    GLOBAL_NEW wxListBox(parent, (wxFunction) NULL, (const char *)"",          \
                         0, x, y, w, h,                                        \
                         0, NULL, wxALWAYS_SB)
#   define CreateFrame(p, t, x, y, w, h) Create(p, t, x, y, w, h)
#endif

// ----------------------------------------------------------------------------
// debugging macros
// ----------------------------------------------------------------------------
#ifdef NDEBUG
#  define   DEBUG_DEF

   // these macros do nothing in release build
#  define ASSERT(x)
#  define ASSERT_MSG(x, msg)
#else
#  define   DEBUG_DEF     void Debug(void) const;
#  ifdef USE_WXWINDOWS2
#     define ASSERT(x)          wxASSERT(x)
#     define ASSERT_MSG(x, msg) wxASSERT_MSG(x, msg)
#  else  // !wxWin2
#     include <assert.h>

#     define ASSERT(x)          assert(x)
#     define ASSERT_MSG(x, msg) DBGMESSAGE((msg)); assert(x)
#  endif // wxWin2
#endif

#define FAIL           ASSERT(0)
#define FAIL_MSG(msg)  ASSERT_MSG(0, msg)

// these definitions work in both modes (debug and release)
#  ifdef USE_WXWINDOWS2
#     define CHECK(x, rc, msg)  wxCHECK_MSG(x, rc, msg)
#     define CHECK_RET(x, msg)  wxCHECK_RET(x, msg)
#  else  // !wxWin2
#     define CHECK(x, rc, msg)  if (!(x)) {FAIL_MSG(msg); return rc; }
#     define CHECK_RET(x, msg)  if (!(x)) {FAIL_MSG(msg); return; }
#  endif // wxWin2

// ----------------------------------------------------------------------------
// message logging macros
// ----------------------------------------------------------------------------

// LOG_INFO defined in yunchanc-client/.h
#undef  LOG_INFO
    
#ifdef   USE_WXWINDOWS2
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

#  ifdef NDEBUG
#     define   DBGMESSAGE(arg)
#     define   TRACEMESSAGE(arg)
#     define   DBGLOG(arg)
#     define   INTERNALERROR(arg) wxLogError arg         // just log the error
#  else
#     define   DBGMESSAGE(arg)    wxLogDebug arg
#     define   TRACEMESSAGE(arg)  wxLogTrace arg
#     define   DBGLOG(arg)        wxLogTrace(wxString("") << arg)
#     define   INTERNALERROR(arg) wxLogFatalError arg    // aborts
#  endif
#else
  /// define log levels
  enum 
  {
//FIXME    LOG_DEBUG = -2,
    LOG_NOISE, 
    LOG_DEFAULT = 0, 
    LOG_INFO = 0, 
    LOG_ERROR, 
    LOG_URGENT 
  };

  // @@ no debug messages in wxWin1
#  define   DBGMESSAGE(arg)
#  define   TRACEMESSAGE(arg)
  
  /// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#  define   ERRORMESSAGE(arg) MDialog_ErrorMessage arg
  /// variable argument macro to do system error messages, call SYSERRMESSAGE((argument))
#  define   SYSERRMESSAGE(arg) MDialog_SystemErrorMessage arg
  /// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#  define   FATALERROR(arg) MDialog_FatalErrorMessage arg
  /// variable argument macro to do information messages, call INFOMESSAGE((argument))
#  define   INFOMESSAGE(arg) MDialog_Message arg
  /// for logging messages
#  define   LOGMESSAGE(arg)   mApplication.Log arg
  /// for internal errors, core dumps or gives message for NDBEGU set
#  ifdef NDEBUG
#     define   INTERNALERROR(arg) MDialog_FatalErrorMessage arg
#     define   DEBUG_DEF
#     define   DBGLOG(x)
#  else
#     define   INTERNALERROR(arg) { MDialog_ErrorMessage arg ; abort(); }
#     define   DEBUG_DEF     void Debug(void) const;
#     define   DBGLOG(x)     cerr << x << endl;
#  endif

#  define  LogMsg(x) cerr << x << endl;
#endif

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

#include "CommonBase.h"

/// for convenience, get an icon:

#define   ICON(name) (mApplication.GetIconManager()->GetIcon(name))

/**@name Macros for calling callback functions.

  These macros do nothing and return the default returnvalue when
  Python is disabled.
*/
//@{
#ifdef   USE_PYTHON
/** This macro takes three arguments: the callback function name to
    look up in a profile, a profile pointer (can be NULL) and a
    default return value.
    It can only be called from within object member functions and the
    object needs to support the GetClasName() method to get the Python
    class name for the argument.
*/
#   define   PY_CALLBACK(name,profile,default)        PythonCallback(name,this,this->GetClassName(),profile)
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

#endif	// MCOMMON_H
