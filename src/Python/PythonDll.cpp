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
   M_PY_WRAPPER_DEF(PyErr_SetObject);
   M_PY_WRAPPER_DEF(PyErr_Format);

   // objects
   M_PY_WRAPPER_DEF(PyObject_Init);
   M_PY_WRAPPER_DEF(PyObject_Call);
   M_PY_WRAPPER_DEF(PyObject_CallFunction);
   M_PY_WRAPPER_DEF(PyObject_CallFunctionObjArgs);
   M_PY_WRAPPER_DEF(PyObject_CallObject);
   M_PY_WRAPPER_DEF(PyObject_Free);
   M_PY_WRAPPER_DEF(PyObject_GenericGetAttr);
   M_PY_WRAPPER_DEF(PyObject_GetAttr);
   M_PY_WRAPPER_DEF(PyObject_GetAttrString);
   M_PY_WRAPPER_DEF(PyObject_IsTrue);
   M_PY_WRAPPER_DEF(PyObject_Malloc);
   M_PY_WRAPPER_DEF(PyObject_SetAttrString);
   M_PY_WRAPPER_DEF(PyObject_Size);
   M_PY_WRAPPER_DEF(PyObject_Str);
   M_PY_WRAPPER_DEF(PyCObject_Import);
   M_PY_WRAPPER_DEF(PyCObject_FromVoidPtr);
   M_PY_WRAPPER_DEF(PyCObject_AsVoidPtr);
   M_PY_WRAPPER_DEF(_PyObject_GetDictPtr);

   // instances
   M_PY_WRAPPER_DEF(PyInstance_NewRaw);
   M_PY_WRAPPER_DEF(_PyInstance_Lookup);
   M_PY_VAR_DEF(PyInstance_Type);

   // integer types
   M_PY_WRAPPER_DEF(PyBool_FromLong);
   M_PY_WRAPPER_DEF(PyInt_AsLong);
   M_PY_WRAPPER_DEF(PyInt_FromLong);
   M_PY_WRAPPER_DEF(PyLong_FromUnsignedLong);
   M_PY_WRAPPER_DEF(PyLong_FromVoidPtr);
   M_PY_WRAPPER_DEF(PyLong_AsLong);
   M_PY_WRAPPER_DEF(PyLong_AsUnsignedLong);
   M_PY_WRAPPER_DEF(PyLong_AsDouble);
   M_PY_VAR_DEF(PyInt_Type);
   M_PY_VAR_DEF(PyLong_Type);
   M_PY_VAR_DEF(_Py_TrueStruct);
   M_PY_VAR_DEF(_Py_ZeroStruct);

   // floats
   M_PY_VAR_DEF(PyFloat_Type);
   M_PY_WRAPPER_DEF(PyFloat_FromDouble);
   M_PY_WRAPPER_DEF(PyFloat_AsDouble);

   // strings
   M_PY_WRAPPER_DEF(PyString_AsString);
   M_PY_WRAPPER_DEF(PyString_AsStringAndSize);
   M_PY_WRAPPER_DEF(PyString_ConcatAndDel);
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
   M_PY_VAR_DEF(PyTuple_Type);

   // dicts
   M_PY_WRAPPER_DEF(PyDict_GetItem);
   M_PY_WRAPPER_DEF(PyDict_GetItemString);
   M_PY_WRAPPER_DEF(PyDict_New);
   M_PY_WRAPPER_DEF(PyDict_SetItem);
   M_PY_WRAPPER_DEF(PyDict_SetItemString);

   // args
   M_PY_WRAPPER_DEF(PyArg_Parse);
   M_PY_WRAPPER_DEF(PyArg_ParseTuple);
   M_PY_WRAPPER_DEF(PyArg_UnpackTuple);

   // ...
   M_PY_WRAPPER_DEF(Py_VaBuildValue);
   M_PY_WRAPPER_DEF(_Py_Dealloc);
   M_PY_WRAPPER_DEF(PyEval_RestoreThread);
   M_PY_WRAPPER_DEF(PyEval_SaveThread);
   M_PY_WRAPPER_DEF(PyList_GetItem);
   M_PY_WRAPPER_DEF(PyList_New);
   M_PY_WRAPPER_DEF(PyList_Append);
   M_PY_WRAPPER_DEF(PyList_SetItem);
   M_PY_WRAPPER_DEF(PyList_Size);
   M_PY_VAR_DEF(PyList_Type);
   M_PY_WRAPPER_DEF(PyImport_ImportModule);
   M_PY_WRAPPER_DEF(PyModule_GetDict);
   M_PY_WRAPPER_DEF(PyModule_AddObject);
   M_PY_WRAPPER_DEF(PySys_SetObject);
   M_PY_VAR_DEF(PyType_Type);
   M_PY_WRAPPER_DEF(Py_BuildValue);
   M_PY_WRAPPER_DEF(Py_FindMethod);
   M_PY_WRAPPER_DEF(Py_InitModule4);
   M_PY_WRAPPER_DEF(PyEval_CallObjectWithKeywords);
   M_PY_WRAPPER_DEF(PyImport_AddModule);
   M_PY_WRAPPER_DEF(PyImport_GetModuleDict);
   M_PY_WRAPPER_DEF(PyImport_ReloadModule);
   M_PY_WRAPPER_DEF(PyRun_String);

   M_PY_WRAPPER_DEF(PyType_IsSubtype);
   M_PY_WRAPPER_DEF(_Py_NegativeRefcount);

   // other misc functions
   M_PY_WRAPPER_DEF(PyOS_snprintf);

   // misc types
   M_PY_VAR_DEF(PyBaseObject_Type);
   M_PY_VAR_DEF(PyClass_Type);
   M_PY_VAR_DEF(PyCFunction_Type);
   M_PY_VAR_DEF(PyModule_Type);
   M_PY_VAR_DEF(_PyWeakref_CallableProxyType);
   M_PY_VAR_DEF(_PyWeakref_ProxyType);

   // variables
   M_PY_VAR_DEF(_Py_RefTotal);
   M_PY_VAR_DEF(_Py_NoneStruct);
   M_PY_VAR_DEF(_Py_NotImplementedStruct);

   // exception objects
   M_PY_VAR_DEF(PyExc_AttributeError);
   M_PY_VAR_DEF(PyExc_IOError);
   M_PY_VAR_DEF(PyExc_IndexError);
   M_PY_VAR_DEF(PyExc_MemoryError);
   M_PY_VAR_DEF(PyExc_NameError);
   M_PY_VAR_DEF(PyExc_NotImplementedError);
   M_PY_VAR_DEF(PyExc_OverflowError);
   M_PY_VAR_DEF(PyExc_RuntimeError);
   M_PY_VAR_DEF(PyExc_SyntaxError);
   M_PY_VAR_DEF(PyExc_SystemError);
   M_PY_VAR_DEF(PyExc_TypeError);
   M_PY_VAR_DEF(PyExc_ValueError);
   M_PY_VAR_DEF(PyExc_ZeroDivisionError);
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

