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

#ifdef   USE_PYTHON

// before including Python.h, undef all these macros defined by our config.h
// and redefined by Python's config.h under Windows to avoid the warnings
#ifdef OS_WIN
   #undef HAVE_STRERROR
   #undef HAVE_PROTOTYPES
   #undef HAVE_STDARG_PROTOTYPES
#endif // OS_WIN

#  include <Python.h>

#  include "PythonHelp.h"

#  define  M_PYTHON_MODULE   "Minit"
#endif

#endif // MPYTHON_H
