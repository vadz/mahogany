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

#ifdef USE_PYTHON_DYNAMIC

#include <wx/dynlib.h>

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

// define the function to return the name of the Python library
inline String GetPythonDllBaseName(const String& version)
{
   String
#if defined(OS_WIN)
      basename = _T("python");
#elif defined(OS_UNIX)
      basename = _T("libpython");
#else
    #error "Unknown Python library naming convention under this OS"
#endif // OS

   return basename + version;
}

// ----------------------------------------------------------------------------
// global data
// ----------------------------------------------------------------------------

extern "C"
{
   // startup/shutdown
   void(*M_Py_Initialize)(void) = NULL;
   void(*M_Py_Finalize)(void) = NULL;

   // errors
   //int(*M_PyErr_BadArgument)(void) = NULL;
   void (*M_PyErr_Clear)(void) = NULL;
   void (*M_PyErr_Fetch)(PyObject **, PyObject **, PyObject **) = NULL;
   //PyObject*(*M_PyErr_NoMemory)(void) = NULL;
   PyObject*(*M_PyErr_Occurred)(void) = NULL;
   void (*M_PyErr_Restore)(PyObject *, PyObject *, PyObject *) = NULL;
   //void(*M_PyErr_SetNone)(PyObject *) = NULL;
   void(*M_PyErr_SetString)(PyObject *, const char *) = NULL;

   // objects
   //PyObject*(*M__PyObject_New)(PyTypeObject *, PyObject *) = NULL;
   //PyObject*(*M__PyObject_Init)(PyObject *, PyTypeObject *) = NULL;
   PyObject *(*M_PyObject_CallFunction)(PyObject *, char *format, ...) = NULL;
   PyObject *(*M_PyObject_CallObject)(PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyObject_GetAttr)(PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyObject_GetAttrString)(PyObject *, char *) = NULL;
   void *(*M_PyObject_Malloc)(size_t) = NULL;
   void (*M_PyObject_Free)(void *) = NULL;
   int (*M_PyObject_SetAttrString)(PyObject *, char *, PyObject *) = NULL;
   int (*M_PyObject_Size)(PyObject *) = NULL;

   // ints
   long(*M_PyInt_AsLong)(PyObject *) = NULL;
   PyObject*(*M_PyInt_FromLong)(long) = NULL;
   PyTypeObject *M_PyInt_Type = NULL;

   // longs
   PyTypeObject *M_PyLong_Type = NULL;

   // strings
   char*(*M_PyString_AsString)(PyObject *) = NULL;
   int (*M_PyString_AsStringAndSize)(PyObject *, char **, int *) = NULL;
   PyObject*(*M_PyString_FromString)(const char *) = NULL;
   PyObject*(*M_PyString_FromStringAndSize)(const char *, int) = NULL;
   int(*M_PyString_Size)(PyObject *) = NULL;
   PyTypeObject* M_PyString_Type = NULL;
   PyObject *(*M_PyString_InternFromString)(const char *) = NULL;

   // tuples
   PyObject *(*M_PyTuple_GetItem)(PyObject *, int) = NULL;

   // ...
   PyObject* (*M_Py_VaBuildValue)(char *, va_list) = NULL;
#ifdef Py_TRACE_REFS
   void (*M__Py_Dealloc)(PyObject *) = NULL;
#endif // Py_TRACE_REFS
   int(*M_PyArg_Parse)(PyObject *, char *, ...) = NULL;
   int(*M_PyArg_ParseTuple)(PyObject *, char *, ...) = NULL;
   int(*M_PyDict_SetItemString)(PyObject *dp, char *key, PyObject *item) = NULL;
   void(*M_PyEval_RestoreThread)(PyThreadState *) = NULL;
   PyThreadState*(*M_PyEval_SaveThread)(void) = NULL;
   PyObject*(*M_PyList_GetItem)(PyObject *, int) = NULL;
   PyObject*(*M_PyList_New)(int size) = NULL;
   int(*M_PyList_SetItem)(PyObject *, int, PyObject *) = NULL;
   int(*M_PyList_Size)(PyObject *) = NULL;
   PyTypeObject* M_PyList_Type = NULL;
   PyObject*(*M_PyImport_ImportModule)(const char *) = NULL;
   PyObject*(*M_PyDict_GetItemString)(PyObject *, const char *) = NULL;
   PyObject*(*M_PyModule_GetDict)(PyObject *) = NULL;
   int(*M_PySys_SetObject)(char *, PyObject *) = NULL;
   PyTypeObject* M_PyType_Type = NULL;
   PyObject*(*M_Py_BuildValue)(char *, ...) = NULL;
   PyObject*(*M_Py_FindMethod)(PyMethodDef[], PyObject *, char *) = NULL;
   PyObject*(*M_Py_InitModule4)(char *, PyMethodDef *, char *, PyObject *, int) = NULL;
   PyObject *(*M_PyEval_CallObjectWithKeywords)(PyObject *, PyObject *, PyObject *) = NULL;
   PyObject *(*M_PyFloat_FromDouble)(double) = NULL;
   PyObject *(*M_PyImport_AddModule)(char *name) = NULL;
   PyObject *(*M_PyImport_GetModuleDict)(void) = NULL;
   PyObject *(*M_PyImport_ReloadModule)(PyObject *) = NULL;
   PyObject *(*M_PyRun_String)(const char *, int, PyObject *, PyObject *) = NULL;

   int (*M_PyType_IsSubtype)(PyTypeObject *, PyTypeObject *) = NULL;
   void (*M__Py_NegativeRefcount)(const char *, int, PyObject *) = NULL;

   // variables
#ifdef Py_REF_DEBUG
   long M__Py_RefTotal = 0;
#endif // Py_REF_DEBUG
   PyTypeObject *M_PyModule_Type = NULL;
   PyObject* M__Py_NoneStruct = NULL;

   // exception objects
   PyObject *M_PyExc_NameError = NULL;
   PyObject *M_PyExc_TypeError = NULL;
}

