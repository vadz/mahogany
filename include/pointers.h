///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   pointers.h
// Purpose:     Memory management, mostly templates and macros
// Author:      Robert Vazan
// Modified by:
// Created:     2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_POINTERS_H
#define M_POINTERS_H

// Our MObjectRC-based smart pointer implementation. To be extended...
template <class ClassName>
class RefCounter
{
public:
   RefCounter() { NewDefault(); }
   // Expects IncRef-ed object
   RefCounter(ClassName *copy) { NewBare(copy); }
   RefCounter(const RefCounter<ClassName> &copy) { NewCopy(copy); }
   ~RefCounter() { Destroy(); }

   // Expects object that has not been IncRef-ed yet, don't use if possible
   void AttachAndIncRef(ClassName *pointer)
   {
      RefCounterAssign(m_pointer,pointer);
      m_pointer = pointer;
   }

   operator ClassName *() { return Get(); }
   ClassName *operator->() { return Get(); }
   operator bool() { return NotNull(); }
   RefCounter<ClassName>& operator=(const RefCounter<ClassName> &copy)
      { return Assign(copy); }

private:
   void NewDefault() { m_pointer = 0; }
   void NewCopy(const RefCounter<ClassName> &copy)
      { RefCounterIncrement(m_pointer = copy.m_pointer); }
   void NewBare(ClassName *copy) { m_pointer = copy; }
   void Destroy() { RefCounterDecrement(m_pointer); }
   
   RefCounter<ClassName>& Assign(const RefCounter<ClassName> &copy)
   {
      AttachAndIncRef(copy.m_pointer);
      return *this;
   }

   ClassName *Get() const { return m_pointer; }
   bool NotNull() const { return m_pointer != 0; }
   
   ClassName *m_pointer;
};

#define DECLARE_REF_COUNTER(ClassName) \
   class ClassName; \
   extern void RefCounterIncrement(ClassName *pointer); \
   extern void RefCounterDecrement(ClassName *pointer); \
   extern void RefCounterAssign(ClassName *target,ClassName *source);

#define DEFINE_REF_COUNTER(ClassName) \
   extern void RefCounterIncrement(ClassName *pointer) \
      { RefCounterIncrement(static_cast<MObjectRC *>(pointer)); } \
   extern void RefCounterDecrement(ClassName *pointer) \
      { RefCounterDecrement(static_cast<MObjectRC *>(pointer)); } \
   extern void RefCounterAssign(ClassName *target,ClassName *source) \
   { \
      RefCounterAssign(static_cast<MObjectRC *>(target), \
         static_cast<MObjectRC *>(source)); \
   }

class MObjectRC;
extern void RefCounterIncrement(MObjectRC *pointer);
extern void RefCounterDecrement(MObjectRC *pointer);
extern void RefCounterAssign(MObjectRC *target,MObjectRC *source);


// Equivalent of auto_ptr, but with private copy constructor and assignment
template <class ClassName>
class AutoPtr
{
public:
   AutoPtr() { NewDefault(); }
   AutoPtr(ClassName *copy) { NewCopy(); }
   ~AutoPtr() { Destroy(); }
   
   void Initialize(ClassName *copy)
   {
      Destroy();
      m_pointer = copy;
   }

   operator ClassName *() { return Get(); }
   ClassName *operator->() { return Get(); }
   
private:
   void NewDefault() { m_pointer = 0; }
   void NewCopy(ClassName *copy) { m_pointer = copy; }
   void Destroy() { if( m_pointer ) delete m_pointer; }
   
   ClassName *Get() { return m_pointer; }

   ClassName *m_pointer;
   
   AutoPtr(const AutoPtr<ClassName>& copy) {}
   void operator=(const AutoPtr<ClassName>& copy) {}
};

#endif // M_POINTERS_H
