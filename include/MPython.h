/*-*- c++ -*-********************************************************
 * MPython.h - include file for M's python code                     *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MPYTHON_H
#define MPYTHON_H

#include "Mconfig.h"    // for USE_PYTHON

#ifdef USE_PYTHON

// SWIG-generated files don't include Mpch.h themselves
#include "Mpch.h"

// before including Python.h, undef all these macros defined by our config.h
// and redefined by Python's config.h under Windows to avoid the warnings
#ifdef OS_WIN
   #undef HAVE_STRERROR
   #undef HAVE_PROTOTYPES
   #undef HAVE_STDARG_PROTOTYPES

   // use dynamic loading of Python DLL
   #ifndef USE_PYTHON_DYNAMIC
      #define USE_PYTHON_DYNAMIC
   #endif

   #ifdef USE_PYTHON_DYNAMIC
      // prevent Python.h from adding the library to our link settings (yes, it
      // really does it)
      #define USE_DL_EXPORT
   #endif // USE_PYTHON_DYNAMIC
#endif // OS_WIN

// under Linux we get annoying messages about redefinition of these symbols
// which are defined by the standard headers already included above
#ifdef __LINUX__
   #ifdef _POSIX_C_SOURCE
      #undef _POSIX_C_SOURCE
   #endif

   #ifdef _XOPEN_SOURCE
      #undef _XOPEN_SOURCE
   #endif
#endif // __LINUX__

// and under Windows for this symbol which is defined in wx/defs.h
#ifdef HAVE_SSIZE_T
   #define HAVE_SSIZE_T_DEFINED_BY_WX
   #undef HAVE_SSIZE_T
#endif

// we don't want to use Python debug DLL even if we're compiled in debug
// ourselves, so make sure _DEBUG is undefined so that Python.h doesn't
// reference any symbols only available in the debug build of Python
#ifdef _DEBUG
   #define MAHOGANY_DEBUG_WAS_DEFINED
   #undef _DEBUG
#endif

// Disable warnings about "inconsistent dll linkage" for several functions
// declared in Python.h without any declspec as they clash with the
// declarations of these functions when using CRT as a DLL.
#ifdef CC_MSC
   #if _MSC_VER >= 1800
      #pragma warning(push)
      #pragma warning(disable: 4273)
   #endif
#endif

#include <Python.h>

#ifdef CC_MSC
   #if _MSC_VER >= 1800
      #pragma warning(pop)
   #endif
#endif

#ifdef MAHOGANY_DEBUG_WAS_DEFINED
   #undef MAHOGANY_DEBUG_WAS_DEFINED
   #define _DEBUG
#endif

#if !defined(HAVE_SSIZE_T) && defined(HAVE_SSIZE_T_DEFINED_BY_WX)
   #define HAVE_SSIZE_T
#endif

#ifdef __LINUX__
   // Just include a standard header again to get the correct values back.
   #include <features.h>
#endif

#ifdef USE_PYTHON_DYNAMIC

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#endif

// this function must be called before using any Python functions, it if
// returns FALSE they can't be used
extern bool InitPythonDll();

// this one must be called if InitPythonDll() returned TRUE
extern void FreePythonDll();

// declare the wrappers
extern "C"
{
   /**
      Declare variable containing a pointer to Python function.

      This variable is later used instead of the real one with the help of
      redefinitions below. The macro also has a side effect of defining the
      type M_name_t for this function which is used elsewhere.

      This macro should be followed by a semicolon.

      @param rettype the return type of the function
      @param name name of the function
      @param args all function arguments inside parentheses
    */
   #define M_PY_WRAPPER_DECL(rettype, name, args)                             \
      typedef rettype (*M_##name##_t)args;                                    \
      extern M_##name##_t M_##name


   /**
      Same as M_PY_WRAPPER_DECL but for a variable.

      This macro should be followed by a semicolon.

      @param type type of the variable
      @param name real name of the variable
    */
   #define M_PY_VAR_DECL(type, name)                                          \
      typedef type M_##name##_t;                                              \
      extern M_##name##_t M_##name


   // startup/shutdown
   M_PY_WRAPPER_DECL(void, Py_Initialize, (void));
   M_PY_WRAPPER_DECL(void, Py_Finalize, (void));

   // errors
   //M_PY_WRAPPER_DECL(int , PyErr_BadArgument, (void));
   M_PY_WRAPPER_DECL(void , PyErr_Clear, (void));
   M_PY_WRAPPER_DECL(void , PyErr_Fetch, (PyObject **, PyObject **, PyObject **));
   //M_PY_WRAPPER_DECL(PyObject * , PyErr_NoMemory, (void));
   M_PY_WRAPPER_DECL(PyObject * , PyErr_Occurred, (void));
   M_PY_WRAPPER_DECL(void , PyErr_Restore, (PyObject *, PyObject *, PyObject *));
   //M_PY_WRAPPER_DECL(void , PyErr_SetNone, (PyObject *));
   M_PY_WRAPPER_DECL(void , PyErr_SetString, (PyObject *, const char *));
   M_PY_WRAPPER_DECL(void , PyErr_SetObject, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyErr_Format, (PyObject *, const char *, ...));

   // objects
   M_PY_WRAPPER_DECL(PyObject *, PyObject_Init, (PyObject *, PyTypeObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_Call, (PyObject *, PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_CallFunction, (PyObject *, char *format, ...));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_CallFunctionObjArgs, (PyObject *, ...));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_CallObject, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_GenericGetAttr, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_GetAttr, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_GetAttrString, (PyObject *, char *));
   M_PY_WRAPPER_DECL(int, PyObject_IsTrue, (PyObject *));
   M_PY_WRAPPER_DECL(void , PyObject_Free, (void *));
   M_PY_WRAPPER_DECL(void *, PyObject_Malloc, (size_t));
   M_PY_WRAPPER_DECL(int , PyObject_SetAttrString, (PyObject *, char *, PyObject *));
   M_PY_WRAPPER_DECL(int , PyObject_Size, (PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyObject_Str, (PyObject *));
   M_PY_WRAPPER_DECL(void *, PyCObject_Import, (char *module_name, char *cobject_name));
   M_PY_WRAPPER_DECL(PyObject *, PyCObject_FromVoidPtr, (void *cobj, void (*destruct)(void*)));
   M_PY_WRAPPER_DECL(void *, PyCObject_AsVoidPtr, (PyObject *));
   M_PY_WRAPPER_DECL(PyObject **, _PyObject_GetDictPtr, (PyObject *));

   // instances
   M_PY_WRAPPER_DECL(PyObject *, PyInstance_NewRaw, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, _PyInstance_Lookup, (PyObject *, PyObject *));
   M_PY_VAR_DECL(PyTypeObject *, PyInstance_Type);

   // integer types
   M_PY_WRAPPER_DECL(PyObject *, PyBool_FromLong, (long));
   M_PY_WRAPPER_DECL(long, PyInt_AsLong, (PyObject *));
   M_PY_WRAPPER_DECL(PyObject*, PyInt_FromLong, (long));
   M_PY_WRAPPER_DECL(PyObject *, PyLong_FromUnsignedLong, (unsigned long));
   M_PY_WRAPPER_DECL(PyObject *, PyLong_FromVoidPtr, (void *));
   M_PY_WRAPPER_DECL(long , PyLong_AsLong, (PyObject *));
   M_PY_WRAPPER_DECL(unsigned long , PyLong_AsUnsignedLong, (PyObject *));
   M_PY_WRAPPER_DECL(double, PyLong_AsDouble, (PyObject *));
   M_PY_VAR_DECL(PyTypeObject *, PyInt_Type);
   M_PY_VAR_DECL(PyTypeObject *, PyLong_Type);
   M_PY_VAR_DECL(PyIntObject *, _Py_TrueStruct);
   M_PY_VAR_DECL(PyIntObject *, _Py_ZeroStruct);

   // floats
   M_PY_VAR_DECL(PyTypeObject *, PyFloat_Type);
   M_PY_WRAPPER_DECL(PyObject *, PyFloat_FromDouble, (double));
   M_PY_WRAPPER_DECL(double, PyFloat_AsDouble, (PyObject *));

   // strings
   M_PY_WRAPPER_DECL(char *, PyString_AsString, (PyObject *));
   M_PY_WRAPPER_DECL(int , PyString_AsStringAndSize, (PyObject *, char **, Py_ssize_t *));
   M_PY_WRAPPER_DECL(void, PyString_ConcatAndDel, (PyObject **, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyString_Format, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyString_FromString, (const char *));
   M_PY_WRAPPER_DECL(PyObject *, PyString_FromStringAndSize, (const char *, int));
   M_PY_WRAPPER_DECL(PyObject *, PyString_FromFormat, (const char *, ...));
   M_PY_WRAPPER_DECL(PyObject *, PyString_InternFromString, (const char *));
   M_PY_WRAPPER_DECL(int , PyString_Size, (PyObject *));
   M_PY_VAR_DECL(PyTypeObject* , PyString_Type);

   // tuples
   M_PY_WRAPPER_DECL(PyObject *, PyTuple_New, (int size));
   M_PY_WRAPPER_DECL(PyObject *, PyTuple_GetItem, (PyObject *, int));
   M_PY_WRAPPER_DECL(int *, PyTuple_SetItem, (PyObject *, int, PyObject *));
   M_PY_VAR_DECL(PyTypeObject * , PyTuple_Type);

   // dicts
   M_PY_WRAPPER_DECL(PyObject *, PyDict_GetItem, (PyObject *mp, PyObject *key));
   M_PY_WRAPPER_DECL(PyObject*, PyDict_GetItemString, (PyObject *, const char *));
   M_PY_WRAPPER_DECL(PyObject *, PyDict_New, (void));
   M_PY_WRAPPER_DECL(int, PyDict_SetItem, (PyObject *mp, PyObject *key, PyObject *item));
   M_PY_WRAPPER_DECL(int, PyDict_SetItemString, (PyObject *dp, const char *key, PyObject *item));

   // arguments
   M_PY_WRAPPER_DECL(int, PyArg_Parse, (PyObject *, char *, ...));
   M_PY_WRAPPER_DECL(int, PyArg_ParseTuple, (PyObject *, char *, ...));
   M_PY_WRAPPER_DECL(int, PyArg_UnpackTuple, (PyObject *, char *, int, int, ...));

   // other misc functions
   M_PY_WRAPPER_DECL(PyObject* , Py_VaBuildValue, (char *, va_list));
   M_PY_WRAPPER_DECL(void , _Py_Dealloc, (PyObject *));
   M_PY_WRAPPER_DECL(void, PyEval_RestoreThread, (PyThreadState *));
   M_PY_WRAPPER_DECL(PyThreadState*, PyEval_SaveThread, (void));
   M_PY_WRAPPER_DECL(PyObject*, PyList_GetItem, (PyObject *, int));
   M_PY_WRAPPER_DECL(PyObject*, PyList_New, (int size));
   M_PY_WRAPPER_DECL(int, PyList_Append, (PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(int, PyList_SetItem, (PyObject *, int, PyObject *));
   M_PY_WRAPPER_DECL(int, PyList_Size, (PyObject *));
   M_PY_VAR_DECL(PyTypeObject *, PyList_Type);
   M_PY_WRAPPER_DECL(PyObject*, PyImport_ImportModule, (const char *));
   M_PY_WRAPPER_DECL(PyObject*, PyModule_GetDict, (PyObject *));
   M_PY_WRAPPER_DECL(int, PyModule_AddObject, (PyObject *, char *, PyObject *));
   M_PY_WRAPPER_DECL(int, PySys_SetObject, (char *, PyObject *));
   M_PY_VAR_DECL(PyTypeObject* , PyType_Type);
   M_PY_WRAPPER_DECL(PyObject*, Py_BuildValue, (char *, ...));
   M_PY_WRAPPER_DECL(PyObject*, Py_FindMethod, (PyMethodDef[], PyObject *, char *));
   M_PY_WRAPPER_DECL(PyObject*, Py_InitModule4, (char *, PyMethodDef *, char *, PyObject *, int));
   M_PY_WRAPPER_DECL(PyObject *, PyEval_CallObjectWithKeywords, (PyObject *, PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyImport_AddModule, (char *name));
   M_PY_WRAPPER_DECL(PyObject *, PyImport_GetModuleDict, (void));
   M_PY_WRAPPER_DECL(PyObject *, PyImport_ReloadModule, (PyObject *));
   M_PY_WRAPPER_DECL(PyObject *, PyRun_String, (const char *, int, PyObject *, PyObject *));
   M_PY_WRAPPER_DECL(int, PyRun_SimpleStringFlags, (const char *, PyCompilerFlags *));
   M_PY_WRAPPER_DECL(int, PyOS_snprintf, (char *str, size_t size, const char *format, ...));

   M_PY_WRAPPER_DECL(int , PyType_IsSubtype, (PyTypeObject *, PyTypeObject *));
   M_PY_WRAPPER_DECL(void , _Py_NegativeRefcount, (const char *fname, int lineno, PyObject *op));

   // types
   M_PY_VAR_DECL(PyTypeObject *, PyBaseObject_Type);
   M_PY_VAR_DECL(PyTypeObject *, PyClass_Type);
   M_PY_VAR_DECL(PyTypeObject *, PyCFunction_Type);
   M_PY_VAR_DECL(PyTypeObject *, PyModule_Type);
   M_PY_VAR_DECL(PyTypeObject *, _PyWeakref_CallableProxyType);
   M_PY_VAR_DECL(PyTypeObject *, _PyWeakref_ProxyType);

   // variables
   M_PY_VAR_DECL(long, _Py_RefTotal);
   M_PY_VAR_DECL(PyObject *, _Py_NoneStruct);
   M_PY_VAR_DECL(PyObject *, _Py_NotImplementedStruct);

   // exception objects
   M_PY_VAR_DECL(PyObject *, PyExc_AttributeError);
   M_PY_VAR_DECL(PyObject *, PyExc_IOError);
   M_PY_VAR_DECL(PyObject *, PyExc_IndexError);
   M_PY_VAR_DECL(PyObject *, PyExc_MemoryError);
   M_PY_VAR_DECL(PyObject *, PyExc_NameError);
   M_PY_VAR_DECL(PyObject *, PyExc_NotImplementedError);
   M_PY_VAR_DECL(PyObject *, PyExc_OverflowError);
   M_PY_VAR_DECL(PyObject *, PyExc_RuntimeError);
   M_PY_VAR_DECL(PyObject *, PyExc_SyntaxError);
   M_PY_VAR_DECL(PyObject *, PyExc_SystemError);
   M_PY_VAR_DECL(PyObject *, PyExc_TypeError);
   M_PY_VAR_DECL(PyObject *, PyExc_ValueError);
   M_PY_VAR_DECL(PyObject *, PyExc_ZeroDivisionError);

   #undef M_PY_VAR_DECL
   #undef M_PY_WRAPPER_DECL
}

// redefine all functions we use to our wrappers instead
// ----------------------------------------------------------------------------

// startup/shutdown
#define Py_Initialize M_Py_Initialize
#define Py_Finalize M_Py_Finalize

// errors
#define PyErr_Clear M_PyErr_Clear
#define PyErr_Fetch M_PyErr_Fetch
#define PyErr_Occurred M_PyErr_Occurred
#define PyErr_Restore M_PyErr_Restore
#define PyErr_SetString M_PyErr_SetString
#define PyErr_SetObject M_PyErr_SetObject
#define PyErr_Format M_PyErr_Format

// objects
#define PyObject_Init M_PyObject_Init
#define PyObject_Call M_PyObject_Call
#define PyObject_CallFunction M_PyObject_CallFunction
#define PyObject_CallFunctionObjArgs M_PyObject_CallFunctionObjArgs
#define PyObject_CallObject M_PyObject_CallObject
#define PyObject_GenericGetAttr M_PyObject_GenericGetAttr
#define PyObject_GetAttr M_PyObject_GetAttr
#define PyObject_GetAttrString M_PyObject_GetAttrString
#define PyObject_IsTrue M_PyObject_IsTrue
#if defined(WITH_PYMALLOC) && defined(PYMALLOC_DEBUG)
   #define _PyObject_DebugMalloc M_PyObject_Malloc
   #define _PyObject_DebugFree M_PyObject_Free
#else
   #define PyObject_Malloc M_PyObject_Malloc
   #define PyObject_Free M_PyObject_Free
#endif
#define PyObject_SetAttrString M_PyObject_SetAttrString
#define PyObject_Size M_PyObject_Size
#define PyObject_Str M_PyObject_Str
#define PyCObject_Import M_PyCObject_Import
#define PyCObject_FromVoidPtr M_PyCObject_FromVoidPtr
#define PyCObject_AsVoidPtr M_PyCObject_AsVoidPtr
#define _PyObject_GetDictPtr M__PyObject_GetDictPtr

// instances
#define PyInstance_NewRaw M_PyInstance_NewRaw
#define _PyInstance_Lookup M__PyInstance_Lookup
#define PyInstance_Type (*M_PyInstance_Type)

// integer types
#define PyBool_FromLong M_PyBool_FromLong
#define PyInt_AsLong M_PyInt_AsLong
#define PyInt_FromLong M_PyInt_FromLong
#define PyLong_FromUnsignedLong M_PyLong_FromUnsignedLong
#define PyLong_FromVoidPtr M_PyLong_FromVoidPtr
#define PyLong_AsLong M_PyLong_AsLong
#define PyLong_AsUnsignedLong M_PyLong_AsUnsignedLong
#define PyLong_AsDouble (*M_PyLong_AsDouble)
#define PyInt_Type (*M_PyInt_Type)
#define PyLong_Type (*M_PyLong_Type)
#define _Py_TrueStruct (*M__Py_TrueStruct)
#define _Py_ZeroStruct (*M__Py_ZeroStruct)

// floats
#define PyFloat_Type (*M_PyFloat_Type)
#define PyFloat_FromDouble M_PyFloat_FromDouble
#define PyFloat_AsDouble (*M_PyFloat_AsDouble)

// strings
#define PyString_AsString M_PyString_AsString
#define PyString_AsStringAndSize M_PyString_AsStringAndSize
#define PyString_ConcatAndDel M_PyString_ConcatAndDel
#define PyString_Format M_PyString_Format
#define PyString_FromString M_PyString_FromString
#define PyString_FromStringAndSize M_PyString_FromStringAndSize
#define PyString_FromFormat M_PyString_FromFormat
#define PyString_InternFromString M_PyString_InternFromString
#define PyString_Type (*M_PyString_Type)

// tuples
#define PyTuple_New M_PyTuple_New
#define PyTuple_GetItem M_PyTuple_GetItem
#define PyTuple_SetItem M_PyTuple_SetItem
#define PyTuple_Type (*M_PyTuple_Type)

// lists
#define PyList_Type (*M_PyList_Type)
#define PyList_New M_PyList_New
#define PyList_SetItem M_PyList_SetItem
#define PyList_Append M_PyList_Append

// dicts
#define PyDict_GetItem M_PyDict_GetItem
#define PyDict_GetItemString M_PyDict_GetItemString
#define PyDict_New M_PyDict_New
#define PyDict_SetItem M_PyDict_SetItem
#define PyDict_SetItemString M_PyDict_SetItemString

// arguments
#define PyArg_Parse M_PyArg_Parse
#define PyArg_ParseTuple M_PyArg_ParseTuple
#define PyArg_UnpackTuple M_PyArg_UnpackTuple

// types: these are objects and not function pointers
#define PyBaseObject_Type (*M_PyBaseObject_Type)
#define PyClass_Type (*M_PyClass_Type)
#define PyCFunction_Type (*M_PyCFunction_Type)
#define PyModule_Type (*M_PyModule_Type)
#define _PyWeakref_CallableProxyType (*M__PyWeakref_CallableProxyType)
#define _PyWeakref_ProxyType (*M__PyWeakref_ProxyType)

// ...
#define _Py_NoneStruct (*M__Py_NoneStruct)
#define _Py_NotImplementedStruct (*M__Py_NotImplementedStruct)
#define Py_BuildValue M_Py_BuildValue
#define Py_VaBuildValue M_Py_VaBuildValue
#define _Py_RefTotal M__Py_RefTotal
#define PyImport_ImportModule M_PyImport_ImportModule
#define PyModule_GetDict M_PyModule_GetDict
#define PyModule_AddObject M_PyModule_AddObject
#define PyType_Type (*M_PyType_Type)
#define PyEval_CallObjectWithKeywords M_PyEval_CallObjectWithKeywords
#define PyImport_AddModule M_PyImport_AddModule
#define PyImport_GetModuleDict M_PyImport_GetModuleDict
#define PyImport_ReloadModule M_PyImport_ReloadModule
// PyRun_String is already a macro in 2.6 but, surprisingly, the DLL still
// exports it as a function too and using it seems to work fine...
#undef PyRun_String
#define PyRun_String M_PyRun_String
#define PyRun_SimpleStringFlags M_PyRun_SimpleStringFlags
#define PyOS_snprintf M_PyOS_snprintf

// exception objects
#define PyExc_AttributeError M_PyExc_AttributeError
#define PyExc_IOError M_PyExc_IOError
#define PyExc_IndexError M_PyExc_IndexError
#define PyExc_MemoryError M_PyExc_MemoryError
#define PyExc_NameError M_PyExc_NameError
#define PyExc_NotImplementedError M_PyExc_NotImplementedError
#define PyExc_OverflowError M_PyExc_OverflowError
#define PyExc_RuntimeError M_PyExc_RuntimeError
#define PyExc_SyntaxError M_PyExc_SyntaxError
#define PyExc_SystemError M_PyExc_SystemError
#define PyExc_TypeError M_PyExc_TypeError
#define PyExc_ValueError M_PyExc_ValueError
#define PyExc_ZeroDivisionError M_PyExc_ZeroDivisionError

// Python 2.3+
#define PyType_IsSubtype M_PyType_IsSubtype
#define _Py_NegativeRefcount M__Py_NegativeRefcount

// _Py_Dealloc  may be defined as a macro or not (in debug builds)
#ifndef _Py_Dealloc
#define _Py_Dealloc M__Py_Dealloc
#endif

// Py_InitModule4 may be defined as Py_InitModule4TraceRefs (in debug builds)
#ifdef Py_TRACE_REFS
#define Py_InitModule4TraceRefs M_Py_InitModule4
#define M_Py_InitModule4TraceRefs M_Py_InitModule4
#else
// Another complication: under 64 bit systems Python uses a different name to
// avoid loading 32 bit modules into 64 bit interpreter
#if SIZEOF_SIZE_T != SIZEOF_INT
#define Py_InitModule4_64 M_Py_InitModule4
#else
#define Py_InitModule4 M_Py_InitModule4
#endif
#endif

#endif // USE_PYTHON_DYNAMIC

#include "PythonHelp.h"

#define  M_PYTHON_MODULE "Minit"

#endif // USE_PYTHON

#endif // MPYTHON_H