// ----------------------------------------------------------------------------
// private data
// ----------------------------------------------------------------------------

#ifdef OS_WIN
    #define PYTHON_PROC FARPROC
#else
    #define PYTHON_PROC void *
#endif

#define PYTHON_FUNC(func) { _T(#func), NULL, (PYTHON_PROC *)&M_ ## func },
#define PYTHON_FUNC_ALT(func, alt) { _T(#func), _T(#alt), (PYTHON_PROC *)&M_ ## func },

static struct PythonFunc
{
    const wxChar *name;       // normal function name
    const wxChar *nameAlt;    // alternative name for Debug build
    PYTHON_PROC *ptr;         // function pointer
} pythonFuncs[] =
{
   // startup/shutdown
   PYTHON_FUNC(Py_Initialize)
   PYTHON_FUNC(Py_Finalize)

   // errors
   PYTHON_FUNC(PyErr_Clear)
   PYTHON_FUNC(PyErr_Fetch)
   PYTHON_FUNC(PyErr_Occurred)
   PYTHON_FUNC(PyErr_Restore)
   PYTHON_FUNC(PyErr_SetString)

   // objects
   PYTHON_FUNC(PyObject_CallFunction)
   PYTHON_FUNC(PyObject_CallObject)
   PYTHON_FUNC(PyObject_GetAttr)
   PYTHON_FUNC(PyObject_GetAttrString)
   PYTHON_FUNC(PyObject_Malloc)
   PYTHON_FUNC(PyObject_Free)
   PYTHON_FUNC(PyObject_SetAttrString)
   PYTHON_FUNC(PyObject_Size)

   // ints
   PYTHON_FUNC(PyInt_AsLong)
   PYTHON_FUNC(PyInt_FromLong)
   PYTHON_FUNC(PyInt_Type)

   // longs
   PYTHON_FUNC(PyLong_Type)

   // strings
   PYTHON_FUNC(PyString_AsString)
   PYTHON_FUNC(PyString_AsStringAndSize)
   PYTHON_FUNC(PyString_FromString)
   PYTHON_FUNC(PyString_FromStringAndSize)
   PYTHON_FUNC(PyString_InternFromString)
   PYTHON_FUNC(PyString_Type)

   // tuples
   PYTHON_FUNC(PyTuple_GetItem)

   // ...
   PYTHON_FUNC(_Py_NoneStruct)
   PYTHON_FUNC_ALT(Py_InitModule4, Py_InitModule4TraceRefs)
   PYTHON_FUNC(Py_BuildValue)
   PYTHON_FUNC(Py_VaBuildValue)
#ifdef Py_REF_DEBUG
   PYTHON_FUNC(_Py_RefTotal)
#endif // Py_REF_DEBUG
   PYTHON_FUNC(PyModule_Type)
#ifdef Py_TRACE_REFS
   PYTHON_FUNC(_Py_Dealloc)
#endif // Py_TRACE_REFS

   PYTHON_FUNC(PyArg_Parse)
   PYTHON_FUNC(PyArg_ParseTuple)
   PYTHON_FUNC(PyDict_GetItemString)
   PYTHON_FUNC(PyDict_SetItemString)
   PYTHON_FUNC(PyImport_ImportModule)
   PYTHON_FUNC(PyModule_GetDict)
   PYTHON_FUNC(PyEval_CallObjectWithKeywords)
   PYTHON_FUNC(PyExc_NameError)
   PYTHON_FUNC(PyExc_TypeError)
   PYTHON_FUNC(PyFloat_FromDouble)
   PYTHON_FUNC(PyImport_AddModule)
   PYTHON_FUNC(PyImport_GetModuleDict)
   PYTHON_FUNC(PyImport_ReloadModule)
   PYTHON_FUNC(PyRun_String)
   PYTHON_FUNC(PyType_IsSubtype)
   { "", NULL }
};

