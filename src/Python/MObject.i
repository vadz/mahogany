// -*- c++ -*-/////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MObject.h - the base class for all ref counted objects
// Purpose:     MObject provides the standard lock/unlock methods and deletes
//              itself when it's ref count reaches 0. As a consequence, it
//              should only be allocated with "new" and never deleted.
// Author:      Vadim Zeitlin
// Modified by: 
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////
%module   MObject
%{
#include   "Mconfig.h"
#include   "Mcommon.h"   
#include   "MObject.h"
// we don't want to export our functions as we don't build a shared library
#if defined(__WIN32__)
#   undef SWIGEXPORT
// one of these must be defined and the other commented out
#   define SWIGEXPORT(a,b) a b
//#   define SWIGEXPORT(a) a
#endif
%}

%import MString.i

%include "../../include/MObject.h"

