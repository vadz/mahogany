/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#include   "Mpch.h"
#include   "Mcommon.h"
#include   "Python.h"
#include   "PythonHelp.h"

#ifndef   USE_PCH
#   include   "Mdefaults.h"
#   include   "MApplication.h"
#   include   "gui/wxMApp.h"
#   include   "gui/wxMDialogs.h"
#endif


// the module initialisations
extern "C"
{
   void initStringc();
   void initProfilec();
   void initMailFolderc();
   void initMAppBasec();
   void initMessagec();
};


/// used by PythonHelp.cc helper functions
PyObject *Python_MinitModule = NULL;


void
InitPython(void)
{
   String
      tmp;

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
   // initialise the interpreter
   Py_Initialize();
      
   // initialise the modules
   initStringc();
   initProfilec();
   initMailFolderc();
   initMAppBasec();
   initMessagec();
   
   // run the init script

   Python_MinitModule = PyImport_ImportModule("Minit");
   if(PyErr_Occurred())
   {
      String err;
      PyH_GetErrorMessage(&err);
      MDialog_ErrorMessage(Str("Python Error:\n"+err));
      PyErr_Print();
   }
   if(Python_MinitModule == NULL)
      MDialog_ErrorMessage("Python: Cannot find/evaluate Minit.py initialisation script.");
   else
   {
      Py_INCREF(Python_MinitModule); // keep it in memory
      PyObject *minit = PyObject_GetAttrString(Python_MinitModule,
                                               "Minit");
      if(minit)
      {
         Py_INCREF(minit);
         PyObject_CallObject(minit,NULL);
         if(PyErr_Occurred())
         {
            //PyErr_Print();
            String err;
            PyH_GetErrorMessage(&err);
            MDialog_ErrorMessage(Str("Python Error:\n"+err));
         }
         Py_DECREF(minit);
      }
      else
         MDialog_ErrorMessage("Python: Cannot find Minit.Minit function in Minit module.");
   }
}
