///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/swigcmn.i: common stuff for several SWIG files
// Purpose:     hacks to make SWIG grok our headers
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// we don't want to export MObjectRC (it doesn't provide anything except
// reference counting which any self respecting script language already has
// anyhow), so just pretend it is empty for SWIG
class MObjectRC { };

// SWIG can't deal with the macros below -- no big deal, we don't need to
// export them anyhow, so just hide them from SWIG
#define WXDLLEXPORT
#define GCC_DTOR_WARN_OFF
#define MOBJECT_NAME(x)

#define DECLARE_AUTOPTR(x)
#define DECLARE_AUTOPTR_WITH_CONVERSION(x)
#define DECLARE_AUTOPTR_NO_REF(x)

// define typemaps for mapping our String (same as std::string) to Python
// string
%typemap(in) String {
   char * temps; int templ;
   if (PyString_AsStringAndSize($input, &temps, &templ)) return NULL;
   $1 = $1_ltype (temps, templ);
}
%typemap(in) const String& (String tempstr) {
   char * temps; int templ;
   if (PyString_AsStringAndSize($input, &temps, &templ)) return NULL;
   tempstr = String(temps, templ);
   $1 = &tempstr;
}

// this is for setting string structure members:
%typemap(in) String* ($*1_ltype tempstr) {
   char * temps; int templ;
   if (PyString_AsStringAndSize($input, &temps, &templ)) return NULL;
   tempstr = $*1_ltype(temps, templ);
   $1 = &tempstr;
}

%typemap(out) String "$result = PyString_FromStringAndSize($1.data(), $1.length());";
%typemap(out) const String& "$result = PyString_FromStringAndSize($1->data(), $1->length());";
%typemap(out) String* "$result = PyString_FromStringAndSize($1->data(), $1->length());";

%typemap(varin) String {
   char *temps; int templ;
   if (PyString_AsStringAndSize($input, &temps, &templ)) return NULL;
   $1 = $1_ltype(temps, templ);
}
%typemap(varout) String "$result = PyString_FromStringAndSize($1.data(), $1.length());";