// the handle of Python DLL
static wxDllType gs_dllPython = 0;

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool InitPythonDll()
{
   CHECK( !gs_dllPython, true, _T("shouldn't be called more than once") );

   // load the library
   static const wxChar *versions[] =
   {
#ifdef OS_WIN
      _T("23"), _T("22"), _T("20"), _T("15")
#elif defined(OS_UNIX)
      _T("2.3"), _T("2.2"), _T("2.0"), _T("1.5")
#else
    #error "Unknown Python library naming convention under this OS"
#endif
   };

   wxDynamicLibrary dllPython;

   // don't give errors about missing DLL here
   {
      wxLogNull noLog;
      for ( size_t nVer = 0; nVer < WXSIZEOF(versions); nVer++ )
      {
         String name = GetPythonDllBaseName(versions[nVer]);
         const String ext = wxDynamicLibrary::GetDllExt();

         if ( dllPython.Load(name + ext) )
            break;

#ifdef OS_WIN
         // also try debug version of the DLL
         if ( dllPython.Load(name + _T("_d") + ext) )
            break;
#endif // OS_WIN
      }
   }

   if ( dllPython.IsLoaded() )
   {
      // load all functions
      void *funcptr;
      PythonFunc *pf = pythonFuncs;
      while ( pf->ptr )
      {
         if ( pf->nameAlt )
         {
            // try alt name and fail silently if it is not found
            wxLogNull noLog;

            funcptr = dllPython.GetSymbol(pf->nameAlt);
         }
         else
         {
            funcptr = NULL;
         }

         if ( !funcptr )
         {
            funcptr = dllPython.GetSymbol(pf->name);
         }

         if ( !funcptr )
         {
            // error message is already given by GetSymbol()
            FreePythonDll();

            break;
         }

         *(pf++->ptr) = (PYTHON_PROC)funcptr;
      }

      if ( !pf->ptr )
      {
         gs_dllPython = dllPython.Detach();
      }
   }

   return gs_dllPython != NULL;
}

extern void FreePythonDll()
{
   if ( gs_dllPython )
   {
      wxDynamicLibrary::Unload(gs_dllPython);
      gs_dllPython = 0;
   }
}

#endif // USE_PYTHON_DYNAMIC

