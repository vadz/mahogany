/*-*- c++ -*-********************************************************
 * PythonHelp.cc - Helper functions for the python interface        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
 * Revision 1.1  1998/05/24 14:47:34  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 *
 *******************************************************************/

#include   "Mconfig.h"  // for correct evaluation of Python.h
#include   "Python.h"
#include   "PythonHelp.h"

static
PyObject *
PyH_LoadModule(const char *modname)            /* returns module object */
{
   /* 4 cases...
     * 1) module "__main__" has no file, and not prebuilt: fetch or make
     * 2) dummy modules have no files: don't try to reload them
     * 3) reload set and already loaded (on sys.modules): "reload()" before use
     * 4) not loaded yet, or loaded but reload off: "import" to fetch or load */

   PyObject *module, *result;                  
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
   if (resTarget == NULL) {        /* NULL: ignore result */
      Py_DECREF(presult);         /* procedures return None */
      return 1;
   }
   if (! PyArg_Parse(presult, (char *)resFormat, resTarget)) { /* convert Python->C */
      Py_DECREF(presult);                             /* may not be a tuple */
      return 0;                                      /* error in convert? */
   }
   if (strcmp(resFormat, "O") != 0)       /* free object unless passed-out */
      Py_DECREF(presult);
   return 1;                              /* 0=success, -1=failure */
}                                          /* if 0: result in *resTarget  */

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
