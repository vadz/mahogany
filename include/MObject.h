// -*- c++ -*-// //// //// //// //// //// //// //// //// //// //// //// //// //
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
// // //// //// //// //// //// //// //// //// //// //// //// //// //// //// ///

#ifndef   MOBJECT_H
#define   MOBJECT_H

// ----------------------------------------------------------------------------
// MObject: the mother of all classes
// ----------------------------------------------------------------------------

/// the magic number
#define   MOBJECT_MAGIC   1234567890

/** This class is base of all other M objects. It has a simple
    mechanism of magic numbers to check an object's validity.
    This class is empty if compiled with debugging disabled.
*/
class MObject
{
public:
#ifdef   DEBUG
   /// initialise the magic number
   MObject()
      { m_magic = MOBJECT_MAGIC; }
   /** Check validity of this object.
       This function should be called wherever such an object is used,
       especially at the beginning of all methods.
   */
   virtual void MOcheck(void) const
      {
         // check that this != NULL
         wxASSERT(this);
         // check that the object is really a MObject
         wxASSERT(m_magic == MOBJECT_MAGIC);
      }
   /// virtual destructor
   virtual ~MObject()
      {
         // check we are valid
         MOcheck();
         // make sure we are no longer
         m_magic = 0;
      }
protected:
   /// a simple magic number as a validity check
   int m_magic;
#else
   /// virtual destructor
   virtual ~MObject() {}
   /// empty MOcheck() method
   void MOcheck(void) const {}
#endif
};

// ----------------------------------------------------------------------------
// MObjectRC: the mother of all reference counted classes
// ----------------------------------------------------------------------------

/**
  This class uses reference counting.

  In practice, it means that:
    0) all objects of this class must be created on the heap: in particular,
       creating a global variable of this type will lead to a crash.
    1) you should never delete the objects of this class (in fact, you can't
       anyhow because dtor is private)
    2) instead call DecRef() when you're finished with the object (so that it
       may delete itself if it was the last reference)
    3) call IncRef() if you wish to have control over object's lifetime (i.e.
       before storing pointer to it - otherwise it may go away leaving you
       with invalid pointer) and, of course, call DecRef() for any IncRef().
    4) any function returning "MObjectRC *" locks it (according to 3), so you
       should call DecRef() when you're done with the returned pointer.

  Of course, "objects of this class" also applies for objects of all classes
  derived from MObjectRC.

  Debugging: in the debug mode only (when DEBUG is defined) all MObjectRC-derived
  objects are added to the global list. Calling MObjectRC::CheckLeaks() on program
  termination will print a detailed report about leaked objects, including their
  number and their description for all of them. CheckLeaks() does nothing in the
  release build.
*/
class MObjectRC : public MObject
{
public:
  // ctor creates the object with the ref. count of 1
#ifdef   DEBUG
  MObjectRC();
#else
  MObjectRC() { m_nRef = 1; }
#endif

  // debugging support
#ifdef   DEBUG
    // call this function on program termination to check for memory leaks
    // (of course, you shouldn't allocate memory in static object's: otherwise
    // it will be reported as leaked)
    static void CheckLeaks();

    // override this function (see also MOBJECT_DEBUG macro) to provide some
    // rich information about your object (MObjectRC::Dump() prints the ref
    // count only)
    virtual String Dump() const;

#else
    static void CheckLeaks() { }
#endif

  // ref counting
#ifdef   DEBUG
    // increment
  void IncRef();
    // decrement and delete if reached 0, return TRUE if item wasn't deleted
  bool DecRef();
#else  //release
  void IncRef()
    { MOcheck(); wxASSERT(m_nRef > 0); m_nRef++; }
  bool DecRef()
    { MOcheck(); if ( --m_nRef ) return TRUE; delete this; return FALSE; }
#endif //debug/release

protected:
  /// dtor is protected because only DecRef() can delete us
  virtual ~MObjectRC() {}

#ifndef DEBUG // we may use m_nRef only for diagnostic functions
private:
#endif

  size_t m_nRef;  // always > 0 - as soon as it becomes 0 we delete ourselves
};

#ifdef   DEBUG
     /// declare all diagnostic functions (you must still implement them!)
#   define MOBJECT_DEBUG public: virtual String Dump() const;
#else
#   define MOBJECT_DEBUG
#endif

// ----------------------------------------------------------------------------
// utility functions
// ----------------------------------------------------------------------------

// lock the pointer only if it's !NULL
inline void SafeIncRef(MObjectRC *p) { if ( p != NULL ) p->IncRef(); }

// unlock the pointer only if it's !NULL
inline void SafeDecRef(MObjectRC *p) { if ( p != NULL ) p->DecRef(); }

#endif  //MOBJECT_H
