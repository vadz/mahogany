///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   PythonDll.cpp - stubs for Python functions
// Purpose:     to allow real dynamic loading of Python.dll under Windows
//              we resolve the Python functions we use at run-time, otherwise
//              we'd need to always distribute Python.dll with Mahogany
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.03.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
    #include "Mcommon.h"
#endif // USE_PCH

#include "MPython.h"

#if defined(USE_PYTHON) && defined(USE_PYTHON_DYNAMIC)

#include <wx/dynlib.h>

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// define the macro to return the name of the Python library
#if defined(OS_WIN)
    // we have to use the debug DLL in debug build under Windows
    #ifdef DEBUG
        #define PYTHON_LIB(ver)     "python" #ver "_d"
    #else  // !Debug
        #define PYTHON_LIB(ver)     "python" #ver
    #endif // Debug/!Debug
#else // !Win
    #define PYTHON_LIB(ver)         "libpython" #ver
#endif // Win/!Win

inline String GetDllName(const String& basename)
{
    return basename + wxDllLoader::GetDllExt();
}

#ifdef OS_WIN
    #define PYTHON_PROC FARPROC
#else
    #define PYTHON_PROC void *
#endif

// ----------------------------------------------------------------------------
// global data
// ----------------------------------------------------------------------------

extern "C"
{
   PyObject* (*M_Py_VaBuildValue)(char *, va_list) = NULL;
   void (*M__Py_Dealloc)(PyObject *) = NULL;
   void (*M_PyErr_Fetch)(PyObject **, PyObject **, PyObject **) = NULL;
   void (*M_PyErr_Restore)(PyObject *, PyObject *, PyObject *) = NULL;
   void (*M_PyErr_Print)(void) = NULL;
   int(*M_PyArg_Parse)(PyObject *, char *, ...) = NULL;
   int(*M_PyArg_ParseTuple)(PyObject *, char *, ...) = NULL;
   int(*M_PyDict_SetItemString)(PyObject *dp, char *key, PyObject *item) = NULL;
   int(*M_PyErr_BadArgument)(void) = NULL;
   void(*M_PyErr_Clear)(void) = NULL;
   PyObject*(*M_PyErr_NoMemory)(void) = NULL;
   PyObject*(*M_PyErr_Occurred)(void) = NULL;
   void(*M_PyErr_SetNone)(PyObject *) = NULL;
   void(*M_PyErr_SetString)(PyObject *, const char *) = NULL;
   void(*M_PyEval_RestoreThread)(PyThreadState *) = NULL;
   PyThreadState*(*M_PyEval_SaveThread)(void) = NULL;
   long(*M_PyInt_AsLong)(PyObject *) = NULL;
   PyObject*(*M_PyInt_FromLong)(long) = NULL;
   PyTypeObject* M_PyInt_Type = NULL;
   PyObject*(*M_PyList_GetItem)(PyObject *, int) = NULL;
   PyObject*(*M_PyList_New)(int size) = NULL;
   int(*M_PyList_SetItem)(PyObject *, int, PyObject *) = NULL;
   int(*M_PyList_Size)(PyObject *) = NULL;
   PyTypeObject* M_PyList_Type = NULL;
   PyObject*(*M_PyImport_ImportModule)(const char *) = NULL;
   PyObject*(*M_PyDict_GetItemString)(PyObject *, const char *) = NULL;
   PyObject*(*M_PyModule_GetDict)(PyObject *) = NULL;
   int(*M_PyRun_SimpleString)(char *) = NULL;
   char*(*M_PyString_AsString)(PyObject *) = NULL;
   PyObject*(*M_PyString_FromString)(const char *) = NULL;
   PyObject*(*M_PyString_FromStringAndSize)(const char *, int) = NULL;
   int(*M_PyString_Size)(PyObject *) = NULL;
   PyTypeObject* M_PyString_Type = NULL;
   int(*M_PySys_SetObject)(char *, PyObject *) = NULL;
   PyTypeObject* M_PyType_Type = NULL;
   PyObject*(*M_Py_BuildValue)(char *, ...) = NULL;
   PyObject*(*M_Py_FindMethod)(PyMethodDef[], PyObject *, char *) = NULL;
   PyObject*(*M_Py_InitModule4)(char *, PyMethodDef *, char *, PyObject *, int) = NULL;
   void(*M_Py_Initialize)(void) = NULL;
   PyObject*(*M__PyObject_New)(PyTypeObject *, PyObject *) = NULL;
   PyObject*(*M__PyObject_Init)(PyObject *, PyTypeObject *) = NULL;
   PyObject *(*M_PyEval_CallObjectWithKeywords)(PyObject *, PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyFloat_FromDouble)(double) = NULL;
   PyObject *(*M_PyImport_AddModule)(char *name) = NULL;
   PyObject *(*M_PyImport_GetModuleDict)(void) = NULL;
   PyObject *(*M_PyImport_ReloadModule)(PyObject *) = NULL;
   PyObject *(*M_PyObject_CallFunction)(PyObject *, char *format, ...) = NULL;
   PyObject *(*M_PyObject_CallObject)(PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyObject_GetAttr)(PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyObject_GetAttrString)(PyObject *, char *) = NULL;
   int (*M_PyRun_SimpleFile)(FILE *, char *) = NULL;
   PyObject *(*M_PyRun_String)(char *, int, PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyString_InternFromString)(const char *) = NULL;

   // variables
   long M__Py_RefTotal = 0;
   PyTypeObject *M_PyModule_Type = NULL;
   PyObject* M__Py_NoneStruct = NULL;

   // exception objects
   PyObject *M_PyExc_NameError = NULL;
   PyObject *M_PyExc_TypeError = NULL;
}

