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

// the module initialisations

extern "C"
{
   void init_MString();
   void init_MObject();
};
//   void init_MProfile();
//   void init_MailFolder();
//   void init_MAppBase();
//   void init_Message();


/// used by PythonHelp.cc helper functions
PyObject *Python_MinitModule = NULL;

// returns TRUE if no error, FALSE if a Python error occured. In the last case
// an appropriate error message is logged.
static bool CheckPyError()
{
   if(PyErr_Occurred())
   {
      String err;
      PyH_GetErrorMessage(&err);
      ERRORMESSAGE(("Python error: %s", err.c_str()));
      PyErr_Print();

      return FALSE;
   }
   else {
      // no error
      return TRUE;
   }
}

bool
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

   // set PYTHONPATH correctly to find our modules
   String tmp = _T("PYTHONPATH=");

   String path = READ_APPCONFIG(MP_PYTHONPATH);
   bool didntHavePath = path.empty();
   if ( didntHavePath )
   {
      String pythondir;
      String globaldir = mApplication->GetGlobalDir();
      if ( globaldir.empty() )
      {
         // not installed
         pythondir << mApplication->GetDataDir() << DIR_SEPARATOR << _T("Python");
      }
      else
      {
         pythondir = globaldir;
      }

      // note that "scripts" has small 's'
      path << pythondir << PATH_SEPARATOR
           << pythondir << DIR_SEPARATOR << _T("Python") << PATH_SEPARATOR
           << pythondir << DIR_SEPARATOR << _T("scripts")
           // add also the uninstalled locations
#ifdef M_TOP_BUILDDIR
           // but "Python/Scripts" has a capital 'S'!
           << PATH_SEPARATOR << M_TOP_BUILDDIR << _T("/src/Python")
           << PATH_SEPARATOR << M_TOP_BUILDDIR << _T("/src/Python/Scripts")
#endif
           ;

      String localdir = mApplication->GetLocalDir();
      if ( localdir != globaldir )
      {
         path << PATH_SEPARATOR
              << localdir << DIR_SEPARATOR << _T("Scripts") << PATH_SEPARATOR;
      }
   }

   tmp += path;

   const char *pathOld = getenv("PYTHONPATH");
   if ( pathOld )
   {
      tmp << PATH_SEPARATOR << pathOld;
   }

   // on some systems putenv() takes "char *", cast silents the warnings but
   // should be harmless otherwise
   putenv((char *)tmp.c_str());

   // initialise the interpreter - this we do always, just to avoid problems
   Py_Initialize();

   if( !READ_CONFIG(mApplication->GetProfile(), MP_USEPYTHON) )
      return true; // it is not an error to have it disabled

   // initialise the modules
   init_MString();
   //init_MProfile();
   //init_MailFolder();
   //init_MAppBase();
   //init_Message();
   init_MObject();

   // the return code
   bool rc = TRUE;

   // run the init script
   Python_MinitModule = PyImport_ImportModule("Minit");

   if( !CheckPyError() || Python_MinitModule == NULL)
   {
      ERRORMESSAGE(("Python: Cannot find/evaluate Minit.py initialisation script."));

      rc = FALSE;
   }
   else
   {
      Py_INCREF(Python_MinitModule); // keep it in memory
      PyObject *minit = PyObject_GetAttrString(Python_MinitModule,
                                               "Minit");
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

         rc = FALSE;
      }
   }

   if ( rc && didntHavePath )
   {
      // rememember the path
      mApplication->GetProfile()->writeEntry(MP_PYTHONPATH, path);
   }

   return rc;
}

#endif // USE_PYTHON

