///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MimePartVirtual.cpp
// Purpose:     a "virtual" MimePart object containing arbitrary data
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-13
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
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

   #include "Mcclient.h"
#endif

#include "MimePartVirtual.h"

// ============================================================================
// MimePartVirtual implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

MimePartVirtual::MimePartVirtual(const String& msgText)
               : m_msgText(msgText)
{
   m_lenHeader = NULL;

   if ( !CclientParseMessage(msgText, &m_env, &m_body, &m_lenHeader) )
   {
      // no idea what is user supposed to make out of this message...
      wxLogError(_("Failed to create a virtual MIME part."));
   }
}

MimePartVirtual::~MimePartVirtual()
{
   mail_free_envelope(&m_env);
   mail_free_body(&m_body);
}

// ----------------------------------------------------------------------------
// data access
// ----------------------------------------------------------------------------

String MimePartVirtual::GetHeaders() const
{
   return String(m_msgText, m_lenHeader);
}

const void *MimePartVirtual::GetRawContent(unsigned long *len) const
{
   size_t lenHeader = m_lenHeader + 2; // +2 for "\r\n"

   if ( len )
      *len = m_msgText.length() - lenHeader;

   return m_msgText.c_str() + lenHeader;
}

