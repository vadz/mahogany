/*-*- c++ -*-********************************************************
 * InitPython.cpp: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

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

/// used by PythonHelp.cc helper functions
PyObject *Python_MinitModule = NULL;

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
InitPython(void)
{
   // first check if Python is available
#ifdef USE_PYTHON_DYNAMIC
   if ( !InitPythonDll() )
   {
      wxLogError(_("Python dynamic library couldn't be found."));

      return false;
   }
#endif // USE_PYTHON_DYNAMIC

   // set PYTHONPATH correctly to find our modules and scripts
   String pythonPathNew = _T("PYTHONPATH=");

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
   const char *pythonPathOld = getenv("PYTHONPATH");
   if ( pythonPathOld )
   {
      pythonPathNew << PATH_SEPARATOR << pythonPathOld;
   }

   // on some systems putenv() takes "char *", cast silents the warnings but
   // should be harmless otherwise
   putenv((char *)pythonPathNew.c_str());

   if ( !READ_CONFIG(mApplication->GetProfile(), MP_USEPYTHON) )
   {
      // it is not an error to have it disabled
      return true;
   }

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
   Python_MinitModule = PyImport_ImportModule("Minit");

   if( !CheckPyError() || Python_MinitModule == NULL)
   {
      ERRORMESSAGE(("Python: Cannot find/evaluate Minit.py initialisation script."));

      rc = false;
   }
   else
   {
      Py_INCREF(Python_MinitModule); // keep it in memory
      PyObject *minit = PyObject_GetAttrString(Python_MinitModule, "Minit");
      if(minit)
      {
         Py_INCREF(minit);
         PyObject_CallObject(minit,NULL);
         rc = CheckPyError();
         Py_DECREF(minit);
      }
      else
      {
         (void)CheckPyError();

         ERRORMESSAGE(("Python: Cannot find Minit.Minit function in Minit module."));

         rc = false;
      }
   }

   if ( rc && didntHavePath )
   {
      // rememember the path
      mApplication->GetProfile()->writeEntry(MP_PYTHONPATH, path);
   }

   return rc;
}

extern
void FreePython()
{
   if ( gs_isPythonInitialized )
   {
      Py_Finalize();

      gs_isPythonInitialized = false;
   }

#ifdef USE_PYTHON_DYNAMIC
   FreePythonDll();
#endif // USE_PYTHON_DYNAMIC
}

#endif // USE_PYTHON

