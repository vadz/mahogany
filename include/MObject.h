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

class WeakRefCounter;
class MObjectRC;

// ----------------------------------------------------------------------------
// Private smart pointer helpers
// ----------------------------------------------------------------------------

extern void RefCounterIncrement(MObjectRC *pointer);
extern void RefCounterDecrement(MObjectRC *pointer);
extern void RefCounterAssign(MObjectRC *target,MObjectRC *source);
extern WeakRefCounter *WeakRefAdd(MObjectRC *pointer);
extern void WeakRefDeleted(WeakRefCounter *counter);

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
      {
         m_magic = MOBJECT_MAGIC;
         Register();
      }
   MObject(const MObject &oldobj)
      {
         oldobj.MOcheck();
         m_magic = MOBJECT_MAGIC;
         Register();
      }
   /** Check validity of this object.
       This function should be called wherever such an object is used,
       especially at the beginning of all methods.
   */
   virtual void MOcheck(void) const
      {
         /// check that this != NULL
         wxASSERT(this);
         /// check that the object is really a MObject
         wxASSERT(m_magic == MOBJECT_MAGIC);
      }
   /// virtual destructor
   virtual ~MObject()
      {
         // check we are valid
         MOcheck();
         DeRegister();
         // make sure we are no longer
         m_magic = 0;
      }
   

    /// call this function on program termination to check for memory
    /// leaks (of course, you shouldn't allocate memory in static
    /// objects: otherwise it will be reported as leaked)
    static void CheckLeaks();

    /// override this function (see also MOBJECT_DEBUG macro) to provide
    /// some rich information about your object (MObjectRC::Dump() prints
    /// the base information such as name, pointer and ref count only)
    virtual String DebugDump() const;

    /// this function just returns the class name (also overriden by
    /// MOBJECT_DEBUG macro)
    virtual const char *DebugGetClassName() const { return "<<Invalid>>"; }

private:
   /// Adds the object to list of MObjects
   void Register(void);
   /// Removes the object from the list.
   void DeRegister(void);
protected:
   /// a simple magic number as a validity check
   int m_magic;
#else
   /// virtual destructor
   virtual ~MObject() {}
   /// empty MOcheck() method
   void MOcheck(void) const {}
   /// nothing
   static void CheckLeaks() { }
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

  Debugging: in the debug mode only (when DEBUG is defined) all MObjectRC-
  derived objects are added to the global list. Calling MObjectRC::CheckLeaks()
  on program termination will print a detailed report about leaked objects,
  including their number and their description. CheckLeaks() does nothing in
  the release build. You can also insert the MOBJECT_DEBUG(classname) macro
  into the declaration of the class classname to provide some more interesting
  information about the instance of this class for debugging purposes (it will
  also put the right classname in the leaked object report then)
*/
class MObjectRC : public MObject
{
public:
  /// ctor creates the object with the ref. count of 1
#ifdef   DEBUG
  MObjectRC();
#else
  MObjectRC() { m_nRef = 1; m_weak = 0; }
#endif

  /// debugging support
#ifdef   DEBUG
    /// call this function on program termination to check for memory
    /// leaks (of course, you shouldn't allocate memory in static objects:
    /// otherwise it will be reported as leaked)
    static void CheckLeaks();

    /// override this function (see also MOBJECT_DEBUG macro) to provide
    /// some rich information about your object (MObjectRC::Dump() prints
    /// the base information such as name, pointer and ref count only)
    virtual String DebugDump() const;

    /// this function just returns the class name (also overriden by
    /// MOBJECT_DEBUG macro)
    virtual const char *DebugGetClassName() const { return "<<Unknown>>"; }
#else
    static void CheckLeaks() { }
#endif

  /// ref counting
#ifdef   DEBUG
    /// increment
   virtual void IncRef();
    /// decrement and delete if reached 0, return TRUE if item wasn't deleted
   virtual bool DecRef();
#else  ///release
   virtual void IncRef()
    { MOcheck(); wxASSERT(m_nRef > 0); m_nRef++; }
   virtual bool DecRef()
    { MOcheck(); if ( --m_nRef ) return TRUE; delete this; return FALSE; }
#endif ///debug/release

protected:
   //// dtor is protected because only DecRef() can delete us
   virtual ~MObjectRC()
      { MOcheck(); wxASSERT(m_nRef == 0); WeakRefDeleted(m_weak); }
   //// return the reference count:
   size_t GetNRef(void) const { return m_nRef; }
#ifndef DEBUG // we may use m_nRef only for diagnostic functions
private:
#endif

   size_t m_nRef;  // always > 0 - as soon as it becomes 0 we delete ourselves

   // Support for WeakRef
   friend WeakRefCounter *WeakRefAdd(MObjectRC *pointer);
   WeakRefCounter *m_weak;
};

