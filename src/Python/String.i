/*-*- c++ -*-********************************************************
 * String class interface for python                                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/08/22 23:19:29  VZ
 * many, many changes (I finally had to correct all the small things I was delaying
 * before). Among more or less major changes:
 *  1) as splash screen now looks more or less decently, show it on startup
 *  2) we have an options dialog (not functional yet, though)
 *  3) MApplication has OnShutDown() function which is the same to OnStartup() as
 *     OnExit() to OnInit()
 *  4) confirmation before closing compose window
 * &c...
 *
 * Revision 1.2  1998/05/30 17:55:56  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.1  1998/05/24 14:51:50  KB
 * lots of progress on Python, but cannot call functions yet
 *
 *
 *******************************************************************/

// naming this module String is a very bad idea for systems with
// case-insensitive file system (Windows...) because of standard Python
// module string.
%module 	MString
%{
#include  "Mcommon.h"

// we don't want to export our functions as we don't build a shared library
#undef SWIGEXPORT
#define SWIGEXPORT(a,b) a b
%}

class String 
{
public:
   String(const char *);
   const char *c_str(void);
};

