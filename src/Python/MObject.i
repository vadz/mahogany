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

// ----------------------------------------------------------------------------
// MObject: the mother of all classes
// ----------------------------------------------------------------------------

/// the magic number
#define   MOBJECT_MAGIC   1234567890

%}

%import MString.i

/** This class is base of all other M objects. It has a simple
    mechanism of magic numbers to check an object's validity.
    This class is empty if compiled with debugging disabled.
*/
class MObject
{
public:
#ifdef   DEBUG
   MObject();
   void MOcheck(void) const;
   ~MObject();
protected:
   int m_magic;
#else
   ~MObject();
   void MOcheck(void) const;
#endif
};

// ----------------------------------------------------------------------------
// MObjectRC: the mother of all reference counted classes
// ----------------------------------------------------------------------------

class MObjectRC : public MObject
{
public:
  // ctor creates the object with the ref. count of 1
  MObjectRC();

#ifdef   DEBUG
    static void CheckLeaks();
    String Dump() const;
#else
    static void CheckLeaks() { }
#endif

  void IncRef();
  bool DecRef();
protected:
  /// dtor is protected because only DecRef() can delete us
  ~MObjectRC();
#ifndef DEBUG // we may use m_nRef only for diagnostic functions
private:
#endif
  size_t m_nRef;  // always > 0 - as soon as it becomes 0 we delete ourselves
};

// ----------------------------------------------------------------------------
// utility functions
// ----------------------------------------------------------------------------

