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

// note: this source file is intentionally empty in release mode, all the code
// here is only for the debugging support, otherwise the functions are inline in
// the header.
#include "MObject.h"

#ifdef DEBUG

#include <wx/log.h>
#include <wx/dynarray.h>

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
      wxFAIL_MSG("MObjectRC memory leaks detected, see debug log for details.");

      wxLogDebug("%d MObjectRCs leaked:", nCount);
   }

   for ( size_t n = 0; n < nCount; n++ ) {
      wxLogDebug("Object %d: %s", n, gs_aObjects[n]->DebugDump().c_str());
   }
}

String MObjectRC::DebugDump() const
{
   return MObject::DebugDump() + String::Format(" m_nRef = %d: ", m_nRef);
}

void MObject::CheckLeaks()
{
   // this is now done in MObjectRC::CheckLeaks()
}

String MObject::DebugDump() const
{
   MOcheck();
   String str;
   str.Printf("%s at 0x%p", DebugGetClassName(), this);

   return str;
}

// ----------------------------------------------------------------------------
// debug implementations of other functions
// ----------------------------------------------------------------------------

MObjectRC::MObjectRC()
{
   gs_aObjects.Add(this);
   m_nRef = 1;
}

void MObjectRC::IncRef()
{
   MOcheck();
   wxASSERT(m_nRef > 0);
   m_nRef++;

   if ( this == gs_traceObject )
   {
      wxLogTrace("Object %x: IncRef() called, m_nRef = %u.", this, m_nRef);
   }
}

bool MObjectRC::DecRef()
{
   MOcheck();
   wxASSERT(m_nRef > 0);

   m_nRef--;

   if ( this == gs_traceObject )
   {
      wxLogTrace("Object %x: DecRef() called, m_nRef = %u.", this, m_nRef);
   }

   if ( m_nRef == 0 )
   {
      gs_aObjects.Remove(this);
      delete this;

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