#define PYTHON_SYMBOL(func) { #func, NULL, (PYTHON_PROC *)&M_ ## func },
#define PYTHON_FUNC_ALT(func, alt) { #func, #alt, (PYTHON_PROC *)&M_ ## func },

static struct PythonFunc
{
    const char *name;       // normal function name
    const char *nameAlt;    // alternative name for Debug build
    PYTHON_PROC *ptr;       // function pointer
} pythonFuncs[] =
{
   // startup/shutdown
   PYTHON_SYMBOL(Py_Initialize)
   PYTHON_SYMBOL(Py_Finalize)

   // errors
   PYTHON_SYMBOL(PyErr_Clear)
   PYTHON_SYMBOL(PyErr_Fetch)
   PYTHON_SYMBOL(PyErr_Occurred)
   PYTHON_SYMBOL(PyErr_Restore)
   PYTHON_SYMBOL(PyErr_SetString)
   PYTHON_SYMBOL(PyErr_SetObject)
   PYTHON_SYMBOL(PyErr_Format)

   // objects
   PYTHON_SYMBOL(PyObject_Init)
   PYTHON_SYMBOL(PyObject_Call)
   PYTHON_SYMBOL(PyObject_CallFunction)
   PYTHON_SYMBOL(PyObject_CallFunctionObjArgs)
   PYTHON_SYMBOL(PyObject_CallObject)
   PYTHON_SYMBOL(PyObject_Free)
   PYTHON_SYMBOL(PyObject_GenericGetAttr)
   PYTHON_SYMBOL(PyObject_GetAttr)
   PYTHON_SYMBOL(PyObject_GetAttrString)
   PYTHON_SYMBOL(PyObject_IsTrue)
   PYTHON_SYMBOL(PyObject_Malloc)
   PYTHON_SYMBOL(PyObject_SetAttrString)
   PYTHON_SYMBOL(PyObject_Size)
   PYTHON_SYMBOL(PyObject_Str)
   PYTHON_SYMBOL(PyCObject_Import)
   PYTHON_SYMBOL(PyCObject_FromVoidPtr)
   PYTHON_SYMBOL(PyCObject_AsVoidPtr)
   PYTHON_SYMBOL(_PyObject_GetDictPtr)

   // instances
   PYTHON_SYMBOL(PyInstance_NewRaw)
   PYTHON_SYMBOL(PyInstance_Type)
   PYTHON_SYMBOL(_PyInstance_Lookup)

   // integer types
   PYTHON_SYMBOL(PyBool_FromLong)
   PYTHON_SYMBOL(PyInt_AsLong)
   PYTHON_SYMBOL(PyInt_FromLong)
   PYTHON_SYMBOL(PyLong_FromUnsignedLong)
   PYTHON_SYMBOL(PyLong_FromVoidPtr)
   PYTHON_SYMBOL(PyLong_AsLong)
   PYTHON_SYMBOL(PyLong_AsUnsignedLong)
   PYTHON_SYMBOL(PyLong_AsDouble)
   PYTHON_SYMBOL(PyInt_Type)
   PYTHON_SYMBOL(PyLong_Type)
   PYTHON_SYMBOL(_Py_TrueStruct)
   PYTHON_SYMBOL(_Py_ZeroStruct)

   // floats
   PYTHON_SYMBOL(PyFloat_Type)
   PYTHON_SYMBOL(PyFloat_FromDouble)
   PYTHON_SYMBOL(PyFloat_AsDouble)

   // strings
   PYTHON_SYMBOL(PyString_AsString)
   PYTHON_SYMBOL(PyString_AsStringAndSize)
   PYTHON_SYMBOL(PyString_ConcatAndDel)
   PYTHON_SYMBOL(PyString_Format)
   PYTHON_SYMBOL(PyString_FromString)
   PYTHON_SYMBOL(PyString_FromStringAndSize)
   PYTHON_SYMBOL(PyString_FromFormat)
   PYTHON_SYMBOL(PyString_InternFromString)
   PYTHON_SYMBOL(PyString_Type)

   // tuples
   PYTHON_SYMBOL(PyTuple_New)
   PYTHON_SYMBOL(PyTuple_GetItem)
   PYTHON_SYMBOL(PyTuple_SetItem)
   PYTHON_SYMBOL(PyTuple_Type)

   // dicts
   PYTHON_SYMBOL(PyDict_GetItem)
   PYTHON_SYMBOL(PyDict_GetItemString)
   PYTHON_SYMBOL(PyDict_New)
   PYTHON_SYMBOL(PyDict_SetItem)
   PYTHON_SYMBOL(PyDict_SetItemString)

   // args
   PYTHON_SYMBOL(PyArg_Parse)
   PYTHON_SYMBOL(PyArg_ParseTuple)
   PYTHON_SYMBOL(PyArg_UnpackTuple)

   // ...
   PYTHON_SYMBOL(_Py_NoneStruct)
   PYTHON_SYMBOL(_Py_NotImplementedStruct)
   PYTHON_SYMBOL(PyCFunction_Type)
   PYTHON_FUNC_ALT(Py_InitModule4, Py_InitModule4TraceRefs)
   PYTHON_SYMBOL(Py_BuildValue)
   PYTHON_SYMBOL(Py_VaBuildValue)
#ifdef Py_REF_DEBUG
   PYTHON_SYMBOL(_Py_RefTotal)
#endif // Py_REF_DEBUG
#ifdef Py_TRACE_REFS
   PYTHON_SYMBOL(_Py_Dealloc)
#endif // Py_TRACE_REFS

   // misc types
   PYTHON_SYMBOL(PyBaseObject_Type)
   PYTHON_SYMBOL(PyClass_Type)
   PYTHON_SYMBOL(PyModule_Type)
   PYTHON_SYMBOL(_PyWeakref_CallableProxyType)
   PYTHON_SYMBOL(_PyWeakref_ProxyType)

   PYTHON_SYMBOL(PyImport_ImportModule)
   PYTHON_SYMBOL(PyModule_GetDict)
   PYTHON_SYMBOL(PyModule_AddObject)
   PYTHON_SYMBOL(PyEval_CallObjectWithKeywords)

   PYTHON_SYMBOL(PyExc_AttributeError)
   PYTHON_SYMBOL(PyExc_IOError)
   PYTHON_SYMBOL(PyExc_IndexError)
   PYTHON_SYMBOL(PyExc_MemoryError)
   PYTHON_SYMBOL(PyExc_NameError)
   PYTHON_SYMBOL(PyExc_NotImplementedError)
   PYTHON_SYMBOL(PyExc_OverflowError)
   PYTHON_SYMBOL(PyExc_RuntimeError)
   PYTHON_SYMBOL(PyExc_SyntaxError)
   PYTHON_SYMBOL(PyExc_SystemError)
   PYTHON_SYMBOL(PyExc_TypeError)
   PYTHON_SYMBOL(PyExc_ValueError)
   PYTHON_SYMBOL(PyExc_ZeroDivisionError)

   PYTHON_SYMBOL(PyImport_AddModule)
   PYTHON_SYMBOL(PyImport_GetModuleDict)
   PYTHON_SYMBOL(PyImport_ReloadModule)
   PYTHON_SYMBOL(PyRun_String)
   PYTHON_SYMBOL(PyType_IsSubtype)
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
      _T("24"), _T("23"), _T("22"), _T("20"), _T("15")
#elif defined(OS_UNIX)
      _T("2.4"), _T("2.3"), _T("2.2"), _T("2.0"), _T("1.5")
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

