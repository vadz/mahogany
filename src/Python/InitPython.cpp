/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#include   "Mpch.h"


#ifndef   USE_PCH
#   include   "Mcommon.h"

#   include   "MApplication.h"
#   include   "gui/wxMApp.h"
#   include   "strutil.h"
#endif

#include   "Mdefaults.h"

#include   "Python.h"
#include   "PythonHelp.h"
#include   "MDialogs.h"

// the module initialisations
extern "C"
{
   void initMStringc();
   void initMProfilec();
   void initMailFolderc();
   void initMAppBasec();
   void initMessagec();
   void initMObjectc();
};


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
   String
      tmp, path;

   // VZ: I don't know why, but Python ignores this variable if it's set from
   //     here under Windows - so don't waste our efforts...
#  ifndef OS_WIN
   // initialise python interpreter
   tmp = "PYTHONPATH=";
   path = mApplication->GetProfile()->readEntry(MP_PYTHONPATH,MP_PYTHONPATH_D);
   if(strutil_isempty(path))
   {
      path += mApplication->GetLocalDir();
      path += "/scripts";
      path += PATH_SEPARATOR;
      path += mApplication->GetGlobalDir();
      path += "/scripts";
      tmp += path;
      mApplication->GetProfile()->writeEntry(MP_PYTHONPATH,path);
   }
   else
      tmp += mApplication->GetProfile()->readEntry(MP_PYTHONPATH,MP_PYTHONPATH_D);
   if(getenv("PYTHONPATH"))
   {
      tmp += PATH_SEPARATOR;
      tmp += getenv("PYTHONPATH");
   }
   char *pathstring = strutil_strdup(tmp);  // this string must not be used again or freed
   putenv(pathstring);
#  endif

   // initialise the interpreter - this we do always, just to avoid problems
   Py_Initialize();

   if( !READ_CONFIG(mApplication->GetProfile(), MP_USEPYTHON) )
      return true; // it is not an error to have it disabled
   
   // initialise the modules
   initMStringc();
   initMProfilec();
   initMailFolderc();
   initMAppBasec();
   initMessagec();
   initMObjectc();

   // the return code
   bool rc = TRUE;

   // run the init script
   Python_MinitModule = PyImport_ImportModule("Minit");

   if( !CheckPyError() || Python_MinitModule == NULL) {
      ERRORMESSAGE(("Python: Cannot find/evaluate Minit.py initialisation script."));

      rc = FALSE;
   }
   else  {
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
      else {
         ERRORMESSAGE(("Python: Cannot find Minit.Minit function in Minit module."));

         rc = FALSE;
      }
   }

   return rc;
}
