/*-*- c++ -*-********************************************************
 * String class interface for python                                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 *******************************************************************/

// naming this module String is a very bad idea for systems with
// case-insensitive file system (Windows...) because of standard Python
// module string.
%module 	MString
%{
#include  "Mcommon.h"

#include  <wx/config.h>

// we don't want to export our functions as we don't build a shared library
#if defined(__WIN32__)
#   undef SWIGEXPORT
// one of these must be defined and the other commented out
#   define SWIGEXPORT(a,b) a b
//#   define SWIGEXPORT(a) a
#endif
%}

class String 
{
public:
   String(const char *);
   const char *c_str(void);
};
