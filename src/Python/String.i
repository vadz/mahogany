/*-*- c++ -*-********************************************************
 * String class interface for python                                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/05/30 17:55:56  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.1  1998/05/24 14:51:50  KB
 * lots of progress on Python, but cannot call functions yet
 *
 *
 *******************************************************************/

%module 	String
%{
#include	"Mpch.h"
%}

class String 
{
public:
   String(const char *);
   const char *c_str(void);
};

