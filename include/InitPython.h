///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
// File name:   PythonInit.h: functions to initialize and shutdown Python
// Purpose:     initialize/shutdown embedded Python interpreter
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.01.04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _PYTHONINIT_H_
#define _PYTHONINIT_H_

#ifdef  USE_PYTHON

/**
   Initialize Python interpreter.

   If the function fails, it logs the error message itself, caller doesn't have
   to do it.

   @return true if ok (you must call FreePython() then), false if failed.
 */
extern bool InitPython();

/**
   Check if Python interpreter has been initialized.

   @return true if Python can be used
 */
extern bool IsPythonInitialized();

/**
   Shutdown Python interpreter and free memory

   Python can't be used after calling this function without successfully
   calling InitPython() again.
 */
extern void FreePython();

#endif // USE_PYTHON

#endif // _PYTHONINIT_H_
