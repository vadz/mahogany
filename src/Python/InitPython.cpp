/*-*- c++ -*-********************************************************
 * InitPython.cc: initialisation of the Python interpreter          *
 *                                                                  *
 * (C) 1998by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
 * Revision 1.1  1998/05/02 18:31:38  KB
 * Python integration works.
 *
 *
 *******************************************************************/

#include     "Mpch.h"

#ifndef   USE_PYTHON
void InitPython(void)
{
}
#else

#ifndef   USE_PCH
#   include   "Mdefaults.h"
#   include   "MApplication.h"
#endif

#include   "Python.h"

// the module initialisations
extern "C"
{
   void initMTestc();
   void initProfilec();
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
   tmp = mApplication.GetGlobalDir();
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
   initMTestc();
   initProfilec();
   initMApplicationc();
   
   // do some testing
   PyRun_SimpleString("import sys,os");
   PyRun_SimpleString("print 'Hello,', os.environ['USER'] + '.'");
   // run the init script
   PyRun_SimpleString("import Minit");
}
#endif // USE_PYTHON
