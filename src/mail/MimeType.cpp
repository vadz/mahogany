//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimeType.cpp: implementation of MimeType class
// Purpose:     MimeType is an implementation of MimeType ABC using c-client
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-13 (extracted from MimePartCC.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 by Vadim Zeitlin <vadim@wxwindows.org>
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
   #include "Mcommon.h"
   #include "Mcclient.h" // for body_types
#endif // USE_PCH

#include "MimeType.h"

// ============================================================================
// MimeType implementation
// ============================================================================

// ----------------------------------------------------------------------------
// initializing
// ----------------------------------------------------------------------------

MimeType::MimeType(Primary primary, const String& subtype)
{
   m_primary = primary;
   m_subtype = subtype.Upper();
}

MimeType& MimeType::Assign(const String& mimetype)
{
   String type = mimetype.BeforeFirst('/').Upper();

   m_primary = INVALID;
   for ( size_t n = 0; body_types[n]; n++ )
   {
      if ( type == wxConvertMB2WX(body_types[n]) )
      {
         m_primary = (MimeType::Primary)n;

         break;
      }
   }

   m_subtype = mimetype.AfterFirst('/').Upper();

   return *this;
}

// ----------------------------------------------------------------------------
// accessors
// ----------------------------------------------------------------------------

String MimeType::GetType() const
{
   ASSERT_MSG( IsOk(), _T("using uninitialized MimeType") );

   // body_types is defined in c-client/rfc822.c
   return wxConvertMB2WX(body_types[m_primary]);
}

// ----------------------------------------------------------------------------
// tests
// ----------------------------------------------------------------------------

bool MimeType::Matches(const MimeType& wildcard) const
{
   return m_primary == wildcard.m_primary &&
            (wildcard.m_subtype == '*' || m_subtype == wildcard.m_subtype);
}


