#ifndef   _MPCH_H
#define   _MPCH_H

/// make sure NULL is define properly
#undef  NULL
#define NULL    0

#include        "Mconfig.h"

// Microsoft Visual C++
#ifdef  _MSC_VER
  // suppress the warning "identifier was truncated to 255 characters 
  // in the debug information"
  #pragma warning(disable: 4786)

  // you can't mix iostream.h and iostream, the former doesn't compile
  // with "using namespace std", the latter doesn't compile with wxWin
  // make your choice...
  #ifndef USE_IOSTREAMH
    #define USE_IOSTREAMH   1
  #endif

  // <string> includes <istream> (Grrr...)
  #if     USE_IOSTREAMH
    #undef  USE_WXSTRING
    #define USE_WXSTRING    1
  #endif
#endif  // VC++

#if           USE_IOSTREAMH
#       include <iostream.h>
#       include <fstream.h>
#else
#       include <iostream>
#       include <fstream>
#endif

#include        <list>
#include        <map>

#if           USE_IOSTREAMH
  // can't use namespace std because old iostream doesn't compile with it
  // and can't use std::list because it's a template class
#else
  using namespace std;
#endif

#if         USE_WXWINDOWS
#       if        WXWINDOWS2
#               define  WXCPTR  /**/
#       else
#               define  WXCPTR  (char *)
#       endif
#endif

#if         USE_WXSTRING
# if        !USE_WXWINDOWS2
#               define  c_str() GetData()
#               define  length() Length()       //FIXME dangerous!
#       endif
#       define  String      wxString
#else
#       include <string>
  typedef std::string String;
#endif

#define Bool    int

// includes for c-client library
extern "C"
{
  #include      <mail.h>
  #include      <osdep.h>
  #include      <rfc822.h>
  #include      <smtp.h>
  #include      <nntp.h>

  // windows.h included from osdep.h #defines all these
  #undef    GetMessage
  #undef    FindWindow
  #undef    GetCharWidth
  #undef    LoadAccelerators
}

#include        "guidef.h"

#include        "strutil.h"
#include        "appconf.h"

#endif  //_MPCH_H
