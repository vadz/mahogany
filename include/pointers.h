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
   RefCounter(ClassName *copy) { NewBare(copy); }
   RefCounter(const RefCounter<ClassName> &copy) { NewCopy(copy); }
   ~RefCounter() { Destroy(); }
   
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
      { NewBare(copy.m_pointer); }
   void NewBare(ClassName *copy)
      { RefCounterIncrement(m_pointer = copy); }
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

extern void RefCounterIncrement(MObjectRC *pointer);
extern void RefCounterDecrement(MObjectRC *pointer);
extern void RefCounterAssign(MObjectRC *target,MObjectRC *source);

#endif // M_POINTERS_H
