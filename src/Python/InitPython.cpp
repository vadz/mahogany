/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
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
#include   "Python.h"
#include   "PythonHelp.h"

#ifndef   USE_PCH
#   include   "Mdefaults.h"
#   include   "MApplication.h"
#endif


// the module initialisations
extern "C"
{
   void initStringc();
   void initProfilec();
   void initMailFolderc();
   void initMApplicationc();
};

void
InitPython(void)
{
   String
      tmp;

   // initialise python interpreter
   tmp = "";
   tmp += mApplication.GetLocalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp += mApplication.GetGlobalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp += mApplication.readEntry(MC_PYTHONPATH,MC_PYTHONPATH_D);
   if(getenv("PYTHONPATH"))
   {
      tmp += PATH_SEPARATOR;
      tmp += getenv("PYTHONPATH");
   }
   setenv("PYTHONPATH", tmp.c_str(), 1);

   // initialise the interpreter
   Py_Initialize();
      
   // initialise the modules
   initStringc();
   initProfilec();
   initMailFolderc();
   initMApplicationc();

   // run the init script
   PyRun_SimpleString("from Minit import *");
   PyRun_SimpleString("from MApplication import *");
   PyRun_SimpleString("from String import *");

   // try running a method:
   PyObject *pobj, *presult;
   char *cptr = NULL;
   if(PyH_Expression("mApplication.GetGlobalDir()", "MApplication", "O", &pobj))
   {
      presult = PyObject_CallMethod(pobj, "c_str","");
      if(presult && PyH_ConvertResult(presult, "s", &cptr))
         VAR(cptr);
   }

   PyH_Statement("print \"Hello World!\"","Minit","",NULL);
   PyH_Statement("callback_func","Minit","",NULL);
   
   // this doesn't work:
   //pobj = PyObject_GetAttrString(PyImport_ImportModule("Minit"),"Minit");
   //PyObject_CallFunction(pobj,"Minit","",NULL);
      
   //PyH_RunFunction("callback_func","Minit","",NULL,"",NULL);

}