// ----------------------------------------------------------------------------
// private data
// ----------------------------------------------------------------------------

static struct
{
    const char *name;
    PYTHON_PROC *ptr;
} pythonFuncs[] =
{
   { "Py_Initialize", (PYTHON_PROC *)&M_Py_Initialize },
   { "_Py_NoneStruct", (PYTHON_PROC *)&M__Py_NoneStruct},
#ifdef Py_TRACE_REFS
   { "Py_InitModule4TraceRefs", (PYTHON_PROC *)&M_Py_InitModule4 },
#else
   { "Py_InitModule4", (PYTHON_PROC *)&M_Py_InitModule4 },
#endif
   { "Py_BuildValue", (PYTHON_PROC *)&M_Py_BuildValue },
   { "Py_VaBuildValue", (PYTHON_PROC *)&M_Py_VaBuildValue },
   { "_Py_RefTotal", (PYTHON_PROC *)&M__Py_RefTotal },
   { "PyModule_Type", (PYTHON_PROC *)&M_PyModule_Type },
   { "_Py_Dealloc", (PYTHON_PROC *)&M__Py_Dealloc },
   { "PyErr_Fetch", (PYTHON_PROC *)&M_PyErr_Fetch },
   { "PyErr_Restore", (PYTHON_PROC *)&M_PyErr_Restore },
   { "PyErr_Print", (PYTHON_PROC *)&M_PyErr_Print },
   { "PyArg_Parse", (PYTHON_PROC *)&M_PyArg_Parse },
   { "PyArg_ParseTuple", (PYTHON_PROC *)&M_PyArg_ParseTuple },
   { "PyDict_GetItemString", (PYTHON_PROC *)&M_PyDict_GetItemString },
   { "PyDict_SetItemString", (PYTHON_PROC *)&M_PyDict_SetItemString },
   { "PyErr_Occurred", (PYTHON_PROC *)&M_PyErr_Occurred },
   { "PyErr_SetString", (PYTHON_PROC *)&M_PyErr_SetString },
   { "PyInt_FromLong", (PYTHON_PROC *)&M_PyInt_FromLong },
   { "PyImport_ImportModule", (PYTHON_PROC *)&M_PyImport_ImportModule },
   { "PyModule_GetDict", (PYTHON_PROC *)&M_PyModule_GetDict },
   { "PyString_AsString", (PYTHON_PROC *)&M_PyString_AsString },
   { "PyString_FromString", (PYTHON_PROC *)&M_PyString_FromString },
   { "PyEval_CallObjectWithKeywords", (PYTHON_PROC *)&M_PyEval_CallObjectWithKeywords },
   { "PyExc_NameError", (PYTHON_PROC *)&M_PyExc_NameError },
   { "PyExc_TypeError", (PYTHON_PROC *)&M_PyExc_TypeError },
   { "PyFloat_FromDouble", (PYTHON_PROC *)&M_PyFloat_FromDouble },
   { "PyImport_AddModule", (PYTHON_PROC *)&M_PyImport_AddModule },
   { "PyImport_GetModuleDict", (PYTHON_PROC *)&M_PyImport_GetModuleDict },
   { "PyImport_ReloadModule", (PYTHON_PROC *)&M_PyImport_ReloadModule },
   { "PyObject_CallFunction", (PYTHON_PROC *)&M_PyObject_CallFunction },
   { "PyObject_CallObject", (PYTHON_PROC *)&M_PyObject_CallObject },
   { "PyObject_GetAttr", (PYTHON_PROC *)&M_PyObject_GetAttr },
   { "PyObject_GetAttrString", (PYTHON_PROC *)&M_PyObject_GetAttrString },
   { "PyRun_SimpleFile", (PYTHON_PROC *)&M_PyRun_SimpleFile },
   { "PyRun_String", (PYTHON_PROC *)&M_PyRun_String },
   { "PyString_InternFromString", (PYTHON_PROC *)&M_PyString_InternFromString },
   {"", NULL}
};

// the handle of Python DLL
static wxDllType gs_dllPython = 0;

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool InitPythonDll()
{
   CHECK( !gs_dllPython, true, "shouldn't be called more than once" );

   gs_dllPython = wxDllLoader::LoadLibrary(GetDllName(PYTHON_LIB(20)));
   if ( !gs_dllPython )
   {
      gs_dllPython = wxDllLoader::LoadLibrary(GetDllName(PYTHON_LIB(15)));
   }

   if ( !gs_dllPython )
   {
      return false;
   }

   // load all functions
   for ( size_t i = 0; pythonFuncs[i].ptr; i++ )
   {
      void *funcptr = wxDllLoader::GetSymbol(gs_dllPython, pythonFuncs[i].name);
      if ( !funcptr )
      {
         FreePythonDll();

         return false;
      }

      *pythonFuncs[i].ptr = (PYTHON_PROC)funcptr;
   }

   return true;
}

extern void FreePythonDll()
{
   if ( gs_dllPython )
   {
      wxDllLoader::UnloadLibrary(gs_dllPython);
      gs_dllPython = 0;
   }
}

#endif // USE_PYTHON_DYNAMIC

