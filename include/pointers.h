///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   pointers.h
// Purpose:     Memory management, mostly templates and macros
//              While MObject.h is for *defining* objects, pointers.h
//              is for *referencing* objects, so keep it short
// Author:      Robert Vazan
// Modified by:
// Created:     2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_POINTERS_H
#define M_POINTERS_H

/**
   An MObjectRC-based smart pointer implementation.

   RefCounter<> is a smart pointer which works with any MObjectRC-derived
   class. It has normal copying semantics unlike the std::auto_ptr<> as it uses
   ref counting.

   @todo To be extended...
 */
template <class T>
class RefCounter
{
public:
   /// same as auto_ptr<>::element_type
   typedef T element_type;

   /// synonym for element_type, doesn't exist in auto_ptr
   typedef T value_type;

   /// synonym for pointer to element_type, doesn't exist in auto_ptr
   typedef T *pointer;

   /// a scalar type which doesn't risk to be converted to anything
   typedef T *(RefCounter<T>::*unspecified_bool_type)() const;


   /**
      Default constructor creates an uninitialized pointer.

    */
   RefCounter() { m_pointer = NULL; }

   /**
      Constructor from a rawp ointer.

      We take the ownership of the pointer here, i.e. we will call DecRef() on
      it.

      @param pointer the pointer to take ownership of, may be NULL
    */
   explicit RefCounter(T *pointer) { m_pointer = pointer; }

   /**
      Copy constructor.
    */
   RefCounter(const RefCounter<T>& copy)
   {
      m_pointer = copy.m_pointer;
      RefCounterIncrement(m_pointer);
   }

   /**
      Assignment operator.

      As with copy constructor, this operator has normal semantics and doesn't
      modify its argument.
    */
   RefCounter<T>& operator=(const RefCounter<T> &copy)
      { AttachAndIncRef(copy.m_pointer); return *this; }

   /**
      Destructor releases the pointer possibly destroying it.

      Destructor is not virtual, this class can't be used polymorphically.
    */
   ~RefCounter() { RefCounterDecrement(m_pointer); }

   /// returns the stored pointer, may be @c NULL
   T *Get() const { return m_pointer; }

   /// allows use of this class as a pointer, must be non @c NULL
   T& operator*() const { return *Get(); }

   /// allows use of this class as a pointer, must be non @c NULL
   T *operator->() const { return Get(); }

   /**
       Implicit, but safe, conversion to bool.

       Having this conversion ensures that we can work with the objects of
       this type as with the plain pointers but using a unique type (instead
       of bool or int) ensures that we don't risk to implicitly confuse the
       unrelated pointers.
    */
   operator unspecified_bool_type() const // never throws
   {
       return m_pointer ? &RefCounter<T>::Get : NULL;
   }

   void Attach(T *pointer)
   {
      RefCounterDecrement(m_pointer);
      m_pointer = pointer;
   }
   
   /**
      Releases ownership of the held pointer and returns it.

      The caller is responsible for calling DecRef() on the returned pointer if
      it is not @c NULL.
    */
   T *Release()
   {
      T *pointer = m_pointer;
      m_pointer = NULL;

      return pointer;
   }

   static RefCounter<T> Convert(T *pointer)
   {
      RefCounter<T> result;
      result.AttachAndIncRef(pointer);
      return result;
   }

   /// Expects object that has not been IncRef-ed yet, don't use if possible
   void AttachAndIncRef(T *pointer)
   {
      RefCounterAssign(m_pointer,pointer);
      m_pointer = pointer;
   }

private:
   T *m_pointer;
};


// Used to resolve cyclic references. RefCounter goes in one direction
// and WeakRef goes in the opposite direction. WeakRef (seems that it)
// contains NULL if all RefCounter instances are gone (and the object
// is deleted). WeakRef is a bit inefficient. That's why you should
// always convert it to RefCounter when you want to use object it points to.
template <class T>
class WeakRef
{
public:
   WeakRef() : m_pointer(NULL) {}
   WeakRef(const WeakRef<T> &copy)
      : m_pointer(copy.m_pointer)
      { WeakRefIncrement(m_pointer); }
   ~WeakRef() { WeakRefDecrement(m_pointer); }

