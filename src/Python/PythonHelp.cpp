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

#include "Mdefaults.h"

extern "C"
{
   struct swig_type_info;
   extern swig_type_info *SWIG_TypeQuery(const char *);
   extern PyObject *SWIG_NewPointerObj(void *, swig_type_info *, int);
}

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_USEPYTHON;

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

// ----------------------------------------------------------------------------
// private functions prototypes
// ----------------------------------------------------------------------------

/**
   Converts a PyObject to a C++ variable in a safe way.

   This function takes ownership of pyResult (i.e. it calls Py_DECREF() on it if
   it is not non-NULL) unless it is returned in result.

   @param pyResult the object to convert
   @param format the format string
   @param result where to store it
   @return true if ok, false on error
*/
static bool
PythonConvertResultToC(PyObject *pyResult, const char *format, void *result);

// ============================================================================
// implementation
// ============================================================================

bool
PythonConvertResultToC(PyObject *pyResult, const char *format, void *result)
{
   CHECK( result && format && format[0], false,
            _T("PythonConvertResultToC: invalid parameters") );

   if ( !pyResult )
      return false;

   // try to convert
   if ( !PyArg_Parse(pyResult, (char *)format, result) )
   {
      Py_DECREF(pyResult);
      return false;
   }

   // free the object unless we return it as our result
   if ( format[0] != 'O' )
      Py_DECREF(pyResult);

   return true;
}

int
PythonCallback(const char *name,
               int def,
               void *obj,
               const char *classname,
               Profile *profile)
{
   // first check if Python is not disabled
   if ( !READ_APPCONFIG(MP_USEPYTHON) )
      return def;

   if ( !profile )
   {
      profile = mApplication->GetProfile();
      if ( !profile )
      {
         // called too early during app startup?
         return def;
      }
   }

   String nameCB(profile->readEntry(name, _T("")));

   if ( nameCB.empty() )
   {
      // no callback called
      return def;
   }

   // do call it
   int result = 0;
   if ( !PythonFunction(nameCB, obj, classname, "i", &result) )
   {
      // calling Python function failed
      return def;
   }

   return result;
}

bool
PythonFunction(const char *func,
                   void *obj,
                   const char *classname,
                   const char *resultfmt,
                   void *result)
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
         modname = MP_PYTHONMODULE_TO_LOAD_DEFVAL;
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
            String ptrCls(classname);
            ptrCls += _T(" *");
            PyObject *object = SWIG_NewPointerObj(obj, SWIG_TypeQuery(ptrCls), 0);

            // and do call the function
            PyObject *rc = PyObject_CallFunction(function, "O", object);

            // translate result back to C
            if ( PythonConvertResultToC(rc, resultfmt, result) )
            {
               // everything ok, skip error messages below
               return true;
            }
         }
      }

      String err = PythonGetErrorMessage();
      if ( !err.empty() )
      {
         ERRORMESSAGE((_T("%s"), err.c_str()));
      }
   }

   ERRORMESSAGE((_("Calling Python function \"%s\" failed."), func));

   return false;
}

bool
PythonRunScript(const char *filename)
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
      String err = PythonGetErrorMessage();
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


String PythonGetErrorMessage()
{
   PyObject *exc_typ, *exc_val, *exc_tb;
   /* Fetch the error state now before we cruch it */
   PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
   if ( !exc_typ )
   {
      return String();
   }

#define GPEM_ERROR(what) {errorMsg = "<Error getting traceback - " what ">";goto done;}

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

