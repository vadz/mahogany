///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   PythonHelp.cpp: functions for working with Python interpreter
// Purpose:     Python C API is too low level, we wrap it in functions below
// Author:      Karsten Ballueder, Vadim Zeitlin
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2004 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// TODO: there seems to be some useless junk in this file, to reread/clean up

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include   "Mpch.h"

#ifdef USE_PYTHON

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"
#  include "strutil.h"
#endif   // USE_PCH

#include "MPython.h"

#include   "Mdefaults.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_USEPYTHON;

// from InitPython.cpp
extern PyObject *Python_MinitModule;

// ----------------------------------------------------------------------------
// M_PyObject: safe wrapper for PyObject which never forgets to Py_DECREF()
// ----------------------------------------------------------------------------

class M_PyObject
{
public:
   M_PyObject(PyObject *pyObj) : m_pyObj(pyObj) { }

   M_PyObject& operator=(PyObject *pyObj)
      { Py_XDECREF(m_pyObj); m_pyObj = pyObj; return *this; }
   operator PyObject *() const { return m_pyObj; }
   PyObject * operator->() const { return m_pyObj; }

   ~M_PyObject() { Py_XDECREF(m_pyObj); }

private:
   PyObject *m_pyObj;
};

// ============================================================================
// implementation
// ============================================================================

PyObject *PyH_LoadModule(const char *modname);            /* returns module object */

static void SWIG_MakePtr(char *c, void *ptr, const char *name) {
  static char hex[17] = "0123456789abcdef";
  unsigned long p, s;
  char result[32], *r; 
  r = result;
  p = (unsigned long) ptr;
  if (p > 0) {
    while (p > 0) {
      s = p & 0xf;
      *(r++) = hex[s];
      p = p >> 4;
    }
    *r = '_';
    while (r >= result)
      *(c++) = *(r--);
    strcpy (c, name);
  } else {
    strcpy (c, "NULL");
  }
}

int
PythonCallback(const char *name, int def, void *obj, const char *classname,
               Profile *profile, const char *argfmt,
               ...)
{
   String
      realname;
   int
      result = 0;
   PyObject
      *parg;

   // first check if Python is not disabled
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
      return def;

   va_list argslist;

   if(argfmt && *argfmt != '\0')
   {
      va_start(argslist, argfmt);
      parg = Py_VaBuildValue((char *)argfmt, argslist);     /* input: C->Python */
      if (parg == NULL)
         return 0;
   }
   else
      parg = NULL;

   if(profile)
      realname = profile->readEntry(name,realname);
   if(strutil_isempty(realname))
      realname = mApplication->GetProfile()->readEntry(name,realname);

   if(strutil_isempty(realname))
      return def;    // no callback called, default value 0

   PyH_CallFunction(realname,
                    name,
                    obj, classname,
                    "i",&result, parg);
   return result;
}

PyObject *
PyH_makeObjectFromPointer(void *ptr,const char *classname)
{
   char ptemp[256];
   String ptrtype;

   ptrtype = "_";
   ptrtype += classname;
   ptrtype += "_p";

   SWIG_MakePtr(ptemp,ptr, ptrtype.c_str());

   return Py_BuildValue("s",ptemp);
}

