///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MObject.cpp - implementation of MObject
// Comment:     the only non inline MObject functions are diagnostic ones
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

#ifdef DEBUG

#include "MObject.h"

#include <wx/log.h>
#include <wx/dynarray.h>

// ----------------------------------------------------------------------------
// this module global variables
// ----------------------------------------------------------------------------
WX_DEFINE_ARRAY(MObject *, ArrayObjects);

static ArrayObjects gs_aObjects;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// debug only functions
// ----------------------------------------------------------------------------
void MObject::CheckLeaks()
{
   size_t nCount = gs_aObjects.Count();

   if ( nCount > 0 ) {
      wxFAIL_MSG("MObject memory leaks detected, see debug log for details.");

      wxLogDebug("%d MObjects leaked:", nCount);
   }

   for ( size_t n = 0; n < nCount; n++ ) {
      wxLogDebug("Object %d: %s", n, gs_aObjects[n]->Dump().c_str());
   }
}

String MObject::Dump() const
{
   MOcheck();
   String str;
   str.Printf("MObject, m_nRef = %d", m_nRef);

   return str;
}

// ----------------------------------------------------------------------------
// debug implementations of other functions
// ----------------------------------------------------------------------------

MObject::MObject()
{
   gs_aObjects.Add(this);

   m_nRef = 1;
   m_magic = MOBJECT_MAGIC;
}

bool MObject::DecRef()
{ 
   MOcheck();
   wxASSERT(m_nRef > 0);

   if ( !--m_nRef )
   {
      gs_aObjects.Remove(this);
      delete this;

      return FALSE;
   }
  
   return TRUE;
}

void
MObject::MOcheck(void) const
{
   wxASSERT(this);
   wxASSERT(m_magic == MOBJECT_MAGIC);
}

#endif //DEBUG
