/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
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
//#include   "pythonrun.h"
//#include   "pyerrors.h"
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
   tmp = "";
   tmp += mApplication.GetLocalDir();
   VAR(mApplication.GetLocalDir());
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp += mApplication.GetGlobalDir();
   VAR(mApplication.GetGlobalDir());
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp +=
      mApplication.GetProfile()->readEntry(MC_PYTHONPATH,MC_PYTHONPATH_D);
   VAR(mApplication.GetProfile()->readEntry(MC_PYTHONPATH,MC_PYTHONPATH_D));
   if(getenv("PYTHONPATH"))
   {
      tmp += PATH_SEPARATOR;
      tmp += getenv("PYTHONPATH");
   }
   setenv("PYTHONPATH", tmp.c_str(), 1);
   VAR(getenv("PYTHONPATH"));
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
      PyErr_Print();
   if(Python_MinitModule == NULL)
      MDialog_ErrorMessage("Python: Cannot find/evaluate Minit.py initialisation script.");

   Py_INCREF(Python_MinitModule);
   PyObject *minit = PyObject_GetAttrString(Python_MinitModule, "Minit");
   if(minit)
   {
      PyObject_CallObject(minit,NULL);
      if(PyErr_Occurred())
         PyErr_Print();
   }
   else
      MDialog_ErrorMessage("Python: Cannot find Minit function in Minit module.");


/* example code for calling a callback function:
  String teststring = "Hello World";
   PyH_CallFunction("StringPrint",
                      "StringPrint",
                      &teststring, "String",
                      "",NULL);
*/
}
