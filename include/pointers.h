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


   Rules:
   
   All newly allocated objects are passed to RefCounter constructor,
   for example:
   
      RefCounter<MyClass> ref(new MyClass(x,y));
   
   Pointer to "this" or any other raw pointer can be converted to
   RefCounter using Convert member, for example:
   
      RefCounter<MyClass> ref(RefCounter<MyClass>::convert(this));

   RefCounter members must not form cycles. It should be possible to create
   graph of classes where arrows go from classes containing RefCounter to
   classes used as template parameter to such RefCounter. Cases, where
   backreference to parent is needed, can be resolved using WeakRef to
   parent. Recurrent structures should be modelled using tree container.


   Compatibility with legacy raw pointer interfaces:
   
   Raw pointers are converted in one of two ways depending on
   whether they are already IncRef-ed or not:
   
      // IncRef in GetProfile
      ref.attach(x->GetProfile());
   
      // IncRef in Convert
      ref.attach(RefCounter<MyClass>::convert(x->GetProfile()));
   
   Local RefCounter can be returned IncRef-ed using Release:
   
      return x.release();

   Raw pointer parameters are not IncRef-ed. They are passed using Get:
   
      SomeFunc(x.get());
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
      Default constructor creates NULL pointer.
    */
   RefCounter() : m_pointer(NULL) {}

   /**
      Constructor from a raw pointer.

      We take the ownership of the pointer here, i.e. we will call DecRef() on
      it.

      @param pointer the pointer to take ownership of, may be NULL
    */
   explicit RefCounter(T *pointer) : m_pointer(pointer) {}

   /**
      Copy constructor.
    */
   RefCounter(const RefCounter<T>& copy)
      : m_pointer(copy.m_pointer)
   {
      RefCounterIncrement(m_pointer);
   }

   /**
      Assignment operator.

      As with copy constructor, this operator has normal semantics and doesn't
      modify its argument.
    */
   RefCounter<T>& operator=(const RefCounter<T> &copy)
   {
      RefCounterAssign(m_pointer,copy.m_pointer);
      m_pointer = copy.m_pointer;
      return *this;
   }

   /**
      Destructor releases the pointer possibly destroying it.

      Destructor is not virtual, this class can't be used polymorphically.
    */
   ~RefCounter() { RefCounterDecrement(m_pointer); }

   /// returns the stored pointer, may be @c NULL
   T *get() const { return m_pointer; }

   /// allows use of this class as a pointer, must be non @c NULL
   T& operator*() const { return *get(); }

   /// allows use of this class as a pointer, must be non @c NULL
   T *operator->() const { return get(); }

   /**
       Implicit, but safe, conversion to bool.

       Having this conversion ensures that we can work with the objects of
       this type as with the plain pointers but using a unique type (instead
       of bool or int) ensures that we don't risk to implicitly confuse the
       unrelated pointers.
    */
   operator unspecified_bool_type() const // never throws
   {
       return m_pointer ? &RefCounter<T>::get : NULL;
   }

   /**
      Reset to @c NULL.
      
      It looks better than @c NULL assignment and it saves some cycles.
    */
   void reset()
   {
      RefCounterDecrement(m_pointer);
      m_pointer = NULL;
   }
   
   /**
      Takes ownership of pointer.
      
      The caller has already called IncRef() on the pointer if it is not
      @c NULL.
    */
   void attach(T *pointer)
   {
      RefCounterDecrement(m_pointer);
      m_pointer = pointer;
   }
   
   /**
      Releases ownership of the held pointer and returns it.

      The caller is responsible for calling DecRef() on the returned pointer if
      it is not @c NULL.
    */
   T *release()
   {
      T *pointer = m_pointer;
      m_pointer = NULL;

      return pointer;
   }

   /**
      Converts raw pointer to smart pointer.
      
      The pointer is not manipulated in any way. Returned RefCounter
      calls IncRef() for its own copy of the pointer.
    */
   static RefCounter<T> convert(T *pointer)
   {
      RefCounterIncrement(pointer);
      return RefCounter<T>(pointer);
   }

private:
   T *m_pointer;
};


