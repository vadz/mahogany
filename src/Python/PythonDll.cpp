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
    #include "Mdefaults.h"
    #include "MApplication.h"
#endif // USE_PCH

#include "MPython.h"

#ifdef USE_PYTHON_DYNAMIC

#include <wx/dynlib.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_PYTHONDLL;

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

#define M_PY_WRAPPER_DEF(name) M_##name##_t M_##name = NULL

// use 0 and not NULL as not all variables are pointers
#define M_PY_VAR_DEF(name) M_##name##_t M_##name = 0

extern "C"
{
   M_PY_WRAPPER_DEF(Py_Initialize);
   M_PY_WRAPPER_DEF(Py_Finalize);

   // errors
   //M_PY_WRAPPER_DEF(PyErr_BadArgument);
   M_PY_WRAPPER_DEF(PyErr_Clear);
   M_PY_WRAPPER_DEF(PyErr_Fetch);
   //M_PY_WRAPPER_DEF(PyErr_NoMemory);
   M_PY_WRAPPER_DEF(PyErr_Occurred);
   M_PY_WRAPPER_DEF(PyErr_Restore);
   //M_PY_WRAPPER_DEF(PyErr_SetNone);
   M_PY_WRAPPER_DEF(PyErr_SetString);
   M_PY_WRAPPER_DEF(PyErr_Format);

   // objects
   M_PY_WRAPPER_DEF(PyObject_Init);
   M_PY_WRAPPER_DEF(PyObject_CallFunction);
   M_PY_WRAPPER_DEF(PyObject_CallObject);
   M_PY_WRAPPER_DEF(PyObject_GetAttr);
   M_PY_WRAPPER_DEF(PyObject_GetAttrString);
   M_PY_WRAPPER_DEF(PyObject_Malloc);
   M_PY_WRAPPER_DEF(PyObject_Free);
   M_PY_WRAPPER_DEF(PyObject_SetAttrString);
   M_PY_WRAPPER_DEF(PyObject_Size);
   M_PY_WRAPPER_DEF(PyObject_Str);
   M_PY_WRAPPER_DEF(PyCObject_Import);
   M_PY_WRAPPER_DEF(PyCObject_FromVoidPtr);

   // ints and longs
   M_PY_WRAPPER_DEF(PyInt_AsLong);
   M_PY_WRAPPER_DEF(PyInt_FromLong);
   M_PY_WRAPPER_DEF(PyLong_FromUnsignedLong);
   M_PY_WRAPPER_DEF(PyLong_FromVoidPtr);
   M_PY_WRAPPER_DEF(PyLong_AsLong);
   M_PY_WRAPPER_DEF(PyLong_AsUnsignedLong);
   M_PY_VAR_DEF(PyInt_Type);
   M_PY_VAR_DEF(PyLong_Type);
   M_PY_VAR_DEF(_Py_TrueStruct);
   M_PY_VAR_DEF(_Py_ZeroStruct);

   // strings
   M_PY_WRAPPER_DEF(PyString_AsString);
   M_PY_WRAPPER_DEF(PyString_AsStringAndSize);
   M_PY_WRAPPER_DEF(PyString_Format);
   M_PY_WRAPPER_DEF(PyString_FromString);
   M_PY_WRAPPER_DEF(PyString_FromStringAndSize);
   M_PY_WRAPPER_DEF(PyString_FromFormat);
   M_PY_WRAPPER_DEF(PyString_InternFromString);
   M_PY_WRAPPER_DEF(PyString_Size);
   M_PY_VAR_DEF(PyString_Type);

   // tuples
   M_PY_WRAPPER_DEF(PyTuple_New);
   M_PY_WRAPPER_DEF(PyTuple_GetItem);
   M_PY_WRAPPER_DEF(PyTuple_SetItem);

   // ...
   M_PY_WRAPPER_DEF(Py_VaBuildValue);
   M_PY_WRAPPER_DEF(_Py_Dealloc);
   M_PY_WRAPPER_DEF(PyArg_Parse);
   M_PY_WRAPPER_DEF(PyArg_ParseTuple);
   M_PY_WRAPPER_DEF(PyDict_SetItemString);
   M_PY_WRAPPER_DEF(PyEval_RestoreThread);
   M_PY_WRAPPER_DEF(PyEval_SaveThread);
   M_PY_WRAPPER_DEF(PyList_GetItem);
   M_PY_WRAPPER_DEF(PyList_New);
   M_PY_WRAPPER_DEF(PyList_SetItem);
   M_PY_WRAPPER_DEF(PyList_Size);
   M_PY_VAR_DEF(PyList_Type);
   M_PY_WRAPPER_DEF(PyImport_ImportModule);
   M_PY_WRAPPER_DEF(PyDict_GetItemString);
   M_PY_WRAPPER_DEF(PyModule_GetDict);
   M_PY_WRAPPER_DEF(PyModule_AddObject);
   M_PY_WRAPPER_DEF(PySys_SetObject);
   M_PY_VAR_DEF(PyType_Type);
   M_PY_WRAPPER_DEF(Py_BuildValue);
   M_PY_WRAPPER_DEF(Py_FindMethod);
   M_PY_WRAPPER_DEF(Py_InitModule4);
   M_PY_WRAPPER_DEF(PyEval_CallObjectWithKeywords);
   M_PY_WRAPPER_DEF(PyFloat_FromDouble);
   M_PY_WRAPPER_DEF(PyImport_AddModule);
   M_PY_WRAPPER_DEF(PyImport_GetModuleDict);
   M_PY_WRAPPER_DEF(PyImport_ReloadModule);
   M_PY_WRAPPER_DEF(PyRun_String);

   M_PY_WRAPPER_DEF(PyType_IsSubtype);
   M_PY_WRAPPER_DEF(_Py_NegativeRefcount);

   // other misc functions
   M_PY_WRAPPER_DEF(PyOS_snprintf);

   // variables
   M_PY_VAR_DEF(_Py_RefTotal);
   M_PY_VAR_DEF(PyModule_Type);
   M_PY_VAR_DEF(_Py_NoneStruct);
   M_PY_VAR_DEF(_Py_NotImplementedStruct);
   M_PY_VAR_DEF(PyCFunction_Type);

   // exception objects
   M_PY_VAR_DEF(PyExc_NameError);
   M_PY_VAR_DEF(PyExc_NotImplementedError);
   M_PY_VAR_DEF(PyExc_TypeError);
}

