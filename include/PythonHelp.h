/*-*- c++ -*-********************************************************
 * PythonHelp.h - Helper functions for the python interface         *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 * $Log$
 * Revision 1.1  1998/05/24 14:47:16  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 *
 *******************************************************************/

#ifndef   PYTHONHELP_H
#   define PYTHONHELP_H

/// the two possible modes for PyH_Run_Codestr():
enum PyH_RunModes { PY_STATEMENT, PY_EXPRESSION };

/** Function to run statements or expressions in python.
    @param mode either PY_STATMENT or PY_EXPRESSION
    @param code the python code to run
    @param modname the module to load (if needed, otherwise __main__)
    @param resfmt format string for the result
    @param cresult pointer where to store the result
    @return 0 on success
*/
int
PyH_RunCodestr(enum PyH_RunModes mode, const char *code,      /* expr or stmt string */
                const char *modname,                     /* loads module if needed */
                const char *resfmt, void *cresult);       /* converts expr
                                                    /* result to C */

/** Function to call a python function
    @param funcname the function to call
    @param modname the module to load (if needed, otherwise __main__)
    @param resfmt format string for the result
    @param cresult pointer where to store the result
    @param argfmt argument format string
    @param ... the arguments
    @return 0 on error
*/
int
PyH_RunFunction(const char *funcname, const char *modname,          /* load from module */
                const char *resfmt,  void *cresult,           /* convert to c/c++ */
                const char *argfmt,  ... /* arg, arg... */ );  /* convert to python */

/// macro to run an expression
#define   PyH_Expression(exp,module,resfmt,cresult) \
   PyH_RunCodestr(PY_EXPRESSION,exp,module, resfmt, cresult)

/// macro to run a statement      
#define   PyH_Statement(stat,module,resfmt,cresult) \
   PyH_RunCodestr(PY_STATEMENT,stat,module, resfmt, cresult)

/** Converts a PyObject to a C++ variable in a safe way.
    @param presult the object to convert
    @param resFormat the format string
    @param resTarget where to store it
    @return 0 on error
*/
int PyH_ConvertResult(PyObject *presult, const char *resFormat, void *resTarget);

#endif