bool
PyH_CallFunction(const char *func,
                 const char *name,
                 void *obj,
                 const char *classname,
                 const char *resultfmt,
                 void *result,
                 PyObject *parg)
{
   // first check if Python is not disabled
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
   {
      ERRORMESSAGE(( _("Python support is disabled, please enable it in "
                       "the \"Preferences\" dialog.") ));
   }
   else
   {
      // next determine which module should we load the function from
      String functionName = func;

      // does function name contain module name?
      String modname;
      if(strchr(func,'.') != NULL)
      {
         const char *cptr = func;
         while(*cptr != '.')
            modname += *cptr++;
         functionName = "";
         cptr++;
         while(*cptr)
            functionName += *cptr++;
      }
      else // no explicit module, use the default one
      {
         modname = _T("Minit");
      }

      M_PyObject module(PyImport_ImportModule((char *)modname.c_str()));

      if ( !module )
      {
         ERRORMESSAGE(( _("Module \"%s\" couldn't be loaded."),
                        modname.c_str() ));
      }
      else
      {
         // we want to allow modifying the Python code on the fly, so try to
         // reload the module here -- this is nice for playing with Python even
         // if possibly very slow...
         PyObject *moduleRe = PyImport_ReloadModule(module);
         if ( moduleRe )
         {
            // if reloading failed, fall back to the original module
            module = moduleRe;
         }

         PyObject *rc = NULL;

         PyObject *function = PyObject_GetAttrString
                              (
                                 module,
                                 const_cast<char *>(functionName.c_str())
                              );
         if ( !function )
         {
            ERRORMESSAGE(( _("Function \"%s\" not found in module \"%s\"."),
                           functionName.c_str(), modname.c_str() ));
         }
         else // ok
         {
            // now build object reference argument:
            PyObject *object = PyH_makeObjectFromPointer(obj, classname);

            // and do call the function
            if ( parg )
               rc = PyObject_CallFunction(function, "sOO", name, object, parg);
            else
               rc = PyObject_CallFunction(function, "sO", name, object);

            if ( rc )
            {
               // translate result back to C
               PyH_ConvertResult(rc, resultfmt, result);

               return true;
            }
         }
      }

      String err = PyH_GetErrorMessage();
      if ( !err.empty() )
      {
         ERRORMESSAGE((_T("%s"), err.c_str()));
      }
   }

   ERRORMESSAGE((_("Calling Python function \"%s\" failed."), func));

   return false;
}

bool PyH_CallFunctionVa(const char *func,
                      const char *name,
                      void *obj, const char *classname,
                        const char *resultfmt, void *result,
                        const char * /* argfmt */, ...
   )
{
   // first check if Python is not disabled
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
      return false;

   PyObject
      *module,
      *function,
      *object,
      *presult;

   String
      functionName = func;

   if(strchr(func,'.') != NULL)  // function name contains module name
   {
      String modname;
      const char *cptr = func;
      while(*cptr != '.')
         modname += *cptr++;
      functionName = "";
      cptr++;
      while(*cptr)
         functionName += *cptr++;
      module = PyH_LoadModule(modname.c_str());             /* get module, init python */
   }
   else
   {
      module = Python_MinitModule;
      functionName = func;
   }

   if (module == NULL)
      return 0;  // failure

   function = PyObject_GetAttrString(module, (char *)functionName.c_str());
   if(! function)
      return 0; // failure

   // now build object reference argument:

   object = PyH_makeObjectFromPointer(obj, classname);
   presult = PyObject_CallFunction(function,"sO",name,object);

   // expr val to C
   return PyH_ConvertResult(presult, resultfmt, result) != 0;
}


PyObject *
PyH_LoadModule(const char *modname)            /* returns module object */
{
   /* 4 cases...
     * 1) module "__main__" has no file, and not prebuilt: fetch or make
     * 2) dummy modules have no files: don't try to reload them
     * 3) reload set and already loaded (on sys.modules): "reload()" before use
     * 4) not loaded yet, or loaded but reload off: "import" to fetch or load */

   PyObject *module;
   module  = PyDict_GetItemString(                 /* in sys.modules dict? */
      PyImport_GetModuleDict(), (char *)modname);

   if (strcmp(modname, "__main__") == 0)           /* no file */
      return PyImport_AddModule((char *)modname);         /* not incref'd */
   else
      if (module != NULL &&                                /* dummy: no file */
          PyDict_GetItemString(PyModule_GetDict(module), "__dummy__"))
         return module;                                   /* not incref'd */
      else
         if (module != NULL && PyModule_Check(module)) {
            module = PyImport_ReloadModule(module);   /* reload-file/run-code */
            Py_XDECREF(module);                       /* still on sys.modules */
            return module;                            /* not incref'd */
         }
         else {
            module = PyImport_ImportModule((char *)modname);  /* fetch or load module */
            Py_XDECREF(module);                       /* still on sys.modules */
            return module;                            /* not incref'd */
         }
}