#ifdef   DEBUG
     /// declare all diagnostic functions (you must implement DebugDump)
#   define MOBJECT_DEBUG(classname)                                           \
      public:                                                                 \
         virtual const char *DebugGetClassName() const { return #classname; } \
         virtual String DebugDump() const;

#   define MOBJECT_NAME(classname) \
      public:                                                                 \
         virtual const char *DebugGetClassName() const { return #classname; } 
#else
#   define MOBJECT_DEBUG(classname)
#   define MOBJECT_NAME(classname)
#endif

// ----------------------------------------------------------------------------
// smart references classes: for an MObjectRC-derived class Foo we provide
// macros to define a class Foo_obj which is a smart reference to Foo and also
// more flexible macros to allow adding arbitrary code to Foo_obj declaration
//
// in fact, this works for any class which has a DecRef() method, not just
// MObjectRC
// ----------------------------------------------------------------------------

// start auto ptr class declaration
#define BEGIN_DECLARE_AUTOPTR_NO_BOOL(classname)                              \
   class classname##_obj                                                      \
   {                                                                          \
   public:                                                                    \
      classname##_obj(classname *ptr = NULL) { m_ptr = ptr; }                 \
                                                                              \
      void Attach(classname *ptr)                                             \
      {                                                                       \
         ASSERT_MSG( !m_ptr, _T("should have used Detach() first") );         \
                                                                              \
         m_ptr = ptr;                                                         \
      }                                                                       \
                                                                              \
      classname *Detach()                                                     \
      {                                                                       \
         classname *ptr = m_ptr;                                              \
         m_ptr = NULL;                                                        \
         return ptr;                                                          \
      }                                                                       \
                                                                              \
      classname *Get() const { return m_ptr; }                                \
                                                                              \
      classname *operator->() const { return Get(); }                         \
                                                                              \
      void Swap(classname##_obj& other)                                       \
      {                                                                       \
         classname *tmp = other.m_ptr;                                        \
         other.m_ptr = m_ptr;                                                 \
         m_ptr = tmp;                                                         \
      }                                                                       \
                                                                              \
   private:                                                                   \
      classname *m_ptr;                                                       \
                                                                              \
      classname##_obj(const classname##_obj &);                               \
      classname##_obj& operator=(const classname##_obj &);

// normally our autoptr class has an implicit conversion to bool for truth
// testing but we can't have both conversion to bool and to a pointer type as
// in DECLARE_AUTOPTR_WITH_CONVERSION() because of ambiguity between them, so
// we have a separate macro for the part without bool conversion and another
// one for the whole declaration
#define BEGIN_DECLARE_AUTOPTR(classname)                    \
   BEGIN_DECLARE_AUTOPTR_NO_BOOL(classname)                 \
   public:                                                  \
      ~classname##_obj() { if ( m_ptr ) m_ptr->DecRef(); }  \
      operator bool() const { return m_ptr != NULL; }

// finish the class decl
#define END_DECLARE_AUTOPTR() }

// declare a class which is an auto ptr to the given MObjectRC-derived type
#define DECLARE_AUTOPTR(classname)                    \
   BEGIN_DECLARE_AUTOPTR(classname)                   \
   END_DECLARE_AUTOPTR()

// declare an auto ptr with implicit conversion to its real pointer class:
// dangerous but needed for backwards compatibility
#define DECLARE_AUTOPTR_WITH_CONVERSION(classname)          \
   BEGIN_DECLARE_AUTOPTR_NO_BOOL(classname)                 \
   public:                                                  \
      ~classname##_obj() { if ( m_ptr ) m_ptr->DecRef(); }  \
      operator classname *() const { return m_ptr; }        \
   END_DECLARE_AUTOPTR()

// same thing for non ref counted pointers
#define BEGIN_DECLARE_AUTOPTR_NO_REF(classname)             \
   BEGIN_DECLARE_AUTOPTR_NO_BOOL(classname)                 \
   public:                                                  \
      ~classname##_obj() { delete m_ptr; }                  \
      operator bool() const { return m_ptr != NULL; }

// declare an class which is an auto ptr to the given MObjectRC-derived type
#define DECLARE_AUTOPTR_NO_REF(classname)                   \
   BEGIN_DECLARE_AUTOPTR_NO_REF(classname)                  \
   END_DECLARE_AUTOPTR()

// ----------------------------------------------------------------------------
// utility functions
// ----------------------------------------------------------------------------

// lock the pointer only if it's !NULL
inline void SafeIncRef(MObjectRC *p) { if ( p != NULL ) p->IncRef(); }

// unlock the pointer only if it's !NULL
inline void SafeDecRef(MObjectRC *p) { if ( p != NULL ) p->DecRef(); }

#endif  //MOBJECT_H
