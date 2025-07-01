///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   InitPython.cpp: initialisation of the Python interpreter
// Purpose:     public InitPython() and FreePython()
// Author:      Karsten Ballueder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2004 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#include   "Mpch.h"

#ifdef USE_PYTHON

#ifndef   USE_PCH
#   include "Mcommon.h"

#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "strutil.h"
#   include "Profile.h"                 // for writeEntry
#endif

#include "MPython.h"

#include "Mdefaults.h"

#include "MDialogs.h"

extern const MOption MP_PYTHONPATH;
extern const MOption MP_PYTHONMODULE_TO_LOAD;
extern const MOption MP_USEPYTHON;

// module initialization functions (called by InitPython() below)
extern "C"
{
   void init_HeaderInfo();
   void init_MDialogs();
   void init_MailFolder();
   void init_MimePart();
   void init_MimeType();
   void init_Message();
   void init_SendMessage();
}

namespace
{

// had Python been initialized? (MT ok as only used by the main thread)
bool gs_isPythonInitialized = false;

// returns true if no error, false if a Python error occured. In the last case
// an appropriate error message is logged.
bool CheckPyError()
{
   if ( PyErr_Occurred() )
   {
      ERRORMESSAGE((PythonGetErrorMessage()));

      return false;
   }

   // no error
   return true;
}

void SetPythonPath()
{
   String path = READ_APPCONFIG(MP_PYTHONPATH);
   if ( path.empty() )
   {
      // construct a reasonable default path

      // first, use user files
      String localdir = mApplication->GetLocalDir();
      if ( !localdir.empty() )
      {
         path                   << localdir << DIR_SEPARATOR << _T("scripts")
              << PATH_SEPARATOR << localdir << DIR_SEPARATOR << _T("python");
      }

      // next, use the program files
      String globaldir = mApplication->GetDataDir();
      if ( !globaldir.empty() && globaldir != localdir )
      {
          // note that both "python" and "scripts" directories are not
          // capitalized after installation (as used here) but are capitalized
          // in the source tree (below) -- this is a historical accident...
          path << PATH_SEPARATOR << globaldir << DIR_SEPARATOR << _T("scripts")
               << PATH_SEPARATOR << globaldir << DIR_SEPARATOR << _T("python");
      }

      // finally, also add the uninstalled locations: this makes it possible
      // to use the program without installing it which is handy
#ifdef M_TOP_BUILDDIR
      path << PATH_SEPARATOR << M_TOP_BUILDDIR << _T("/src/Python");
#endif // M_TOP_BUILDDIR

#ifdef M_TOP_SOURCEDIR
      path << PATH_SEPARATOR << M_TOP_SOURCEDIR << _T("/src/Python/Scripts");
#endif // M_TOP_SOURCEDIR

      // remember the path to avoid having to construct it again the next time
      mApplication->GetProfile()->writeEntry(MP_PYTHONPATH, path);
   }

   wxLogTrace("Python", "Prepending \"%s\" to sys.path", path);

   PyRun_SimpleString
   (
      wxString::Format
      (
         "import sys\n"
         "sys.path[:0]='%s'.split('%c')\n",
         path, PATH_SEPARATOR
      )
   );
}

} // anonymous namespace

extern bool
IsPythonInitialized()
{
   return gs_isPythonInitialized;
}

extern bool
InitPython(void)
{
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
   {
      // it is not an error to have it disabled
      return true;
   }

   // initialise the interpreter -- this we do always, just to avoid problems
   Py_Initialize();
   gs_isPythonInitialized = true;


   // set up the module loading path so that our modules would be found
   SetPythonPath();

   // initialise the modules
   init_HeaderInfo();
   init_MDialogs();
   init_MailFolder();
   init_MimePart();
   init_MimeType();
   init_Message();
   init_SendMessage();

   // the return code
   bool rc = true;

   // run the init script
   String startScript = READ_APPCONFIG(MP_PYTHONMODULE_TO_LOAD);

   if ( !startScript.empty() )
   {
      PyObject *moduleInit = PyImport_ImportModule(startScript.char_str());

      if ( !CheckPyError() || !moduleInit )
      {
         ERRORMESSAGE(("Cannot load Python module \"%s\".",
                       startScript));

         rc = false;
      }
      else
      {
         Py_INCREF(moduleInit); // keep it in memory
         PyObject *minit = PyObject_GetAttrString(moduleInit, "Init");
         if ( minit )
         {
            PyObject_CallObject(minit, NULL);
            rc = CheckPyError();
         }
         //else: no Init() function, ignore it
      }
   }

   return rc;
}

extern
void FreePython()
{
   if ( gs_isPythonInitialized )
   {
      // ensure there are no pending errors because otherwise Py_Finalize()
      // calls abort()
      PyErr_Clear();

      Py_Finalize();

      gs_isPythonInitialized = false;
   }
}

#endif // USE_PYTHON