/**
   Weak pointer complementary to RefCounter.
   
   WeakRef (seems that it) contains NULL if all RefCounter instances
   are gone. WeakRef cannot be used directly. It must be converted to
   RefCounter first.
   
   WeakRef is used to resolve cyclic references. See RefCounter for details.
   
   WeakRef is intrusively counted weak pointer. This means that MObjectRC
   holds not only count of RefCounter instances, but also count of WeakRef
   instances. When all RefCounter instances are destroyed, MObjectRC
   destructor is called, but memory is not freed. When all WeakRef instances
   are destroyed, operator delete is called and the object finally
   disappears. This means that memory occupied by an object is held allocated
   until all WeakRef instances, that point to it, are destroyed or
   overwritten. It is not wasteful, because most uses of WeakRef are for
   parent backlinks and these should never become NULL.
 */
template <class T>
class WeakRef
{
public:
   /// same as auto_ptr<>::element_type
   typedef T element_type;

   /// Default constructor creates NULL pointer.
   WeakRef() : m_pointer(NULL) {}
   
   /// Copy constructor.
   WeakRef(const WeakRef<T> &copy)
      : m_pointer(copy.m_pointer)
   {
      WeakRefIncrement(m_pointer);
   }
   
   /// Destructor
   ~WeakRef() { WeakRefDecrement(m_pointer); }

   /// Conversion from RefCounter to WeakRef.
   WeakRef(RefCounter<T> pointer)
      : m_pointer(pointer.Get())
   {
      WeakRefIncrement(m_pointer);
   }

   /// Assignment operator.
   WeakRef<T>& operator=(const WeakRef<T> &copy)
   {
      WeakRefAssign(m_pointer,copy.m_pointer);
      m_pointer = copy.m_pointer;
      return *this;
   }

   /// Conversion from RefCounter to already constructed WeakRef.
   WeakRef<T>& operator=(RefCounter<T> pointer)
   {
      m_pointer = pointer.get();
      WeakRefIncrement(m_pointer);
      return *this;
   }
   
   /// Conversion from WeakRef to RefCounter.
   RefCounter<T> lock() const
   {
      return RefCounter<T>(WeakRefConvert(m_pointer));
   }
   
   /// Implicit conversion from WeakRef to RefCounter.
   operator RefCounter<T>() const { return lock(); }
   
private:
   T *m_pointer;
};


/// Mostly boost::scoped_ptr clone.
template <class T>
class scoped_ptr
{
public:
   /// same as auto_ptr<>::element_type
   typedef T element_type;

   /// a scalar type which doesn't risk to be converted to anything
   typedef T *(scoped_ptr<T>::*unspecified_bool_type)() const;

   /// Default constructor initializes to @c NULL.
   scoped_ptr() : m_pointer(NULL) {}

   /// Takes ownership of raw pointer
   scoped_ptr(T *copy) : m_pointer(copy) {}

   /// Destructor deletes held pointer if it is not @c NULL.
   ~scoped_ptr() { if( m_pointer ) delete m_pointer; }

   /// Late construction. Delete previously help pointer.
   void set(T *copy)
   {
      if( m_pointer )
         delete m_pointer;
      m_pointer = copy;
   }

   /// Return stored pointer.
   T *get() const { return m_pointer; }

   /// Allow use of this class as pointer.
   T *operator->() const { return get(); }

   /**
      Implicit, but safe, conversion to bool.

      It's copy of similar method in RefCounter.
    */
   operator unspecified_bool_type() const // never throws
   {
      return m_pointer ? &scoped_ptr<T>::get : NULL;
   }

private:
   /// Copy constructor is private.
   scoped_ptr(const scoped_ptr<T>& copy) {}
   
   /// Assignment operator is private.
   void operator=(const scoped_ptr<T>& copy) {}

   T *m_pointer;
};


/**
   Use instead of forward declaration to make RefCounter and WeakRef
   instantiable without knowledge that T derives from MObjectRC.
 */
#define DECLARE_REF_COUNTER(T) \
   class T; \
   extern void RefCounterIncrement(T *pointer); \
   extern void RefCounterDecrement(T *pointer); \
   extern void RefCounterAssign(T *target,T *source); \
   extern void WeakRefIncrement(T *pointer); \
   extern void WeakRefDecrement(T *pointer); \
   extern void WeakRefAssign(T *target,T *source); \
   extern T *WeakRefConvert(T *pointer);


/**
   If DECLARE_REF_COUNTER is used anywhere, DEFINE_REF_COUNTER must be
   put it some *.cpp file that #includes ClassName's header.
 */
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
   extern T *WeakRefConvert(T *pointer) \
      { return (T *)WeakRefConvert(static_cast<MObjectRC *>(pointer)); }


#endif // M_POINTERS_H
