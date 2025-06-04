///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   PythonHelp.h - functions for working with Python
// Purpose:     some simple high-level functions for calling Python code
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2004 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _PYTHONHELP_H_
#define _PYTHONHELP_H_

#include "wx/arrstr.h"

class Profile;

/**
   Call a callback function written in Python.

   The callback name is read from the given profile using the given callback
   name as argument. If it is not empty, the callback will be called with its
   name and the object as first arguments. All callbacks must return an integer
   which is interpreted differently depending on the callback.

   @param name name of the callback name in the profiles
   @param def default value to return
   @param obj pointer to the object calling it
   @param classname name of the object class
   @param profile profile to look for the callback name in
   @return integer value returned by callback or def if calling it failed
*/
int
PythonCallback(const char *name,
               int def,
               void *obj,
               const char *classname,
               Profile *profile = NULL);


/**
  Call a Python function.

  So far we support only functions taking a single argument and returning a
  single value. The argument is an arbitrary C++ object among those exported to
  Python via SWIG wrappers and the result is either a simple type or an object
  of such type again.

  @param func Name of the python function, possibly Module.name
  @param obj The C++ object being passed as a parameter
  @param classname  Name of the object's class
  @param resultfmt Format string for the result pointer
  @param result Pointer where to store the result
  @return true on success
*/
bool PythonFunction(const char *func,
                    void *obj,
                    const char *classname,
                    const char *resultfmt,
                    void *result);

/**
   Call a Python function taking string parameters and returning string value.

   @param func the function name, possible module.name
   @param arguments the string arguments, array may be empty
   @param value the string receiving output, none if NULL
   @return true if function could be called, false on error
 */
bool PythonStringFunction(const String& func,
                          const wxArrayString& arguments,
                          String *value);

/**
   Function to run a simple python script in the global namespace.

   If the file name is relative, the script is looked in all directories in
   PYTHONPATH. The code in InitPython.cpp adds our directories to it so any
   script in them can be invoked without specifying its full path.

   @param filename to load the script from
   @return true if script was executed successfully, false otherwise
*/
bool
PythonRunScript(const char *filename);

/**
  Gets the last error message/traceback from Python and stores it in a String.

  @return the error message
*/
String PythonGetErrorMessage();

#endif // _PYTHONHELP_H_