   WeakRef(RefCounter<T> pointer)
      : m_pointer(pointer.Get())
      { WeakRefIncrement(m_pointer); }

   WeakRef<T>& operator=(const WeakRef<T> &copy)
   {
      WeakRefAssign(m_pointer,copy.m_pointer);
      m_pointer = copy.m_pointer;
      return *this;
   }

   WeakRef<T>& operator=(RefCounter<T> pointer)
   {
      m_pointer = pointer.Get();
      WeakRefIncrement(m_pointer);
      return *this;
   }

   bool Expired() const { return WeakRefExpired(m_pointer); }
   
   RefCounter<T> Get() const
   {
      RefCounter<T> result;
      result.AttachAndIncRef(Expired() ? NULL : m_pointer);
      return result;
   }
   
   operator RefCounter<T>() const { return Get(); }
   
private:
   T *m_pointer;
};


// Equivalent of auto_ptr, but with private copy constructor and assignment
template <class T>
class AutoPtr
{
public:
   AutoPtr() { NewDefault(); }
   AutoPtr(T *copy) { NewCopy(); }
   ~AutoPtr() { Destroy(); }

   void Initialize(T *copy)
   {
      Destroy();
      m_pointer = copy;
   }

   operator T *() { return Get(); }
   T *operator->() { return Get(); }

private:
   void NewDefault() { m_pointer = 0; }
   void NewCopy(T *copy) { m_pointer = copy; }
   void Destroy() { if( m_pointer ) delete m_pointer; }

   T *Get() { return m_pointer; }

   T *m_pointer;

   AutoPtr(const AutoPtr<T>& copy) {}
   void operator=(const AutoPtr<T>& copy) {}
};


// Use instead of forward declaration to make RefCounter and WeakRef
// instantiable without knowledge that T derives from MObjectRC.
#define DECLARE_REF_COUNTER(T) \
   class T; \
   extern void RefCounterIncrement(T *pointer); \
   extern void RefCounterDecrement(T *pointer); \
   extern void RefCounterAssign(T *target,T *source); \
   extern void WeakRefIncrement(T *pointer); \
   extern void WeakRefDecrement(T *pointer); \
   extern void WeakRefAssign(T *target,T *source); \
   extern bool WeakRefExpired(const T *pointer);


// If DECLARE_REF_COUNTER is used anywhere, DEFINE_REF_COUNTER must be
// put it some *.cpp file that #includes ClassName's header.
#define DEFINE_REF_COUNTER(T) \
   extern void RefCounterIncrement(T *pointer) \
      { RefCounterIncrement(static_cast<MObjectRC *>(pointer)); } \
   extern void RefCounterDecrement(T *pointer) \
      { RefCounterDecrement(static_cast<MObjectRC *>(pointer)); } \
   extern void RefCounterAssign(T *target,T *source) \
   { \
      RefCounterAssign(static_cast<MObjectRC *>(target), \
         static_cast<MObjectRC *>(source)); \
   } \
   extern void WeakRefIncrement(T *pointer) \
      { WeakRefIncrement(static_cast<MObjectRC *>(pointer)); } \
   extern void WeakRefDecrement(T *pointer) \
      { WeakRefDecrement(static_cast<MObjectRC *>(pointer)); } \
   extern void WeakRefAssign(T *target,T *source) \
   { \
      WeakRefAssign(static_cast<MObjectRC *>(target), \
         static_cast<MObjectRC *>(source)); \
   } \
   extern bool WeakRefExpired(const T *pointer) \
      { return WeakRefExpired(static_cast<const MObjectRC *>(pointer)); } \


#endif // M_POINTERS_H
