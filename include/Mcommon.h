/*-*- c++ -*-********************************************************
 * Mcommon.h: common typedefs etc for M                             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 *******************************************************************/
#ifndef MCOMMON_H
#define	MCOMMON_H

#include	"Mconfig.h"

#ifdef	HAVE_LIBINTL_H
#	define 	USE_GETTEXT	1
#	include	<libintl.h>
#else
#	define	USE_GETTEXT	0
#endif

/// macro which defines compare operators needed for std::list
#define IMPLEMENT_DUMMY_COMPARE_OPERATORS(classname)                          \
    bool operator==(const classname&) const { assert(0); return false; }      \
    bool operator!=(const classname&) const { assert(0); return false; }      \
    bool operator< (const classname&) const { assert(0); return false; }      \
    bool operator> (const classname&) const { assert(0); return false; }

// wxWindows 2 has/will have native gettext support
#ifndef     USE_WXWINDOWS2
#   define	_(string)	mApplication.GetText(string)
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
#   ifdef  USE_WXGTK  
#      define SetIcon(x)
#   endif  //GTK
    
   // @@@ is this really the same thing
#   define wxMessage   wxStaticText
#   define wxCanvas    wxWindow
#   define wxItem      wxControl
#   define wxDialogBox wxDialog

    // the same function sometimes have different return types in wxWin1 and 2
#   define ON_CLOSE_TYPE    bool
#   define SHOW_TYPE        bool

#   define PanelNewLine(panel)

#   define CreateNamedPanel(parent, x, y, w, h, name)                          \
    GLOBAL_NEW wxPanel(parent, -1, wxPoint(x, y), wxSize(w, h), 0, name)
#   define CreatePanel(parent, x, y, w, h)                                     \
    GLOBAL_NEW wxPanel(parent, -1, wxPoint(x, y), wxSize(w, h))

#   define CreateLabel(parent, title)                                          \
    GLOBAL_NEW wxStaticText(parent, -1, _(title))

#   define CreateButton(parent, title, name, id)                              \
    GLOBAL_NEW wxButton(parent, id, _(title), wxDefaultPosition,              \
                        wxDefaultSize, 0, DEFAULT_VALIDATOR name)

#   define CreateText(parent, x, y, w, h, name)                               \
    GLOBAL_NEW wxTextCtrl(parent, -1, "", wxPoint(x, y), wxSize(w, h),        \
                          0, DEFAULT_VALIDATOR name)

#   define CreateListBox(parent, x, y, w, h)                                   \
    GLOBAL_NEW wxListBox(parent, -1, wxPoint(x, y), wxSize(w, h),             \
                         0, NULL, wxLB_SINGLE | wxLB_ALWAYS_SB)

#   define CreateFrame(parent, title, x, y, w, h)                              \
    Create(parent, -1, title, wxPoint(x, y), wxSize(w, h))
#else   // wxWin 1.xx
  // screen coordinates type
  typedef float coord_t;
  typedef coord_t lcoord_t;
  
#   define ON_CLOSE_TYPE     Bool
#   define SHOW_TYPE         void

#   define PanelNewLine(panel)    panel->NewLine()

#   define CreateNamedPanel(p, x, y, w, h, n)  GLOBAL_NEW wxPanel(p, x, y,     \
                                                                 w, h, 0, n)
#   define CreatePanel(p, x, y, w, h)  GLOBAL_NEW wxPanel(p, x, y, w, h)
#   define CreateLabel(p, t)           GLOBAL_NEW wxMessage(p, _(t))
#   define CreateButton(p, t, n, id)   GLOBAL_NEW wxButton(p, NULL, _(t),      \
                                                          -1, -1, -1, -1,     \
                                                          0, n)
#   define CreateText(p, x, y, w, h, name)   GLOBAL_NEW wxText(p, NULL, NULL,  \
                                                       "", x, y, w, h, 0, name)

#   define CreateListBox(parent, x, y, w, h)                                   \
    GLOBAL_NEW wxListBox(parent, (wxFunction) NULL, (const char *)"", \
			 0, x, y, w, h,           \
                         0, NULL, wxALWAYS_SB)
#   define CreateFrame(p, t, x, y, w, h) Create(p, t, x, y, w, h)
#endif


#ifndef NDEBUG
/// macro to define debug method
#	define DEBUG_DEF		void Debug(void) const;
#	define	DBGLOG(x)		cerr << x << endl;
#else
#	define	DEBUG_DEF
#	define	DBGLOG(x)		
#endif

#define	LogMsg(x) 	cerr << x << endl;

// LOG_INFO defined in yunchanc-client/.h
#undef  LOG_INFO
    
#ifdef   USE_WXWINDOWS2
   // wxWindows 2 has built in logging capabilities
#  include <wx/intl.h>
#	include <wx/log.h>

#	define LOG_DEBUG   wxLog::Debug
#	define LOG_NOISE   wxLog::Verbose
#	define LOG_DEFAULT wxLog::Message
#	define LOG_INFO    wxLog::Message
#	define LOG_ERROR   wxLog::Warning 
#	define LOG_URGENT  wxLog::Error
    
#	define	ERRORMESSAGE(arg)   wxLogError arg
#	define	SYSERRMESSAGE(arg)  wxLogError arg
#	define	FATALERROR(arg)     wxLogFatalError arg
#	define	INFOMESSAGE(arg)    wxLogInfo arg
#	define	LOGMESSAGE(arg)	  wxLogGeneric arg
#	ifdef NDEBUG
    // just log the error
#		define	INTERNALERROR(arg) wxLogError arg
#	else
    // aborts
#		define	INTERNALERROR(arg) wxLogFatalError arg
#	endif
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

  // @@
#  define   wxLogDebug
  
  /// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#	define	ERRORMESSAGE(arg) MDialog_ErrorMessage arg
  /// variable argument macro to do system error messages, call SYSERRMESSAGE((argument))
#	define	SYSERRMESSAGE(arg) MDialog_SystemErrorMessage arg
  /// variable argument macro to do error messages, call ERRORMESSAGE((argument))
#	define	FATALERROR(arg) MDialog_FatalErrorMessage arg
  /// variable argument macro to do information messages, call INFOMESSAGE((argument))
#	define	INFOMESSAGE(arg) MDialog_Message arg
  /// for logging messages
#	define	LOGMESSAGE(arg)	mApplication.Log arg
  /// for internal errors, core dumps or gives message for NDBEGU set
#	ifdef NDEBUG
#		define	INTERNALERROR(arg) MDialog_FatalErrorMessage arg
#	else
#		define	INTERNALERROR(arg) { MDialog_ErrorMessage arg ; abort(); }
#	endif
#endif

#ifdef   OS_UNIX
#	include	"Munix.h"
#elif   defined(OS_WIN)
#	include "Mwin.h"
#endif

#define	ABOUTMESSAGE \
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

#include	"CommonBase.h"

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
#   define   CALLBACK(name,profile,default)        PythonCallback(name,this,this->GetClassName(),profile)
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
#   define   CALLBACKVA(arg,default)            PythonCallback arg
#else
#   define   CALLBACK(name)       PythonCallback((int)default)
    inline int PythonCallBack(int def) { return def; }
#   define   CALLBACKVA(arg,default)      PythonCallback((int)default)
#endif
//@}
#endif	// MCOMMON_H