int
PyH_ConvertResult(PyObject *presult, const char *resFormat, void *resTarget)
{
   if (presult == NULL)            /* error when run? */
      return 0;
   if (resTarget == NULL || resFormat == NULL || *resFormat == '\0') {        /* NULL: ignore result */
      Py_DECREF(presult);         /* procedures return None */
      return 0;
   }
   if (! PyArg_Parse(presult, (char *)resFormat, resTarget)) { /* convert Python->C */
      Py_DECREF(presult);                             /* may not be a tuple */
      return 0;                                      /* error in convert? */
   }
   if (strcmp(resFormat, "O") != 0)       /* free object unless passed-out */
      Py_DECREF(presult);
   return 1;                              /* 1=success, 0=failure */
}                                          /* if 1: result in *resTarget  */

int
PyH_RunCodestr(enum PyH_RunModes mode, const char *code,      /* expr or stmt string */
                const char *modname,                     /* loads module if needed */
                const char *resfmt, void *cresult)       /* converts expr result to C */
{
   int parse_mode;                            /* "eval(code, d, d)", or */
   PyObject *module, *dict, *presult;         /* "exec code in d, d" */

   module = PyH_LoadModule(modname);             /* get module, init python */
   if (module == NULL)                        /* not incref'd */
      return 0;
   dict = PyModule_GetDict(module);           /* get dict namespace */
   if (dict == NULL)                          /* not incref'd */
      return 0;

   parse_mode = (mode == PY_EXPRESSION ? Py_eval_input : Py_file_input);
   presult = PyRun_String((char *)code, parse_mode, dict, dict); /* eval direct */
   /* increfs res */
   if (mode == PY_STATEMENT) {
      int result = (presult == NULL? 0 : 1);          /* stmt: 'None' */
      Py_XDECREF(presult);                             /* ignore result */
      return result;
   }
   return PyH_ConvertResult(presult, resfmt, cresult);     /* expr val to C */
}

PyObject *
PyH_LoadAttribute(const char *modname, const char *attrname)
{
    PyObject *module;                         /* fetch "module.attr" */
    module  = PyH_LoadModule(modname);           /* not incref'd, may reload */
    if (module == NULL)
        return NULL;
    return PyObject_GetAttrString(module, (char *)attrname);  /* func, class, var,.. */
}                                                     /* caller must xdecref */

int
PyH_RunFunction(const char *funcname, const char *modname,          /* load from module */
             const char *resfmt,  void *cresult,           /* convert to c/c++ */
             const char *argfmt,  ... /* arg, arg... */ )  /* convert to python */
{
    PyObject *func, *args, *presult;
    va_list argslist;
    va_start(argslist, argfmt);                   /* "modname.funcname(args)" */

    func = PyH_LoadAttribute(modname, funcname);     /* reload?; incref'd */
    if (func == NULL)                             /* func may be a class */
        return 0;
    args = Py_VaBuildValue((char *)argfmt, argslist);     /* convert args to python */
    if (args == NULL) {                           /* args incref'd */
        Py_DECREF(func);
        return 0;
    }

    presult = PyEval_CallObject(func, args);   /* run function; incref'd */

    Py_DECREF(func);
    Py_DECREF(args);                                  /* result may be None */
    return PyH_ConvertResult(presult, resfmt, cresult);  /* convert result to C */
}

bool
PyH_RunScript(const char *filename)
{
   // first check if Python is not disabled
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
   {
      // normally we shouldn't even get here
      FAIL_MSG( _T("Python is disabled, can't run script!") );

      return false;
   }

   // the canonical Python API for what we want is PyRun_SimpleFile() but we
   // can't use it here because it takes a "FILE *" and STDIO data structures
   // are compiler and build (debug/release) dependent and so this call crashes
   // unless the program was built using the same settings as python.dll which
   // is unacceptable for us
   //
   // so emulate this call manually
   String execCmd = _T("execfile('");

   // backslashes are special in Python strings, escape them (could also use
   // raw strings...)
   for ( const char *p = filename; *p; p++ )
   {
      if ( *p == _T('\\') || *p == _T('\'') )
         execCmd += _T('\\');

      execCmd += *p;
   }

   execCmd += _T("')");

   // PyImport_AddModule() and PyModule_GetDict() return borrowed references so
   // we shouldn't free them
   PyObject *dict;
   PyObject *mainMod = PyImport_AddModule("__main__");
   if ( mainMod )
      dict = PyModule_GetDict(mainMod);
   else
      dict = NULL;

   if ( !dict )
   {
      // this is not supposed to ever happen but what can we do if it does??
      ERRORMESSAGE((_("Fatal Python problem: no main module dictionary.")));

      return false;
   }

   M_PyObject rc = PyRun_String(const_cast<char *>(execCmd.mb_str()),
                                Py_file_input, dict, dict);
   if ( !rc )
   {
      String err = PyH_GetErrorMessage();
      if ( !err.empty() )
      {
         ERRORMESSAGE((_T("%s"), err.c_str()));
      }

      ERRORMESSAGE((_("Execution of the Python script \"%s\" failed."),
                    filename));

      return false;
   }

   return true;
}


