///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MObject.cpp - implementation of MObjectRC
// Comment:     the only non inline MObjectRC functions are diagnostic ones
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef USE_PCH
#  ifdef DEBUG
#     include <wx/log.h>
#     include <wx/dynarray.h>        // for WX_DEFINE_ARRAY
#  endif
#endif // USE_PCH

#include "MObject.h"
#include "pointers.h"

#ifdef DEBUG

// ----------------------------------------------------------------------------
// this module global variables
// ----------------------------------------------------------------------------
WX_DEFINE_ARRAY(MObjectRC *, ArrayObjects);

static ArrayObjects gs_aObjects;

WX_DEFINE_ARRAY(MObject *, ArrayMObjects);

static ArrayMObjects gs_aMObjects;

// we don't want to trace all Inc/DecRef()s - there are too many of them. But
// sometimes we want to trace the lifetime of a selected object. For this, you
// should change these variable under debugger - all operations on this object
// will be logged.
static void *gs_traceObject = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// debug only functions
// ----------------------------------------------------------------------------

void MObjectRC::CheckLeaks()
{
   size_t nCount = gs_aObjects.Count();

   if ( nCount > 0 ) {
      wxLogDebug(_T("MEMORY LEAK: %lu object leaked:"), (unsigned long)nCount);
   }

   for ( size_t n = 0; n < nCount; n++ ) {
      wxLogDebug(_T("Object %lu: %s"),
                 (unsigned long)n, gs_aObjects[n]->DebugDump().c_str());
   }
}

String MObjectRC::DebugDump() const
{
   return MObject::DebugDump() + String::Format(_T(" m_nRef = %lu: "), (unsigned long)m_nRef);
}

void MObject::CheckLeaks()
{
   // this is now done in MObjectRC::CheckLeaks()
}

String MObject::DebugDump() const
{
   MOcheck();
   String str;
   str.Printf(_T("%s at %p"), DebugGetClassName(), this);

   return str;
}

// ----------------------------------------------------------------------------
// debug implementations of other functions
// ----------------------------------------------------------------------------

MObjectRC::MObjectRC()
{
   gs_aObjects.Add(this);
   m_nRef = 1;
   m_weakRef = 0;
}

void MObjectRC::IncRef()
{
   MOcheck();
   wxASSERT(m_nRef > 0);
   m_nRef++;

   if ( this == gs_traceObject )
   {
      wxLogTrace(_T("Object %p: IncRef() called, m_nRef = %lu."), this, (unsigned long)m_nRef);
   }
}

bool MObjectRC::DecRef()
{
   MOcheck();
   wxASSERT(m_nRef > 0);

   m_nRef--;

   if ( this == gs_traceObject )
   {
      wxLogTrace(_T("Object %p: DecRef() called, m_nRef = %lu."), this, (unsigned long)m_nRef);
   }

   if ( m_nRef == 0 )
   {
      gs_aObjects.Remove(this);
      this->~MObjectRC();
      if( !m_weakRef )
         ::operator delete(this);

      return FALSE;
   }

   return TRUE;
}


void MObject::Register(void)
{
   MOcheck();
   gs_aMObjects.Add(this);
}

void MObject::DeRegister(void)
{
   MOcheck();
   gs_aMObjects.Remove(this);
}

#endif //DEBUG

// ----------------------------------------------------------------------------
// Reference counting helpers
// ----------------------------------------------------------------------------

extern void RefCounterIncrement(MObjectRC *pointer)
{
   if( pointer )
      pointer->IncRef();
}

extern void RefCounterDecrement(MObjectRC *pointer)
{
   if( pointer )
      pointer->DecRef();
}

extern void RefCounterAssign(MObjectRC *target,MObjectRC *source)
{
   if( source )
      source->IncRef();
   if( target )
      target->DecRef();
}

extern void WeakRefIncrement(MObjectRC *pointer)
{
   if( pointer )
      ++pointer->m_weakRef;
}

extern void WeakRefDecrement(MObjectRC *pointer)
{
   if( pointer )
   {
      --pointer->m_weakRef;
      if( !pointer->m_weakRef && !pointer->m_nRef )
      {
         ::operator delete(pointer);
      }
   }
}

extern void WeakRefAssign(MObjectRC *target,MObjectRC *source)
{
   WeakRefIncrement(source);
   WeakRefDecrement(target);
}

extern void *WeakRefConvert(MObjectRC *pointer)
{
   if( !pointer->m_nRef )
      return NULL;

   pointer->IncRef();
   return pointer;
}
