/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
 * Revision 1.8  1998/07/08 11:56:52  KB
 * M compiles and runs on Solaris 2.5/gcc 2.8/c-client gso
 *
 * Revision 1.7  1998/06/14 12:24:10  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.6  1998/06/09 10:14:38  KB
 * InitPython now exits gracefully if Minit.Minit() cannot be found.
 *
 * Revision 1.5  1998/06/08 08:19:06  KB
 * Fixed makefiles for wxtab/python. Made Python work with new MAppBase.
 *
 * Revision 1.4  1998/05/30 17:55:50  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.3  1998/05/24 14:47:31  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.2  1998/05/24 08:28:54  KB
 * eventually fixed the type problem, now python works as expected
 *
 * Revision 1.1  1998/05/02 18:31:38  KB
 * Python integration works.
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
      MDialog_ErrorMessage("Python Error:\n"+err);
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
            MDialog_ErrorMessage("Python Error:\n"+err);
         }
         Py_DECREF(minit);
      }
      else
         MDialog_ErrorMessage("Python: Cannot find Minit.Minit function in Minit module.");
   }
}
