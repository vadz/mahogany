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

// had Python been initialized? (MT ok as only used by the main thread)
static bool gs_isPythonInitialized = false;

// returns true if no error, false if a Python error occured. In the last case
// an appropriate error message is logged.
static bool CheckPyError()
{
   if ( PyErr_Occurred() )
   {
      ERRORMESSAGE((_T("%s"), PythonGetErrorMessage().c_str()));

      return false;
   }

   // no error
   return true;
}

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

   // set PYTHONPATH correctly to find our modules and scripts: this must be
   // done before loading the Python DLL below because its copy of CRT (which
   // may be different from ours when using VC9) will initialize its _environ
   // array from Win32 process environment when the DLL is loaded, so we must
   // update the environment before this happens
   String pythonPathNew;

   String path = READ_APPCONFIG(MP_PYTHONPATH);
   const bool didntHavePath = path.empty();
   if ( didntHavePath )
   {
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
   }

   wxLogTrace(_T("Python"), "Adding \"%s\" to PYTHONPATH", path.c_str());

   pythonPathNew += path;

   // also keep the old path but after our directories
   wxString pythonPathOld;
   if ( wxGetEnv("PYTHONPATH", &pythonPathOld) )
   {
      pythonPathNew << PATH_SEPARATOR << pythonPathOld;
   }

   if ( !wxSetEnv("PYTHONPATH", pythonPathNew) )
   {
      wxLogWarning(_("Setting PYTHONPATH to \"%s\" failed, loading some "
                     "Python modules might not work."), pythonPathNew);
   }

   // check if Python is available at all if we load it during run-time
#ifdef USE_PYTHON_DYNAMIC
   if ( !InitPythonDll() )
   {
      wxLogError(_("Python dynamic library couldn't be loaded."));

      return false;
   }
#endif // USE_PYTHON_DYNAMIC

   // initialise the interpreter -- this we do always, just to avoid problems
   Py_Initialize();
   gs_isPythonInitialized = true;

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
                       startScript.c_str()));

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

   if ( rc && didntHavePath )
   {
      // remember the path
      mApplication->GetProfile()->writeEntry(MP_PYTHONPATH, path);
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

#ifdef USE_PYTHON_DYNAMIC
   FreePythonDll();
#endif // USE_PYTHON_DYNAMIC
}

#endif // USE_PYTHON

