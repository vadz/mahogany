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

// Python.h detects _DEBUG setting and changes its behaviour depending on this,
// we don't want this to happen as we don't need to debug Python code
#ifdef _DEBUG
   #undef _DEBUG
   #define _DEBUG_WAS_DEFINED
#endif

#include <Python.h>

#ifdef _DEBUG_WAS_DEFINED
   #undef _DEBUG_WAS_DEFINED
   #define _DEBUG
#endif

#ifdef USE_PYTHON_DYNAMIC

// this function must be called before using any Python functions, it if
// returns FALSE they can't be used
extern bool InitPythonDll();

// this one must be called if InitPythonDll() returned TRUE
extern void FreePythonDll();

// declare the wrappers
extern "C"
{
   // functions
   extern PyObject* (*M_Py_VaBuildValue)(char *, va_list);
   extern void (*M__Py_Dealloc)(PyObject *);
   extern void (*M_PyErr_Fetch)(PyObject **, PyObject **, PyObject **);
   extern void (*M_PyErr_Restore)(PyObject *, PyObject *, PyObject *);
   extern int(*M_PyArg_Parse)(PyObject *, char *, ...);
   extern int(*M_PyArg_ParseTuple)(PyObject *, char *, ...);
   extern int(*M_PyDict_SetItemString)(PyObject *dp, char *key, PyObject *item);
   extern int(*M_PyErr_BadArgument)(void);
   extern PyObject*(*M_PyErr_NoMemory)(void);
   extern PyObject*(*M_PyErr_Occurred)(void);
   extern void(*M_PyErr_SetNone)(PyObject *);
   extern void(*M_PyErr_SetString)(PyObject *, const char *);
   extern void(*M_PyEval_RestoreThread)(PyThreadState *);
   extern PyThreadState*(*M_PyEval_SaveThread)(void);
   extern long(*M_PyInt_AsLong)(PyObject *);
   extern PyObject*(*M_PyInt_FromLong)(long);
   extern PyTypeObject* M_PyInt_Type;
   extern PyObject*(*M_PyList_GetItem)(PyObject *, int);
   extern PyObject*(*M_PyList_New)(int size);
   extern int(*M_PyList_SetItem)(PyObject *, int, PyObject *);
   extern int(*M_PyList_Size)(PyObject *);
   extern PyTypeObject* M_PyList_Type;
   extern PyObject*(*M_PyImport_ImportModule)(const char *);
   extern PyObject*(*M_PyDict_GetItemString)(PyObject *, const char *);
   extern PyObject*(*M_PyModule_GetDict)(PyObject *);
   extern char*(*M_PyString_AsString)(PyObject *);
   extern PyObject*(*M_PyString_FromString)(const char *);
   extern PyObject*(*M_PyString_FromStringAndSize)(const char *, int);
   extern int(*M_PyString_Size)(PyObject *);
   extern PyTypeObject* M_PyString_Type;
   extern int(*M_PySys_SetObject)(char *, PyObject *);
   extern PyTypeObject* M_PyType_Type;
   extern PyObject*(*M_Py_BuildValue)(char *, ...);
   extern PyObject*(*M_Py_FindMethod)(PyMethodDef[], PyObject *, char *);
   extern PyObject*(*M_Py_InitModule4)(char *, PyMethodDef *, char *, PyObject *, int);
   extern void(*M_Py_Initialize)(void);
   extern PyObject*(*M__PyObject_New)(PyTypeObject *, PyObject *);
   extern PyObject*(*M__PyObject_Init)(PyObject *, PyTypeObject *);
   extern PyObject *(*M_PyEval_CallObjectWithKeywords)(PyObject *, PyObject *, PyObject *);
   extern PyObject *(*M_PyFloat_FromDouble)(double);
   extern PyObject *(*M_PyImport_AddModule)(char *name);
   extern PyObject *(*M_PyImport_GetModuleDict)(void);
   extern PyObject *(*M_PyImport_ReloadModule)(PyObject *);
   extern PyObject *(*M_PyObject_CallFunction)(PyObject *, char *format, ...);
   extern PyObject *(*M_PyObject_CallObject)(PyObject *, PyObject *);
   extern PyObject *(*M_PyObject_GetAttr)(PyObject *, PyObject *);
   extern PyObject *(*M_PyObject_GetAttrString)(PyObject *, char *);
   extern PyObject *(*M_PyRun_String)(const char *, int, PyObject *, PyObject *);
   extern PyObject *(*M_PyString_InternFromString)(const char *);

   extern int (*M_PyType_IsSubtype)(PyTypeObject *, PyTypeObject *);
   extern int (*M_PyObject_SetAttrString)(PyObject *, char *, PyObject *);
   extern void *(*M_PyObject_Malloc)(size_t);
   extern void (*M__Py_NegativeRefcount)(const char *fname, int lineno, PyObject *op);

   // variables
   extern long M__Py_RefTotal;
   extern PyTypeObject *M_PyModule_Type;
   extern PyObject* M__Py_NoneStruct;

   // exception objects
   extern PyObject *M_PyExc_NameError;
   extern PyObject *M_PyExc_TypeError;
}