bool
PyH_RunMScript(const char *scriptname)
{
   wxString filename = mApplication->GetGlobalDir();
   if ( filename.empty() )
      filename = mApplication->GetDataDir();

   filename << DIR_SEPARATOR << _T("scripts") << DIR_SEPARATOR << scriptname;

   return PyH_RunScript(filename);
}

#define GPEM_ERROR(what) {errorMsg = "<Error getting traceback - " what ">";goto done;}


String PyH_GetErrorMessage()
{
   PyObject *exc_typ, *exc_val, *exc_tb;
   /* Fetch the error state now before we cruch it */
   PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
   if ( !exc_typ )
   {
      return String();
   }

   char *result = NULL;
   char *errorMsg = NULL;
   PyObject *modStringIO = NULL;
   PyObject *modTB = NULL;
   PyObject *obFuncStringIO = NULL;
   PyObject *obStringIO = NULL;
   PyObject *obFuncTB = NULL;
   PyObject *argsTB = NULL;
   PyObject *obResult = NULL;

   /* Import the modules we need - StringIO and traceback */
   modStringIO = PyImport_ImportModule("StringIO");
   if (modStringIO==NULL) GPEM_ERROR("cant import StringIO");
   modTB = PyImport_ImportModule("traceback");
   if (modTB==NULL) GPEM_ERROR("cant import traceback");

   /* Construct a StringIO object */
   obFuncStringIO = PyObject_GetAttrString(modStringIO, "StringIO");
   if (obFuncStringIO==NULL) GPEM_ERROR("cant find StringIO.StringIO");
   obStringIO = PyObject_CallObject(obFuncStringIO, NULL);
   if (obStringIO==NULL) GPEM_ERROR("StringIO.StringIO() failed");

   /* Get the traceback.print_exception function, and call it. */
   obFuncTB = PyObject_GetAttrString(modTB, "print_exception");
   if (obFuncTB==NULL) GPEM_ERROR("cant find traceback.print_exception");
   argsTB = Py_BuildValue("OOOOO",
                          exc_typ ? exc_typ : Py_None,
                          exc_val ? exc_val : Py_None,
                          exc_tb  ? exc_tb  : Py_None,
                          Py_None,
                          obStringIO);
   if (argsTB==NULL) GPEM_ERROR("cant make print_exception arguments");

   obResult = PyObject_CallObject(obFuncTB, argsTB);
   if (obResult==NULL) GPEM_ERROR("traceback.print_exception() failed");

   /* Now call the getvalue() method in the StringIO instance */
   Py_DECREF(obFuncStringIO);
   obFuncStringIO = PyObject_GetAttrString(obStringIO, "getvalue");
   if (obFuncStringIO==NULL) GPEM_ERROR("cant find getvalue function");
   Py_DECREF(obResult);
   obResult = PyObject_CallObject(obFuncStringIO, NULL);
   if (obResult==NULL) GPEM_ERROR("getvalue() failed.");

   result = strutil_strdup(PyString_AsString(obResult));
 done:
   if (result==NULL && errorMsg != NULL)
      result = strutil_strdup(errorMsg);
   Py_XDECREF(modStringIO);
   Py_XDECREF(modTB);
   Py_XDECREF(obFuncStringIO);
   Py_XDECREF(obStringIO);
   Py_XDECREF(obFuncTB);
   Py_XDECREF(argsTB);
   Py_XDECREF(obResult);

   /* Restore the exception state */
   String errString;
   PyErr_Restore(exc_typ, exc_val, exc_tb);
   if(result)
      errString = result;
   free(result);

   return errString;
}

#endif // USE_PYTHON

