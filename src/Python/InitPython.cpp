/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#include   "Mpch.h"

#include   "Python.h"
#include   "PythonHelp.h"

#ifndef   USE_PCH
#   include   "Mcommon.h"

#   include   "Mdefaults.h"
#   include   "MApplication.h"
#   include   "gui/wxMApp.h"
#endif

#include   "MDialogs.h"

// the module initialisations
extern "C"
{
   void initMStringc();
   void initProfilec();
   void initMailFolderc();
   void initMAppBasec();
   void initMessagec();
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
      ERRORMESSAGE(("Python error: %s", err));
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
      tmp;

   // VZ: I don't know why, but Python ignores this variable if it's set from
   //     here under Windows - so don't waste our efforts...
#  ifndef OS_WIN
      // initialise python interpreter
      tmp = "PYTHONPATH=";
      tmp += mApplication.GetLocalDir();
      tmp += "/scripts";
      tmp += PATH_SEPARATOR;
      tmp += mApplication.GetGlobalDir();
      tmp += "/scripts";
      tmp += PATH_SEPARATOR;
      tmp += mApplication.GetProfile()->readEntry(MC_PYTHONPATH,MC_PYTHONPATH_D);
      if(getenv("PYTHONPATH"))
      {
         tmp += PATH_SEPARATOR;
         tmp += getenv("PYTHONPATH");
      }
      putenv(tmp.c_str());
#  endif

   // initialise the interpreter
   Py_Initialize();
      
   // initialise the modules
   initMStringc();
   initProfilec();
   initMailFolderc();
   initMAppBasec();
   initMessagec();

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
