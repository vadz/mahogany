///////////////////////////////////////////////////////////////////////////////
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

#ifndef   _MOBJECT_H
#define   _MOBJECT_H

// ----------------------------------------------------------------------------
// MObject: the mother of all reference counted classes
// ----------------------------------------------------------------------------

/**
  This class uses reference counting.

  In practice, it means that:
    0) all objects of this class must be created on the heap: in particular,
       creating a global variable of this type will lead to a crash.
    1) you should never delete the objects of this class (in fact, you can't
       anyhow because dtor is private)
    2) instead call Unlock() when you're finished with the object (so that it
       may delete itself if it was the last reference)
    3) call Lock() if you wish to have control over object's lifetime (i.e.
       before storing pointer to it - otherwise it may go away leaving you
       with invalid pointer) and, of course, call Unlock() for any Lock().
    4) any function returning "MObject *" locks it (according to 3), so you
       should call Unlock() when you're done with the returned pointer.

  Of course, "objects of this class" also applies for objects of all classes
  derived from MObject.

  Debugging: in the debug mode only (when DEBUG is defined) all MObject-derived
  objects are added to the global list. Calling MObject::CheckLeaks() on program
  termination will print a detailed report about leaked objects, including their
  number and their description for all of them. CheckLeaks() does nothing in the
  release build.
*/
class MObject
{
public:
  // ctor creates the object with the ref. count of 1
#ifdef   DEBUG
  MObject();
#else
  MObject() { m_nRef = 1; }
#endif

  // debugging support
#ifdef   DEBUG
    /// check validity of this object
    void MOcheck(void) const
       {
          wxASSERT(this);
          wxASSERT(m_magic);
       }

    // call this function on program termination to check for memory leaks
    // (of course, you shouldn't allocate memory in static object's: otherwise
    // it will be reported as leaked)
    static void CheckLeaks();

    // override this function (see also MOBJECT_DEBUG macro) to provide some
    // rich information about your object (MObject::Dump() prints the ref
    // count only)
    virtual String Dump() const;

#else
#   define MOcheck()
    static void CheckLeaks() { }
#endif

    // ref counting
    // increment
  void Lock()   { MOcheck(); wxASSERT(m_nRef > 0); m_nRef++; }
    // decrement and delete if reached 0, return TRUE if item wasn't deleted
#ifdef   DEBUG
  bool Unlock();
#else
  bool Unlock() { MOcheck(); if ( --m_nRef ) return TRUE; delete this; return FALSE; }
#endif


protected:
#ifdef DEBUG
  int m_magic;
  // dtor is protected because only Unlock() can delete us
  virtual ~MObject() { MOcheck(); m_magic = 0; }
#else
  // dtor is protected because only Unlock() can delete us
    virtual ~MObject() {}
#endif
  
#ifndef DEBUG // we may use m_nRef only for diagnostic functions
private:
#endif

  size_t m_nRef;  // always > 0 - as soon as it becomes 0 we delete ourselves
};

#ifdef   DEBUG
  // declare all diagnostic functions (you must still implement them!)
  #define MOBJECT_DEBUG public: virtual String Dump() const;
#else
  #define MOBJECT_DEBUG
#endif

// ----------------------------------------------------------------------------
// utility functions
// ----------------------------------------------------------------------------

// unlock the pointer only if it's empty
inline void SafeUnlock(MObject *p) { if ( p != NULL ) p->Unlock(); }

#endif  //_MOBJECT_H
