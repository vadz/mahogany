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

#ifdef DEBUG

#include "MObject.h"

#include <wx/log.h>
#include <wx/dynarray.h>

// ----------------------------------------------------------------------------
// this module global variables
// ----------------------------------------------------------------------------
WX_DEFINE_ARRAY(MObjectRC *, ArrayObjects);

static ArrayObjects gs_aObjects;

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
      wxLogDebug("Object %d: %s", n, gs_aObjects[n]->Dump().c_str());
   }
}

String MObjectRC::Dump() const
{
   MOcheck();
   String str;
   str.Printf("MObjectRC, m_nRef = %d", m_nRef);

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

bool MObjectRC::DecRef()
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

#endif //DEBUG
