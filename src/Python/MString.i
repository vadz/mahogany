/*-*- c++ -*-********************************************************
 * String class interface for python                                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$      *
 *******************************************************************/

// naming this module String is a very bad idea for systems with
// case-insensitive file system (Windows...) because of standard Python
// module string.

%module	MString

%{
//#include  "Mswig.h"
#include "Mcommon.h"
%}

//    Original KB Code

class String
{
public:
   String(const char *);
   const char *c_str(void);
};