// redefine all functions we use to our wrappers instead
#define Py_Initialize M_Py_Initialize
#define _Py_NoneStruct (*M__Py_NoneStruct)
#define Py_BuildValue M_Py_BuildValue
#define Py_VaBuildValue M_Py_VaBuildValue
#define _Py_RefTotal M__Py_RefTotal
#define PyModule_Type (*M_PyModule_Type)
#define PyErr_Fetch M_PyErr_Fetch
#define PyErr_Restore M_PyErr_Restore
#define PyArg_Parse M_PyArg_Parse
#define PyArg_ParseTuple M_PyArg_ParseTuple
#define PyDict_GetItemString M_PyDict_GetItemString
#define PyDict_SetItemString M_PyDict_SetItemString
#define PyErr_Occurred M_PyErr_Occurred
#define PyErr_SetString M_PyErr_SetString
#define PyInt_FromLong M_PyInt_FromLong
#define PyImport_ImportModule M_PyImport_ImportModule
#define PyModule_GetDict M_PyModule_GetDict
#define PyString_AsString M_PyString_AsString
#define PyString_FromString M_PyString_FromString
#define PyString_Type (*M_PyString_Type)
#define PyType_Type (*M_PyType_Type)
#define PyEval_CallObjectWithKeywords M_PyEval_CallObjectWithKeywords
#define PyExc_NameError M_PyExc_NameError
#define PyExc_TypeError M_PyExc_TypeError
#define PyFloat_FromDouble M_PyFloat_FromDouble
#define PyImport_AddModule M_PyImport_AddModule
#define PyImport_GetModuleDict M_PyImport_GetModuleDict
#define PyImport_ReloadModule M_PyImport_ReloadModule
#define PyObject_CallFunction M_PyObject_CallFunction
#define PyObject_CallObject M_PyObject_CallObject
#define PyObject_GetAttr M_PyObject_GetAttr
#define PyObject_GetAttrString M_PyObject_GetAttrString
#define PyRun_String M_PyRun_String
#define PyString_InternFromString M_PyString_InternFromString

// Python 2.3+
#define PyType_IsSubtype M_PyType_IsSubtype
#define PyObject_SetAttrString M_PyObject_SetAttrString
#define PyObject_Malloc M_PyObject_Malloc
#define PyInt_AsLong M_PyInt_AsLong
#define _Py_NegativeRefcount M__Py_NegativeRefcount

// special cases

// _Py_Dealloc  may be defined as a macro or not (in debug builds)
#ifndef _Py_Dealloc
#define _Py_Dealloc M__Py_Dealloc
#endif

// Py_InitModule4 may be defined as Py_InitModule4TraceRefs (in debug builds)
#ifdef Py_TRACE_REFS
#define Py_InitModule4TraceRefs M_Py_InitModule4
#define M_Py_InitModule4TraceRefs M_Py_InitModule4
#else
#define Py_InitModule4 M_Py_InitModule4
#endif

#endif // USE_PYTHON_DYNAMIC

#include "PythonHelp.h"

#define  M_PYTHON_MODULE "Minit"

#endif // USE_PYTHON

#endif // MPYTHON_H