#undef M_PY_VAR_DEF
#undef M_PY_WRAPPER_DEF

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
   PYTHON_FUNC(PyErr_Format)

   // objects
   PYTHON_FUNC(PyObject_Init)
   PYTHON_FUNC(PyObject_CallFunction)
   PYTHON_FUNC(PyObject_CallObject)
   PYTHON_FUNC(PyObject_GetAttr)
   PYTHON_FUNC(PyObject_GetAttrString)
   PYTHON_FUNC(PyObject_Malloc)
   PYTHON_FUNC(PyObject_Free)
   PYTHON_FUNC(PyObject_SetAttrString)
   PYTHON_FUNC(PyObject_Size)
   PYTHON_FUNC(PyObject_Str)
   PYTHON_FUNC(PyCObject_Import)
   PYTHON_FUNC(PyCObject_FromVoidPtr)

   // ints and longs
   PYTHON_FUNC(PyInt_AsLong)
   PYTHON_FUNC(PyInt_FromLong)
   PYTHON_FUNC(PyLong_FromUnsignedLong)
   PYTHON_FUNC(PyLong_FromVoidPtr)
   PYTHON_FUNC(PyLong_AsLong)
   PYTHON_FUNC(PyLong_AsUnsignedLong)
   PYTHON_FUNC(PyInt_Type)
   PYTHON_FUNC(PyLong_Type)
   PYTHON_FUNC(_Py_TrueStruct)
   PYTHON_FUNC(_Py_ZeroStruct)

   // strings
   PYTHON_FUNC(PyString_AsString)
   PYTHON_FUNC(PyString_AsStringAndSize)
   PYTHON_FUNC(PyString_Format)
   PYTHON_FUNC(PyString_FromString)
   PYTHON_FUNC(PyString_FromStringAndSize)
   PYTHON_FUNC(PyString_FromFormat)
   PYTHON_FUNC(PyString_InternFromString)
   PYTHON_FUNC(PyString_Type)

   // tuples
   PYTHON_FUNC(PyTuple_New)
   PYTHON_FUNC(PyTuple_GetItem)
   PYTHON_FUNC(PyTuple_SetItem)

   // ...
   PYTHON_FUNC(_Py_NoneStruct)
   PYTHON_FUNC(_Py_NotImplementedStruct)
   PYTHON_FUNC(PyCFunction_Type)
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
   PYTHON_FUNC(PyModule_AddObject)
   PYTHON_FUNC(PyEval_CallObjectWithKeywords)
   PYTHON_FUNC(PyExc_NameError)
   PYTHON_FUNC(PyExc_NotImplementedError)
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

   String pathDLL = READ_APPCONFIG(MP_PYTHONDLL);
   if ( !pathDLL.empty() )
   {
#if defined(OS_WIN) && defined(DEBUG)
      // we must use debug version of Python DLL
      wxString path, name, ext;
      wxSplitPath(pathDLL, &path, &name, &ext);
      if ( name.Right(2).Lower() != _T("_d") )
         name += _T("_d");
      pathDLL = path + wxFILE_SEP_PATH + name + wxFILE_SEP_EXT + ext;
#endif // Win debug

      dllPython.Load(pathDLL);
   }
   else // try to find the DLL ourselves
   {
      // don't give errors about missing DLL here
      wxLogNull noLog;

      // try all supported versions
      for ( size_t nVer = 0; nVer < WXSIZEOF(versions); nVer++ )
      {
         String name = GetPythonDllBaseName(versions[nVer]);
         const String ext = wxDynamicLibrary::GetDllExt();

#if defined(OS_WIN) && defined(DEBUG)
         // debug version of M should only use debug Python version under
         // Windows, otherwise the CRTs they use mismatch and bad things happen
         name += _T("_d");
#endif // Win debug

         if ( dllPython.Load(name + ext) )
            break;
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

