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
   RefCounter() { m_pointer = 0; }
   ~RefCounter() { RefCounterDecrement(m_pointer); }
   
   void AttachAndIncRef(ClassName *pointer)
   {
      RefCounterAssign(m_pointer,pointer);
      m_pointer = pointer;
   }

   ClassName *Get() { return m_pointer; }
   bool NotNull() { return m_pointer != 0; }
   
   operator ClassName *() { return Get(); }
   ClassName *operator->() { return Get(); }
   operator bool() { return NotNull(); }

private:
   ClassName *m_pointer;
};

#define DECLARE_REF_COUNTER(ClassName) \
   extern void RefCounterDecrement(ClassName *pointer); \
   extern void RefCounterAssign(ClassName *target,ClassName *source);

#define DEFINE_REF_COUNTER(ClassName) \
   extern void RefCounterDecrement(ClassName *pointer) \
      { RefCounterDecrement(static_cast<MObjectRC *>(pointer)); } \
   extern void RefCounterAssign(ClassName *target,ClassName *source) \
   { \
      RefCounterAssign(static_cast<MObjectRC *>(target), \
         static_cast<MObjectRC *>(source)); \
   }

extern void RefCounterDecrement(MObjectRC *pointer);
extern void RefCounterAssign(MObjectRC *target,MObjectRC *source);

#endif // M_POINTERS_H
