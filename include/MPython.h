/*-*- c++ -*-********************************************************
 * MPython.h - include file for M's python code                     *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/05/24 14:47:10  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 *
 *******************************************************************/

#ifndef MPYTHON_H
#define MPYTHON_H

#ifdef   USE_PYTHON
#   include   "Python.h"
#   include   "PythonHelp.h"
#   define   M_PYTHON_MODULE   "Minit"
#endif

#endif
